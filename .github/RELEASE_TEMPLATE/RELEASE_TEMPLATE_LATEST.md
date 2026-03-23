**Full Changelog**: https://github.com/trotroni/toontrad/commits/v1.0.0-beta

# ToonTrad

> **⚠️ Projet en cours de développement — version bêta**
> ToonTrad est actuellement en phase bêta active. Des fonctionnalités peuvent être incomplètes, instables ou sujettes à changement sans préavis.

---

ToonTrad est un outil desktop open-source conçu pour automatiser le workflow de scantrad (traduction de manga et webtoon). Il couvre l'ensemble du pipeline : téléchargement des RAWs, extraction de texte par OCR, traduction assistée par IA, validation et typesetting final.

## Fonctionnalités principales

- **OCR multi-moteurs** — PaddleOCR, EasyOCR, TrOCR, manga-ocr, Tesseract
- **Détection automatique des bulles** — mode manga et mode webtoon
- **Traduction assistée** — intégration DeepL et autres APIs
- **Interface graphique Qt6** — visualisation, édition et validation des blocs de texte
- **Export** — JSON, PNG typesettée, TXT
- **Gestion de projets** — organisation par dossier, sauvegarde de l'état de traduction

## Compatibilité

| Plateforme | Statut |
|------------|--------|
| macOS (Apple Silicon / Intel) | 🔧 En test |
| Windows 10/11 (64-bit) | 🔧 En test |
| Linux | 🔧 Non testé |

## Installation

Des binaires pré-compilés sont disponibles dans les [Releases](https://github.com/trotroni/toontrad/releases/tag/v1.0.0-beta) du projet.

Pour la compilation depuis les sources, voir la documentation sur le site du projet.

## Prérequis

- Python 3.10+ avec les dépendances listées dans `requirements.txt`
- Tesseract OCR (installé automatiquement au build)

## Documentation & informations

Pour la documentation complète, les guides d'utilisation, la roadmap et les informations sur le projet :

👉 **[Site officiel du projet](https://trotroni.github.io/toontrad/)**

## Contribution

Les contributions sont les bienvenues. Ce projet étant en bêta, n'hésitez pas à ouvrir des issues pour signaler des bugs ou proposer des améliorations.

## Licence

Ce projet est open-source. Voir le fichier `LICENSE` pour les détails.

---

<p align="center"><sub>ToonTrad — fait avec ❤️ pour la communauté scantrad</sub></p>