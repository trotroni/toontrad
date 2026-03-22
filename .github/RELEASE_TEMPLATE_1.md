# ToonTrad {TAG} — Release Notes

> Brouillon auto-généré — compléter avant publication.

---

## Nouveautés

<!-- Décris ici les nouvelles fonctionnalités -->
- 

## Corrections de bugs

<!-- Liste les bugs corrigés -->
- 

## Améliorations

<!-- Améliorations de performance, UX, etc. -->
- 

---

## Installation

| Plateforme | Fichier | Instructions |
|------------|---------|--------------|
| macOS | `ToonTrad-{VERSION}-Darwin.dmg` | Ouvrir le DMG → glisser ToonTrad dans Applications |
| Windows | `ToonTrad-{VERSION}-win64.exe` | Lancer l'installateur, suivre les étapes |

### Prérequis

- **Python ≥ 3.10** installé sur le système
- Les dépendances Python se lancent automatiquement au premier démarrage (`pip install -r requirements.txt`)
- GPU CUDA optionnel — mode CPU automatique si absent

### Moteurs OCR disponibles

| Moteur | GPU | CPU | Langue cible |
|--------|-----|-----|--------------|
| PaddleOCR | ✓ | ✓ | Asiatique / Général |
| EasyOCR | ✓ | ✓ | Général |
| TrOCR | ✓ | ✓ | Texte stylisé |
| manga-ocr | ✓ | ✓ | Manga japonais |
| Tesseract | — | ✓ | Fallback CPU |

---

## Notes de migration

<!-- Si des changements cassent la compatibilité avec des projets existants -->
Aucun changement breaking dans cette version.

---

*ToonTrad {VERSION} — outil open-source de scantrad manga/webtoon*  
*Build CI : {DATE}*