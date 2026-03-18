"""
ocr_manager.py — Gestion multi-moteurs OCR avec contrôle GPU/CPU.

Pipeline :
  1. Détection des bulles par contours OpenCV  (bubble_detector.py)
  2. Crop de chaque bulle
  3. OCR sur chaque crop avec le moteur choisi
  4. Nettoyage du texte                        (text_cleaner.py)
  5. Réinjection des coordonnées dans l'espace image complet

Fallback : OCR image entière si OpenCV absent ou aucune bulle détectée.
"""
import os
import sys
import tempfile
import subprocess
from typing import List, Dict, Any

from ocr_engine import resolve_device, select_engine_auto
from bubble_detector import BubbleDetector
from text_cleaner import TextCleaner


def _find_tessdata() -> str:
    env = os.environ.get("TESSDATA_PREFIX", "")
    if env and os.path.isdir(env):
        return env
    candidates = [
        "/usr/local/share/tessdata",
        "/opt/homebrew/share/tessdata",
        "/usr/share/tessdata",
        "/usr/local/share/tesseract-ocr/5/tessdata",
        "/usr/share/tesseract-ocr/4.00/tessdata",
    ]
    for p in candidates:
        if os.path.isdir(p):
            return p
    try:
        result = subprocess.run(
            ["brew", "--prefix", "tesseract"],
            capture_output=True, text=True, timeout=5)
        p = os.path.join(result.stdout.strip(), "share", "tessdata")
        if os.path.isdir(p):
            return p
    except Exception:
        pass
    return ""


