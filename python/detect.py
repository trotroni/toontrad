"""
detect.py — Appelé par Qt via QProcess.

Usage :
    python3 detect.py '<json_args>'

Retourne sur stdout :
    [{"id":1, "rect":[x,y,w,h], "inner_rect":[ix,iy,iw,ih],
      "raw":"texte", "confidence":0.9}, ...]
"""
import sys
import os
import json
import cv2

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ocr import extract_text


def compute_inner_rect(x, y, w, h, ratio: float):
    nw = int(w * ratio)
    nh = int(h * ratio)
    ox = (w - nw) // 2
    oy = (h - nh) // 2
    return (x + ox, y + oy, nw, nh)


def detect(cv_image, lang, psm, min_area, min_text_len, inner_ratio,
           confidence_threshold):
    gray = cv2.cvtColor(cv_image, cv2.COLOR_BGR2GRAY)
    _, thresh = cv2.threshold(gray, 210, 255, cv2.THRESH_BINARY)

    contours, _ = cv2.findContours(
        thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    bubbles = []
    id_counter = 1

    for cnt in contours:
        area = cv2.contourArea(cnt)
        if area < min_area:
            continue

        x, y, w, h = cv2.boundingRect(cnt)
        crop = cv_image[y:y + h, x:x + w]
        text = extract_text(crop, lang=lang, psm=psm)

        if len(text.strip()) < min_text_len:
            continue

        ix, iy, iw, ih = compute_inner_rect(x, y, w, h, inner_ratio)

        bubbles.append({
            "id":         id_counter,
            "rect":       [x, y, w, h],
            "inner_rect": [ix, iy, iw, ih],
            "raw":        text.strip(),
            "confidence": 0.9,
        })
        id_counter += 1

    # Tri par position verticale
    bubbles.sort(key=lambda b: b["rect"][1])
    for i, b in enumerate(bubbles):
        b["id"] = i + 1

    return bubbles


def main():
    if len(sys.argv) < 2:
        print("Usage: detect.py '<json_args>'", file=sys.stderr)
        sys.exit(1)

    try:
        args = json.loads(sys.argv[1])
    except json.JSONDecodeError as e:
        print(f"[detect] Erreur JSON: {e}", file=sys.stderr)
        sys.exit(1)

    image_path        = args.get("image_path", "")
    lang              = args.get("language",          "eng")
    psm               = int(args.get("psm_mode",      6))
    min_area          = int(args.get("min_bubble_area", 2000))
    min_text_len      = int(args.get("min_text_len",  3))
    inner_ratio       = float(args.get("inner_ratio", 0.85))
    conf_threshold    = float(args.get("confidence_threshold", 0.4))

    if not image_path or not os.path.isfile(image_path):
        print(f"[detect] Image introuvable: {image_path}", file=sys.stderr)
        sys.exit(1)

    cv_image = cv2.imread(image_path)
    if cv_image is None:
        print(f"[detect] Impossible de lire: {image_path}", file=sys.stderr)
        sys.exit(1)

    try:
        bubbles = detect(cv_image, lang, psm, min_area, min_text_len,
                         inner_ratio, conf_threshold)
    except Exception as e:
        import traceback
        print(f"[detect] Erreur: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)

    print(json.dumps(bubbles, ensure_ascii=False))


if __name__ == "__main__":
    main()
