"""
text_cleaner.py — Nettoyage post-OCR configurable.
Repris de l'ancien gui_app.py (sanitize_text) avec correction du bug
sur les replacements (la boucle n'appliquait jamais k→v).

Fichiers de config optionnels (à placer dans python/) :
  remove_chars.txt  — un caractère à supprimer par ligne
  replacements.txt  — paires  SOURCE=CIBLE  (un par ligne)
"""
import re
import os
import sys


class TextCleaner:

    def __init__(self,
                 remove_chars_path: str = "remove_chars.txt",
                 replacements_path: str = "replacements.txt"):

        self.remove_chars = self._load_remove_chars(remove_chars_path)
        self.replacements = self._load_replacements(replacements_path)

        print(f"[TextCleaner] {len(self.remove_chars)} suppression(s), "
              f"{len(self.replacements)} remplacement(s)", file=sys.stderr)

    def clean(self, text: str) -> str:
        # 1. Suppression des caractères indésirables
        for char in self.remove_chars:
            text = text.replace(char, "")

        # 2. Remplacements mot entier (bug corrigé : l'ancien code n'appliquait pas k→v)
        for k, v in self.replacements.items():
            text = re.sub(r'\b' + re.escape(k) + r'\b', v, text)

        # 3. Normalisation espaces multiples
        text = re.sub(r"\s{2,}", " ", text)

        return text.strip()

    @staticmethod
    def _load_remove_chars(path: str) -> list:
        if not os.path.exists(path):
            return []
        with open(path, "r", encoding="utf-8") as f:
            return [line.strip() for line in f if line.strip()]

    @staticmethod
    def _load_replacements(path: str) -> dict:
        if not os.path.exists(path):
            return {}
        replacements = {}
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if "=" in line:
                    k, v = line.split("=", 1)
                    replacements[k.strip()] = v.strip()
        return replacements