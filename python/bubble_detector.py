"""
bubble_detector.py — Détection des bulles de dialogue par contours OpenCV.
Logique reprise et améliorée de l'ancien detection.py.
"""
import sys
from typing import List, Tuple


class BubbleDetector:

    def __init__(
            self,
            white_threshold: int  = 210,
            min_area: int         = 2000,
            max_area_ratio: float = 0.80,
            min_ratio: float      = 0.20,
            max_ratio: float      = 5.00,
            padding: int          = 4,
    ):
        self.white_threshold = white_threshold
        self.min_area        = min_area
        self.max_area_ratio  = max_area_ratio
        self.min_ratio       = min_ratio
        self.max_ratio       = max_ratio
        self.padding         = padding

    def detect(self, image_path: str) -> List[Tuple[int, int, int, int]]:
        """
        Retourne une liste de (x, y, w, h) triée de haut en bas.
        Retourne [] si OpenCV absent ou image illisible.
        """
        try:
            import cv2
        except ImportError:
            print("[BubbleDetector] OpenCV non installé — pip install opencv-python",
                  file=sys.stderr)
            return []

        img = cv2.imread(image_path)
        if img is None:
            print(f"[BubbleDetector] Image illisible : {image_path}", file=sys.stderr)
            return []

        rects = self._find_contours(img)
        print(f"[BubbleDetector] {len(rects)} bulle(s) détectée(s)", file=sys.stderr)
        return rects

    def detect_from_point(self, image_path: str, px: int, py: int
                          ) -> Tuple[int, int, int, int] | None:
        """
        Retourne la bulle dont le centre est le plus proche du point (px, py).
        Utilisé pour le clic manuel dans OCRwindow.
        """
        rects = self.detect(image_path)
        if not rects:
            return None

        best, best_dist = None, float("inf")
        for (x, y, w, h) in rects:
            dist = (x + w // 2 - px) ** 2 + (y + h // 2 - py) ** 2
            if dist < best_dist:
                best_dist = dist
                best = (x, y, w, h)
        return best

    def _find_contours(self, img) -> List[Tuple[int, int, int, int]]:
        import cv2
        import numpy as np

        h_img, w_img = img.shape[:2]

        # Identique à l'ancien detection.py : gray → threshold(210) → contours
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        _, thresh = cv2.threshold(
            gray, self.white_threshold, 255, cv2.THRESH_BINARY)

        # Amélioration : fermeture morphologique pour souder les contours ouverts
        kernel = np.ones((3, 3), np.uint8)
        thresh = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, kernel, iterations=2)

        contours, _ = cv2.findContours(
            thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        rects = []
        for cnt in contours:
            area = cv2.contourArea(cnt)

            if area < self.min_area:
                continue
            if area > w_img * h_img * self.max_area_ratio:
                continue

            x, y, w, h = cv2.boundingRect(cnt)

            ratio = w / max(h, 1)
            if ratio < self.min_ratio or ratio > self.max_ratio:
                continue

            # Padding autour du crop (améliore l'OCR sur les bords)
            x = max(0, x - self.padding)
            y = max(0, y - self.padding)
            w = min(w_img - x, w + self.padding * 2)
            h = min(h_img - y, h + self.padding * 2)

            rects.append((x, y, w, h))

        rects.sort(key=lambda r: r[1])
        return rects