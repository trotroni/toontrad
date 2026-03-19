"""
detect.py — Appelé par Qt via QProcess.

Deux modes :

  mode=crop (défaut) :
    Reçoit un crop d'image (zone déjà sélectionnée par l'utilisateur).
    Extrait simplement le texte du crop et le retourne.
    Utilisé quand l'utilisateur drag une zone sur l'image.

    Args : image_path, lang, psm
    Retourne : [{"id":1, "rect":[0,0,w,h], "inner_rect":[...], "raw":"texte", "confidence":0.9}]

  mode=reocr :
    Reçoit la liste des zones existantes (rects) et re-OCR chacune.
    Ne crée pas de nouvelles bulles, met juste à jour le texte.
    Utilisé par le bouton "Relancer OCR".

    Args : image_path, lang, psm, rects=[{"id":1,"rect":[x,y,w,h]}, ...]
    Retourne : [{"id":1, "rect":[x,y,w,h], "inner_rect":[...], "raw":"texte", "confidence":0.9}]

Usage :
    python3 detect.py '<json_args>'
"""
import sys
import os
import json
import cv2
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


# ─────────────────────────────────────────────────────────────────────────────
#  TESSDATA_PREFIX — résolution automatique
# ─────────────────────────────────────────────────────────────────────────────

def _setup_tessdata(tessdata_path: str = ""):
    """
    Définit TESSDATA_PREFIX.
    Priorité : 1) tessdata_path passé par C++  2) déjà défini  3) candidates système
    """
    # 1. Chemin explicite passé par Qt (le plus fiable)
    if tessdata_path and os.path.isdir(tessdata_path):
        os.environ["TESSDATA_PREFIX"] = tessdata_path
        print(f"[detect] TESSDATA_PREFIX={tessdata_path} (depuis Qt)", file=sys.stderr)
        return

    # 2. Déjà défini et valide
    env = os.environ.get("TESSDATA_PREFIX", "")
    if env and os.path.isdir(env):
        print(f"[detect] TESSDATA_PREFIX={env} (existant)", file=sys.stderr)
        return

    # 3. Candidates système (fallback)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    exe_dir    = os.path.normpath(os.path.join(script_dir, ".."))

    candidates = [
        os.path.join(exe_dir, "resources", "tessdata"),
        os.path.join(exe_dir, "..", "Resources", "tessdata"),
        os.path.join(exe_dir, "..", "..", "Resources", "tessdata"),
        "/opt/homebrew/share/tessdata",
        "/usr/local/share/tessdata",
        "/usr/share/tessdata",
    ]

    for path in candidates:
        norm = os.path.normpath(path)
        if os.path.isdir(norm):
            os.environ["TESSDATA_PREFIX"] = norm
            print(f"[detect] TESSDATA_PREFIX={norm} (fallback)", file=sys.stderr)
            return

    # 4. brew --prefix tesseract
    try:
        import subprocess
        result = subprocess.run(
            ["brew", "--prefix", "tesseract"],
            capture_output=True, text=True, timeout=5)
        path = os.path.join(result.stdout.strip(), "share", "tessdata")
        if os.path.isdir(path):
            os.environ["TESSDATA_PREFIX"] = path
            print(f"[detect] TESSDATA_PREFIX={path} (brew)", file=sys.stderr)
            return
    except Exception:
        pass

    print("[detect] AVERTISSEMENT: tessdata introuvable", file=sys.stderr)


# ─────────────────────────────────────────────────────────────────────────────
#  Mapping langue ISO → code Tesseract
# ─────────────────────────────────────────────────────────────────────────────

LANG_MAP = {
    "en":  "eng", "eng": "eng",
    "fr":  "fra", "fra": "fra",
    "ja":  "jpn", "jpn": "jpn",
    "zh":  "chi_sim", "chi_sim": "chi_sim",
    "de":  "deu", "deu": "deu",
    "es":  "spa", "spa": "spa",
    "ko":  "kor", "kor": "kor",
}

def resolve_lang(lang: str) -> str:
    code = LANG_MAP.get(lang.lower(), "eng")

    # Vérifie que le fichier traineddata existe réellement
    tessdata = os.environ.get("TESSDATA_PREFIX", "")
    if tessdata:
        path = os.path.join(tessdata, f"{code}.traineddata")
        if not os.path.isfile(path):
            print(f"[detect] {code}.traineddata introuvable → fallback eng",
                  file=sys.stderr)
            # Vérifie que eng existe au moins
            eng_path = os.path.join(tessdata, "eng.traineddata")
            if os.path.isfile(eng_path):
                return "eng"
            # Cherche n'importe quel .traineddata disponible
            available = [f.replace(".traineddata", "")
                         for f in os.listdir(tessdata)
                         if f.endswith(".traineddata")]
            if available:
                print(f"[detect] Langues disponibles: {available}", file=sys.stderr)
                return available[0]

    return code


# ─────────────────────────────────────────────────────────────────────────────
#  Helpers
# ─────────────────────────────────────────────────────────────────────────────

def compute_inner_rect(x, y, w, h, ratio: float):
    nw = int(w * ratio)
    nh = int(h * ratio)
    ox = (w - nw) // 2
    oy = (h - nh) // 2
    return (x + ox, y + oy, nw, nh)


