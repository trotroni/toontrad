"""
main_ocr.py — Point d'entrée appelé par Qt via QProcess.

Utilisation :
    python3 main_ocr.py '{"image_path": "...", "engine": "auto", ...}'

Retourne sur stdout un JSON :
    [{"text": "...", "confidence": 0.97, "box": [[x,y],[x,y],[x,y],[x,y]]}, ...]

En cas d'erreur : exit code != 0, message sur stderr.
"""
import sys
import json
import os


def main():
    if len(sys.argv) < 2:
        print("Usage: main_ocr.py '<json_args>'", file=sys.stderr)
        sys.exit(1)

    # Parse les arguments JSON
    try:
        args = json.loads(sys.argv[1])
    except json.JSONDecodeError as e:
        print(f"[main_ocr] Erreur JSON args: {e}", file=sys.stderr)
        sys.exit(1)

    image_path = args.get("image_path", "")
    if not image_path or not os.path.isfile(image_path):
        print(f"[main_ocr] Image introuvable: {image_path}", file=sys.stderr)
        sys.exit(1)

    # Instancie l'OCRManager avec les paramètres Qt
    try:
        from ocr_manager import OCRManager
        ocr = OCRManager(
            engine               = args.get("engine",               "auto"),
            device               = args.get("device",               "auto"),
            gpu_id               = int(args.get("gpu_id",           0)),
            gpu_memory_fraction  = float(args.get("gpu_mem",        0.5)),
            ram_fraction         = float(args.get("ram_fraction",   0.5)),
            confidence_threshold = float(args.get("confidence_threshold", 0.4)),
            min_bubble_area      = int(args.get("min_bubble_area",  2000)),
            language             = args.get("language",             "en"),
            psm_mode             = int(args.get("psm_mode",         6)),
        )
    except Exception as e:
        print(f"[main_ocr] Erreur chargement moteur: {e}", file=sys.stderr)
        import traceback; traceback.print_exc(file=sys.stderr)
        sys.exit(1)

    # Lance l'OCR
    try:
        results = ocr.extract(image_path)
    except Exception as e:
        print(f"[main_ocr] Erreur OCR: {e}", file=sys.stderr)
        import traceback; traceback.print_exc(file=sys.stderr)
        sys.exit(1)

    # Sortie JSON sur stdout (Qt lit ça)
    print(json.dumps(results, ensure_ascii=False))


if __name__ == "__main__":
    # Ajoute le dossier python/ au path pour les imports relatifs
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    main()