class OCRManager:

    ENGINES = ["paddleocr", "easyocr", "trocr", "manga-ocr", "tesseract"]

    def __init__(
            self,
            engine: str = "auto",
            device: str = "auto",
            gpu_id: int = 0,
            gpu_memory_fraction: float = 0.5,
            ram_fraction: float = 0.5,
            confidence_threshold: float = 0.4,
            min_bubble_area: int = 2000,
            language: str = "en",
            psm_mode: int = 6,
    ):
        self.confidence_threshold = confidence_threshold
        self.min_bubble_area      = min_bubble_area
        self.language             = language
        self.psm_mode             = psm_mode

        self.device      = resolve_device(device, gpu_id)
        self.gpu_id      = gpu_id
        self.engine_name = engine if engine != "auto" else select_engine_auto(self.device)

        if self.device.startswith("cuda"):
            self._limit_gpu_memory(gpu_memory_fraction, gpu_id)

        if not self.device.startswith("cuda"):
            import resource
            try:
                total_ram = os.sysconf("SC_PAGE_SIZE") * os.sysconf("SC_PHYS_PAGES")
                resource.setrlimit(resource.RLIMIT_AS,
                                   (int(total_ram * ram_fraction),) * 2)
            except Exception:
                pass

        # ── Modules issus de l'ancien code Python ──────────────────────────
        self.bubble_detector = BubbleDetector(min_area=self.min_bubble_area)
        self.cleaner         = TextCleaner()

        print(f"[OCRManager] Moteur: {self.engine_name} | Device: {self.device}",
              file=sys.stderr)
        self.model = self._load_engine()

    # ── Contrôle mémoire GPU ────────────────────────────────────────────────

    def _limit_gpu_memory(self, fraction: float, gpu_id: int):
        try:
            import torch
            torch.cuda.set_per_process_memory_fraction(
                max(0.1, min(fraction, 1.0)), device=gpu_id)
            print(f"[OCRManager] VRAM limitée à {fraction*100:.0f}%", file=sys.stderr)
        except Exception as e:
            print(f"[OCRManager] Impossible de limiter VRAM: {e}", file=sys.stderr)

    # ── Chargement du modèle ────────────────────────────────────────────────

    def _load_engine(self):
        name = self.engine_name
        print(f"[OCRManager] Chargement du moteur '{name}'...", file=sys.stderr)

        if name == "paddleocr":
            from paddleocr import PaddleOCR
            return PaddleOCR(use_angle_cls=True, lang=self._paddle_lang(),
                             use_gpu=self.device.startswith("cuda"), show_log=False)

        elif name == "easyocr":
            import easyocr
            return easyocr.Reader([self._easyocr_lang()],
                                  gpu=self.device.startswith("cuda"), verbose=False)

        elif name == "trocr":
            from transformers import TrOCRProcessor, VisionEncoderDecoderModel
            self.trocr_processor = TrOCRProcessor.from_pretrained(self._trocr_model())
            return VisionEncoderDecoderModel.from_pretrained(
                self._trocr_model()).to(self.device)

        elif name == "manga-ocr":
            from manga_ocr import MangaOcr
            return MangaOcr()

        elif name == "tesseract":
            import pytesseract
            return pytesseract

        else:
            raise ValueError(f"Moteur inconnu : {name}")

    # ── Conversions de langue ───────────────────────────────────────────────

    def _tesseract_lang(self) -> str:
        return {"en":"eng","ja":"jpn","zh":"chi_sim",
                "fr":"fra","de":"deu","es":"spa","ko":"kor"}.get(self.language, "eng")

    def _paddle_lang(self) -> str:
        return {"ja":"japan","zh":"ch","en":"en","fr":"fr",
                "de":"german","es":"es","ko":"korean"}.get(self.language, "en")

    def _easyocr_lang(self) -> str:
        return self.language

    def _trocr_model(self) -> str:
        return "microsoft/trocr-large-printed"

    # ── Point d'entrée principal ────────────────────────────────────────────

    def extract(self, image_path: str) -> List[Dict[str, Any]]:
        """
        Pipeline complet :
          1. Détection bulles (BubbleDetector — logique detection.py)
          2. OCR sur chaque crop
          3. Nettoyage texte (TextCleaner — logique sanitize_text)
          4. Réinjection coordonnées espace image complet
        """
        rects = self.bubble_detector.detect(image_path)

        if not rects:
            print("[OCRManager] Fallback : OCR image entière", file=sys.stderr)
            return self._run_and_clean(image_path)

        try:
            import cv2
        except ImportError:
            return self._run_and_clean(image_path)

        img_cv = cv2.imread(image_path)
        all_blocks = []

        for (x, y, w, h) in rects:
            crop = img_cv[y:y+h, x:x+w]

            tmp = tempfile.NamedTemporaryFile(suffix=".png", delete=False)
            tmp_path = tmp.name
            tmp.close()

            try:
                cv2.imwrite(tmp_path, crop)
                blocks = self._extract_engine(tmp_path)
            except Exception as e:
                print(f"[OCRManager] Erreur crop ({x},{y}): {e}", file=sys.stderr)
                blocks = []
            finally:
                os.unlink(tmp_path)

            # Ignore les crops sans texte significatif (< 3 caractères)
            if len(" ".join(b["text"] for b in blocks).strip()) < 3:
                continue

            for b in blocks:
                b["box"]  = [[pt[0] + x, pt[1] + y] for pt in b["box"]]
                b["text"] = self.cleaner.clean(b["text"])

            all_blocks.extend(blocks)

        print(f"[OCRManager] {len(all_blocks)} bloc(s) au total", file=sys.stderr)
        return all_blocks

    def _run_and_clean(self, image_path: str) -> List[Dict[str, Any]]:
        """OCR image entière + nettoyage (utilisé en fallback)."""
        blocks = self._extract_engine(image_path)
        for b in blocks:
            b["text"] = self.cleaner.clean(b["text"])
        return blocks

    # ── Dispatch vers le moteur ─────────────────────────────────────────────

    def _extract_engine(self, image_path: str) -> List[Dict[str, Any]]:
        name = self.engine_name
        if name == "paddleocr":   return self._extract_paddleocr(image_path)
        elif name == "easyocr":   return self._extract_easyocr(image_path)
        elif name == "trocr":     return self._extract_trocr(image_path)
        elif name == "manga-ocr": return self._extract_mangaocr(image_path)
        elif name == "tesseract": return self._extract_tesseract(image_path)
        return []

    def _filter(self, blocks: list) -> list:
        return [b for b in blocks if b["confidence"] >= self.confidence_threshold]

    # ── Adaptateurs par moteur ──────────────────────────────────────────────

    def _extract_paddleocr(self, path: str) -> list:
        result = self.model.ocr(path, cls=True)
        if not result or not result[0]:
            return []
        blocks = []
        for line in result[0]:
            box_raw, (text, conf) = line
            box = [[int(p[0]), int(p[1])] for p in box_raw]
            blocks.append({"text": text, "confidence": float(conf), "box": box})
        return self._filter(blocks)

    def _extract_easyocr(self, path: str) -> list:
        blocks = []
        for (box_raw, text, conf) in self.model.readtext(path):
            box = [[int(p[0]), int(p[1])] for p in box_raw]
            blocks.append({"text": text, "confidence": float(conf), "box": box})
        return self._filter(blocks)

    def _extract_trocr(self, path: str) -> list:
        import torch
        from PIL import Image

        pil_img = Image.open(path).convert("RGB")
        w, h = pil_img.size
        pixel_values = self.trocr_processor(
            images=pil_img, return_tensors="pt").pixel_values.to(self.device)

        with torch.no_grad():
            ids = self.model.generate(pixel_values)
        text = self.trocr_processor.batch_decode(
            ids, skip_special_tokens=True)[0].strip()

        if not text:
            return []
        return [{"text": text, "confidence": 0.9,
                 "box": [[0,0],[w,0],[w,h],[0,h]]}]

    def _extract_mangaocr(self, path: str) -> list:
        from PIL import Image

        pil_img = Image.open(path).convert("RGB")
        w, h = pil_img.size
        text = self.model(pil_img).strip()

        if not text:
            return []
        return [{"text": text, "confidence": 0.85,
                 "box": [[0,0],[w,0],[w,h],[0,h]]}]

    def _extract_tesseract(self, path: str) -> list:
        import pytesseract
        from PIL import Image

        if not os.environ.get("TESSDATA_PREFIX"):
            found = _find_tessdata()
            if found:
                os.environ["TESSDATA_PREFIX"] = found
                print(f"[tesseract] TESSDATA_PREFIX → {found}", file=sys.stderr)
            else:
                print("[tesseract] AVERTISSEMENT: TESSDATA_PREFIX non défini",
                      file=sys.stderr)

        pil_img = Image.open(path)
        w, h = pil_img.size
        text = pytesseract.image_to_string(
            pil_img, lang=self._tesseract_lang(),
            config=f"--psm {self.psm_mode} --oem 3"
        ).strip().replace("\n", " ")

        if not text:
            return []
        return [{"text": text, "confidence": 0.8,
                 "box": [[0,0],[w,0],[w,h],[0,h]]}]