def ocr_region(pil_img, tess_lang: str, psm: int) -> tuple:
    """
    Lance Tesseract sur une image PIL et retourne (texte, confiance).
    """
    import pytesseract
    data = pytesseract.image_to_data(
        pil_img,
        lang=tess_lang,
        config=f"--psm {psm} --oem 3",
        output_type=pytesseract.Output.DICT
    )
    words = []
    confs = []
    for i in range(len(data["text"])):
        word = str(data["text"][i]).strip()
        conf = int(data["conf"][i])
        if word and conf > 0:
            words.append(word)
            confs.append(conf)

    text     = " ".join(words).strip()
    avg_conf = (sum(confs) / len(confs) / 100.0) if confs else 0.0
    return text, avg_conf


# ─────────────────────────────────────────────────────────────────────────────
#  Mode CROP : OCR direct sur un crop (drag utilisateur)
# ─────────────────────────────────────────────────────────────────────────────

def mode_crop(cv_image, tess_lang: str, psm: int, inner_ratio: float) -> list:
    """
    Extrait le texte du crop entier.
    Retourne une liste avec un seul bloc couvrant tout le crop.
    """
    from PIL import Image as PILImage
    h, w = cv_image.shape[:2]
    pil_img = PILImage.fromarray(cv2.cvtColor(cv_image, cv2.COLOR_BGR2RGB))

    text, conf = ocr_region(pil_img, tess_lang, psm)

    if not text:
        print("[detect] Aucun texte trouvé dans le crop", file=sys.stderr)
        return []

    ix, iy, iw, ih = compute_inner_rect(0, 0, w, h, inner_ratio)
    return [{
        "id":         1,
        "rect":       [0, 0, w, h],
        "inner_rect": [ix, iy, iw, ih],
        "raw":        text,
        "confidence": round(conf, 2),
    }]


# ─────────────────────────────────────────────────────────────────────────────
#  Mode REOCR : re-OCR sur zones existantes
# ─────────────────────────────────────────────────────────────────────────────

def mode_reocr(cv_image, rects: list, tess_lang: str, psm: int,
               inner_ratio: float) -> list:
    """
    Pour chaque rect fourni, crop l'image et re-OCR.
    Retourne les mêmes IDs avec le texte mis à jour.
    """
    from PIL import Image as PILImage
    h_img, w_img = cv_image.shape[:2]
    results = []

    for entry in rects:
        bid = entry.get("id", 0)
        r   = entry.get("rect", [])
        if len(r) < 4:
            continue

        x, y, w, h = int(r[0]), int(r[1]), int(r[2]), int(r[3])

        # Clamp aux dimensions de l'image
        x  = max(0, min(x, w_img - 1))
        y  = max(0, min(y, h_img - 1))
        x2 = min(x + w, w_img)
        y2 = min(y + h, h_img)
        w  = x2 - x
        h  = y2 - y

        if w <= 0 or h <= 0:
            continue

        crop    = cv_image[y:y2, x:x2]
        pil_img = PILImage.fromarray(cv2.cvtColor(crop, cv2.COLOR_BGR2RGB))
        text, conf = ocr_region(pil_img, tess_lang, psm)

        ix, iy, iw, ih = compute_inner_rect(x, y, w, h, inner_ratio)
        results.append({
            "id":         bid,
            "rect":       [x, y, w, h],
            "inner_rect": [ix, iy, iw, ih],
            "raw":        text,
            "confidence": round(conf, 2),
        })
        print(f"[detect] ReOCR bulle #{bid}: '{text[:40]}'", file=sys.stderr)

    return results


# ─────────────────────────────────────────────────────────────────────────────
#  Point d'entrée
# ─────────────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 2:
        print("Usage: detect.py '<json_args>'", file=sys.stderr)
        sys.exit(1)

    try:
        args = json.loads(sys.argv[1])
    except json.JSONDecodeError as e:
        print(f"[detect] Erreur JSON: {e}", file=sys.stderr)
        sys.exit(1)

    # Tessdata en premier — avant tout import pytesseract
    tessdata    = args.get("tessdata_path", "")
    _setup_tessdata(tessdata)

    mode        = args.get("mode",        "crop")
    image_path  = args.get("image_path",  "")
    lang        = args.get("language",    "eng")
    psm         = int(args.get("psm_mode",       6))
    inner_ratio = float(args.get("inner_ratio",  0.85))

    tess_lang = resolve_lang(lang)
    print(f"[detect] mode={mode} lang={lang}→{tess_lang}", file=sys.stderr)

    if not image_path or not os.path.isfile(image_path):
        print(f"[detect] Image introuvable: {image_path}", file=sys.stderr)
        sys.exit(1)

    cv_image = cv2.imread(image_path)
    if cv_image is None:
        print(f"[detect] Impossible de lire: {image_path}", file=sys.stderr)
        sys.exit(1)

    print(f"[detect] {image_path} ({cv_image.shape[1]}x{cv_image.shape[0]})",
          file=sys.stderr)

    try:
        if mode == "crop":
            results = mode_crop(cv_image, tess_lang, psm, inner_ratio)

        elif mode == "reocr":
            rects = args.get("rects", [])
            results = mode_reocr(cv_image, rects, tess_lang, psm, inner_ratio)

        else:
            print(f"[detect] Mode inconnu: {mode}", file=sys.stderr)
            sys.exit(1)

    except Exception as e:
        import traceback
        print(f"[detect] Erreur: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)

    print(f"[detect] {len(results)} résultat(s)", file=sys.stderr)
    print(json.dumps(results, ensure_ascii=False))


if __name__ == "__main__":
    main()