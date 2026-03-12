"""
scripts/download_tessdata.py
Appelé automatiquement par CMake lors du build.
Télécharge les fichiers tessdata manquants dans resources/tessdata/.
L'utilisateur final ne voit jamais ce script.

Usage (CMake uniquement) :
    python3 scripts/download_tessdata.py <resources/tessdata dir>
"""
import sys
import os
import urllib.request
import urllib.error

TESSDATA_URLS = {
    "eng.traineddata": "https://github.com/tesseract-ocr/tessdata_fast/raw/main/eng.traineddata",
    "jpn.traineddata": "https://github.com/tesseract-ocr/tessdata_fast/raw/main/jpn.traineddata",
    "chi_sim.traineddata": "https://github.com/tesseract-ocr/tessdata_fast/raw/main/chi_sim.traineddata",
    "fra.traineddata": "https://github.com/tesseract-ocr/tessdata_fast/raw/main/fra.traineddata",
    "deu.traineddata": "https://github.com/tesseract-ocr/tessdata_fast/raw/main/deu.traineddata",
    "spa.traineddata": "https://github.com/tesseract-ocr/tessdata_fast/raw/main/spa.traineddata",
    "kor.traineddata": "https://github.com/tesseract-ocr/tessdata_fast/raw/main/kor.traineddata",
}


def download_file(url: str, dest: str):
    print(f"  Téléchargement : {os.path.basename(dest)}")

    def _progress(count, block_size, total):
        if total > 0:
            pct = min(count * block_size * 100 // total, 100)
            print(f"    {pct}%", end="\r")

    urllib.request.urlretrieve(url, dest, _progress)
    print(f"  OK → {dest}                    ")


def main():
    if len(sys.argv) < 2:
        print("Usage: download_tessdata.py <tessdata_dir>")
        sys.exit(1)

    tessdata_dir = sys.argv[1]
    os.makedirs(tessdata_dir, exist_ok=True)

    print(f"[tessdata] Vérification dans : {tessdata_dir}")

    missing = []
    for filename in TESSDATA_URLS:
        path = os.path.join(tessdata_dir, filename)
        if not os.path.isfile(path):
            missing.append(filename)

    if not missing:
        print("[tessdata] Tous les fichiers sont déjà présents. Rien à faire.")
        return

    print(f"[tessdata] {len(missing)} fichier(s) manquant(s) → téléchargement...")
    errors = []
    for filename in missing:
        url  = TESSDATA_URLS[filename]
        dest = os.path.join(tessdata_dir, filename)
        try:
            download_file(url, dest)
        except Exception as e:
            print(f"  ERREUR : {filename} — {e}")
            errors.append(filename)

    if errors:
        print(f"[tessdata] ÉCHEC pour : {', '.join(errors)}")
        print("[tessdata] Vérifie ta connexion internet et relance le build.")
        sys.exit(1)

    print(f"[tessdata] Téléchargement terminé. {len(missing)} fichier(s) ajouté(s).")


if __name__ == "__main__":
    main()