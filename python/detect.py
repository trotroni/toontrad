"""
detect.py — Point d'entrée appelé par Qt via QProcess.

Usage :
    python3 detect.py '<json_args>'

JSON args attendu :
    {
        "image_path"  : "/chemin/vers/image.jpg",
        "lang"        : "eng",          // langue Tesseract
        "psm"         : 6,              // mode Tesseract
        "min_area"    : 2000,           // aire contour minimale (px²)
        "min_text_len": 3,              // longueur texte minimale
        "inner_ratio" : 0.85            // ratio rectangle interne
    }

Retourne sur stdout un JSON :
    [
        {
            "id"        : 1,
            "rect"      : [x, y, w, h],
            "inner_rect": [ix, iy, iw, ih],
            "raw"       : "texte OCR"
        },
        ...
    ]

En cas d'erreur : exit code != 0, message sur stderr.
"""
import sys
import os
import json
import cv2

# Ajoute le dossier python/ au path pour les imports relatifs
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from ocr import extract_text


def compute_inner_rect(x, y, w, h, ratio: float) -> tuple:
    """Même logique que Bubble.computeInnerRect() C++ et models.py Python."""
    nw = int(w * ratio)
    nh = int(h * ratio)
    ox = (w - nw) // 2
    oy = (h - nh) // 2
    return (x + ox, y + oy, nw, nh)


def detect_bubbles(cv_image, lang: str, psm: int,
                   min_area: int, min_text_len: int,
                   inner_ratio: float) -> list:
    """
    Détecte les bulles via contours OpenCV + OCR Tesseract.
    Logique identique à core/detection.py de la version Python.
    """
    gray = cv2.cvtColor(cv_image, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, 210, 255, cv2.THRESH_BINARY)

    contours, _ = cv2.findContours(
        thresh,
        cv2.RETR_EXTERNAL,
        cv2.CHAIN_APPROX_SIMPLE
    )

    bubbles = []
    id_counter = 1

    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area < min_area:
            continue

        x, y, w, h = cv2.boundingRect(cnt)
        crop = cv_image[y:y + h, x:x + w]

        text = extract_text(crop, lang=lang, psm=psm)
        if len(text) < min_text_len:
            continue

        ix, iy, iw, ih = compute_inner_rect(x, y, w, h, inner_ratio)

        bubbles.append({
            "id"        : id_counter,
            "rect"      : [x, y, w, h],
            "inner_rect": [ix, iy, iw, ih],
            "raw"       : text,
        })
        id_counter += 1

    # Tri par position verticale (comme la version Python)
    bubbles.sort(key=lambda b: b["rect"][1])

    # Réindexation après tri
    for i, b in enumerate(bubbles):
        b["id"] = i + 1

    return bubbles


def main():
    if len(sys.argv) < 2:
        print("Usage: detect.py '<json_args>'", file=sys.stderr)
        sys.exit(1)

    # Parse les arguments JSON
    try:
        args = json.loads(sys.argv[1])
    except json.JSONDecodeError as e:
        print(f"[detect] Erreur JSON args: {e}", file=sys.stderr)
        sys.exit(1)

    image_path   = args.get("image_path", "")
    lang         = args.get("lang",          "eng")
    psm          = int(args.get("psm",       6))
    min_area     = int(args.get("min_area",  2000))
    min_text_len = int(args.get("min_text_len", 3))
    inner_ratio  = float(args.get("inner_ratio", 0.85))

    # Vérifie que l'image existe
    if not image_path or not os.path.isfile(image_path):
        print(f"[detect] Image introuvable: {image_path}", file=sys.stderr)
        sys.exit(1)

    # Charge l'image
    cv_image = cv2.imread(image_path)
    if cv_image is None:
        print(f"[detect] Impossible de lire l'image: {image_path}", file=sys.stderr)
        sys.exit(1)

    # Détection
    try:
        bubbles = detect_bubbles(cv_image, lang, psm, min_area, min_text_len, inner_ratio)
    except Exception as e:
        print(f"[detect] Erreur détection: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)

    # Sortie JSON sur stdout — Qt lit ça
    print(json.dumps(bubbles, ensure_ascii=False))


if __name__ == "__main__":
    main()