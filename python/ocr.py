"""
ocr.py — Extraction de texte via Tesseract.
Appelé par detect.py sur chaque crop de bulle.
"""
import pytesseract


def extract_text(image, lang: str = "eng", psm: int = 6) -> str:
    text = pytesseract.image_to_string(
        image,
        lang=lang,
        config=f"--psm {psm}"
    )
    return text.strip().replace("\n", " ")