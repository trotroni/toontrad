"""
ocr_manager.py — Gestion multi-moteurs OCR avec contrôle GPU/CPU.

Moteurs supportés :
  - paddleocr  : GPU/CPU, très rapide, excellent pour l'asiatique
  - easyocr    : GPU/CPU, bon général
  - trocr      : GPU/CPU, meilleure qualité pour texte stylisé
  - manga-ocr  : GPU/CPU, spécialisé manga japonais
  - tesseract  : CPU uniquement, fallback léger

Retourne une liste de dicts :
  [{"text": str, "confidence": float, "box": [[x,y],[x,y],[x,y],[x,y]]}, ...]
"""
import os
import sys
import subprocess
from typing import List, Dict, Any

from ocr_engine import resolve_device, select_engine_auto


def _find_tessdata() -> str:
    """Trouve le dossier tessdata sur le système (macOS/Linux/Windows)."""
    # 1. Variable d'environnement déjà définie
    env = os.environ.get("TESSDATA_PREFIX", "")
    if env and os.path.isdir(env):
        return env

    # 2. Chemins courants macOS (Homebrew Intel / Apple Silicon)
    candidates = [
        "/usr/local/share/tessdata",           # brew Intel
        "/opt/homebrew/share/tessdata",         # brew Apple Silicon
        "/usr/share/tessdata",                  # Linux apt
        "/usr/local/share/tesseract-ocr/5/tessdata",
        "/usr/share/tesseract-ocr/4.00/tessdata",
    ]
    for p in candidates:
        if os.path.isdir(p):
            return p

    # 3. Essaie de demander à brew
    try:
        result = subprocess.run(
            ["brew", "--prefix", "tesseract"],
            capture_output=True, text=True, timeout=5)
        prefix = result.stdout.strip()
        p = os.path.join(prefix, "share", "tessdata")
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

        # Résolution device
        self.device = resolve_device(device, gpu_id)
        self.gpu_id = gpu_id

        # Sélection moteur
        self.engine_name = engine if engine != "auto" else select_engine_auto(self.device)

        # Limite mémoire GPU
        if self.device.startswith("cuda"):
            self._limit_gpu_memory(gpu_memory_fraction, gpu_id)

        # Limite RAM (via environment variable)
        if not self.device.startswith("cuda"):
            import resource
            try:
                total_ram = os.sysconf("SC_PAGE_SIZE") * os.sysconf("SC_PHYS_PAGES")
                limit = int(total_ram * ram_fraction)
                resource.setrlimit(resource.RLIMIT_AS, (limit, limit))
            except Exception:
                pass  # Non supporté sur Windows

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
            use_gpu = self.device.startswith("cuda")
            return PaddleOCR(
                use_angle_cls=True,
                lang=self._paddle_lang(),
                use_gpu=use_gpu,
                show_log=False,
            )

        elif name == "easyocr":
            import easyocr
            use_gpu = self.device.startswith("cuda")
            return easyocr.Reader(
                [self._easyocr_lang()],
                gpu=use_gpu,
                verbose=False,
            )

        elif name == "trocr":
            from transformers import TrOCRProcessor, VisionEncoderDecoderModel
            model_name = self._trocr_model()
            self.trocr_processor = TrOCRProcessor.from_pretrained(model_name)
            model = VisionEncoderDecoderModel.from_pretrained(model_name)
            return model.to(self.device)

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
        mapping = {
            "en": "eng",
            "ja": "jpn",
            "zh": "chi_sim",
            "fr": "fra",
            "de": "deu",
            "es": "spa",
            "ko": "kor",
        }
        return mapping.get(self.language, "eng")

    def _paddle_lang(self) -> str:
        mapping = {"ja": "japan", "zh": "ch", "en": "en", "fr": "fr",
                   "de": "german", "es": "es", "ko": "korean"}
        return mapping.get(self.language, "en")

    def _easyocr_lang(self) -> str:
        return self.language  # EasyOCR utilise les codes ISO

    def _trocr_model(self) -> str:
        if self.language in ("ja", "zh", "ko"):
            return "microsoft/trocr-large-printed"
        return "microsoft/trocr-large-printed"

    # ── Extraction ──────────────────────────────────────────────────────────

    def extract(self, image_path: str) -> List[Dict[str, Any]]:
        """
        Lance l'OCR sur l'image et retourne les blocs filtrés.
        Chaque bloc : {"text": str, "confidence": float, "box": [[x,y]×4]}
        """
        name = self.engine_name

        if name == "paddleocr":
            return self._extract_paddleocr(image_path)
        elif name == "easyocr":
            return self._extract_easyocr(image_path)
        elif name == "trocr":
            return self._extract_trocr(image_path)
        elif name == "manga-ocr":
            return self._extract_mangaocr(image_path)
        elif name == "tesseract":
            return self._extract_tesseract(image_path)
        return []

    # ── Adaptateurs par moteur ──────────────────────────────────────────────

    def _filter(self, blocks: list) -> list:
        """Filtre par seuil de confiance."""
        return [b for b in blocks if b["confidence"] >= self.confidence_threshold]

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
        results = self.model.readtext(path)
        blocks = []
        for (box_raw, text, conf) in results:
            box = [[int(p[0]), int(p[1])] for p in box_raw]
            blocks.append({"text": text, "confidence": float(conf), "box": box})
        return self._filter(blocks)

    def _extract_trocr(self, path: str) -> list:
        """
        TrOCR ne fait pas de détection de zones — on utilise PaddleOCR ou
        EasyOCR pour détecter les boîtes, puis TrOCR pour reconnaître le texte.
        """
        import torch
        from PIL import Image

        # Détection des zones avec EasyOCR (léger, CPU OK)
        import easyocr
        detector = easyocr.Reader([self._easyocr_lang()], gpu=False)
        detection = detector.readtext(path, detail=1)

        pil_img = Image.open(path).convert("RGB")
        blocks = []

        for (box_raw, _, _) in detection:
            box = [[int(p[0]), int(p[1])] for p in box_raw]
            xs = [p[0] for p in box]
            ys = [p[1] for p in box]
            x1, y1, x2, y2 = min(xs), min(ys), max(xs), max(ys)

            crop = pil_img.crop((x1, y1, x2, y2))
            pixel_values = self.trocr_processor(
                images=crop, return_tensors="pt").pixel_values.to(self.device)

            with torch.no_grad():
                ids = self.model.generate(pixel_values)
            text = self.trocr_processor.batch_decode(ids, skip_special_tokens=True)[0]
            text = text.strip()

            if text:
                blocks.append({"text": text, "confidence": 0.9, "box": box})

        return blocks

    def _extract_mangaocr(self, path: str) -> list:
        """manga-ocr reconnaît l'image entière — on l'utilise zone par zone."""
        import easyocr
        from PIL import Image

        detector = easyocr.Reader(["ja"], gpu=False)
        detection = detector.readtext(path, detail=1)

        pil_img = Image.open(path).convert("RGB")
        blocks = []

        for (box_raw, _, _) in detection:
            box = [[int(p[0]), int(p[1])] for p in box_raw]
            xs = [p[0] for p in box]; ys = [p[1] for p in box]
            x1, y1, x2, y2 = min(xs), min(ys), max(xs), max(ys)

            crop = pil_img.crop((x1, y1, x2, y2))
            text = self.model(crop).strip()
            if text:
                blocks.append({"text": text, "confidence": 0.85, "box": box})

        return blocks

    def _extract_tesseract(self, path: str) -> list:
        import pytesseract
        from PIL import Image
        from collections import defaultdict

        # TESSDATA_PREFIX est déjà défini par setup.py (appelé depuis main_ocr.py)
        tessdata = os.environ.get("TESSDATA_PREFIX", "")
        if tessdata:
            print(f"[tesseract] TESSDATA_PREFIX={tessdata}", file=sys.stderr)
        else:
            print("[tesseract] AVERTISSEMENT: TESSDATA_PREFIX non défini", file=sys.stderr)

        img = Image.open(path)
        config = f"--psm {self.psm_mode} --oem 3"

        # DICT
        data = pytesseract.image_to_data(
            img, lang=self._tesseract_lang(), config=config,
            output_type=pytesseract.Output.DICT)

        n = len(data["text"])
        groups = defaultdict(list)
        for i in range(n):
            word = str(data["text"][i]).strip()
            conf = int(data["conf"][i])
            if not word or conf < 0:
                continue
            key = (data["block_num"][i], data["par_num"][i])
            groups[key].append({
                "word": word, "conf": conf,
                "x": data["left"][i], "y": data["top"][i],
                "w": data["width"][i], "h": data["height"][i],
            })

        blocks = []
        for words in groups.values():
            text = " ".join(w["word"] for w in words)
            if not text.strip():
                continue
            conf = sum(w["conf"] for w in words) / len(words) / 100.0
            x1 = min(w["x"]          for w in words)
            y1 = min(w["y"]          for w in words)
            x2 = max(w["x"] + w["w"] for w in words)
            y2 = max(w["y"] + w["h"] for w in words)
            box = [[x1,y1],[x2,y1],[x2,y2],[x1,y2]]
            blocks.append({"text": text, "confidence": conf, "box": box})

        return self._filter(blocks)