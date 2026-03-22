"""
scripts/generate_icons.py
Génère ToonTrad.ico (Windows), ToonTrad.icns et ttproject.icns (macOS).

Stratégie par plateforme :
  macOS  → cairosvg + iconutil (natif)
  Windows → Pillow + svglib (pas besoin de libcairo)
  Fallback → fichier vide pour ne pas bloquer le build

Usage (CMake uniquement) :
    python3 generate_icons.py <src.svg> <out.ico> <out.icns> <out-project.icns>
"""
import sys
import os
import subprocess
import shutil
import tempfile
from pathlib import Path


# ── Détection plateforme ──────────────────────────────────────────────────────

IS_WINDOWS = sys.platform.startswith("win")
IS_MAC     = sys.platform == "darwin"


# ── Installation des dépendances ──────────────────────────────────────────────

def _pip_install(*packages):
    subprocess.run(
        [sys.executable, "-m", "pip", "install", *packages, "--quiet"],
        check=False
    )

def _ensure_deps():
    """Installe les dépendances selon la plateforme."""
    # Pillow : toujours nécessaire
    try:
        import PIL
    except ImportError:
        print("  Installation de Pillow...")
        _pip_install("Pillow")

    if IS_WINDOWS:
        # Sur Windows : svglib + reportlab (pas besoin de libcairo)
        try:
            import svglib
        except ImportError:
            print("  Installation de svglib + reportlab...")
            _pip_install("svglib", "reportlab")
    else:
        # macOS / Linux : cairosvg
        try:
            import cairosvg
        except ImportError:
            print("  Installation de cairosvg...")
            _pip_install("cairosvg")


# ── Conversion SVG → bytes PNG ────────────────────────────────────────────────

def svg_to_png_bytes(svg_path: Path, size: int) -> bytes:
    """Retourne bytes PNG du SVG rendu à <size>×<size>."""

    # Méthode 1 : cairosvg (macOS/Linux)
    if not IS_WINDOWS:
        try:
            import cairosvg
            return cairosvg.svg2png(
                url=str(svg_path), output_width=size, output_height=size)
        except Exception as e:
            print(f"  cairosvg échoué ({e}), essai suivant...")

    # Méthode 2 : svglib + reportlab (Windows, pas de dépendance native)
    try:
        from svglib.svglib import svg2rlg
        from reportlab.graphics import renderPM
        import io
        drawing = svg2rlg(str(svg_path))
        if drawing:
            sx = size / drawing.width
            sy = size / drawing.height
            drawing.width  = size
            drawing.height = size
            drawing.transform = (sx, 0, 0, sy, 0, 0)
            buf = io.BytesIO()
            renderPM.drawToFile(drawing, buf, fmt="PNG")
            return buf.getvalue()
    except Exception as e:
        print(f"  svglib échoué ({e}), essai suivant...")

    # Méthode 3 : Inkscape CLI
    inkscape = shutil.which("inkscape")
    if inkscape:
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as f:
            tmp = f.name
        try:
            r = subprocess.run(
                [inkscape, str(svg_path),
                 f"--export-filename={tmp}",
                 f"--export-width={size}", f"--export-height={size}"],
                capture_output=True, timeout=30
            )
            if r.returncode == 0:
                with open(tmp, "rb") as f:
                    return f.read()
        finally:
            if os.path.exists(tmp):
                os.unlink(tmp)

    raise RuntimeError(
        "Impossible de convertir le SVG. "
        "Aucune méthode disponible (cairosvg, svglib, inkscape)."
    )


# ── Génération .ico (Windows + tous) ─────────────────────────────────────────

def make_ico(svg_path: Path, out_path: Path):
    from PIL import Image
    import io
    sizes = [16, 32, 48, 256]
    images = [Image.open(io.BytesIO(svg_to_png_bytes(svg_path, s))).convert("RGBA")
              for s in sizes]
    images[0].save(str(out_path), format="ICO",
                   sizes=[(s, s) for s in sizes],
                   append_images=images[1:])
    print(f"  OK -> {out_path.name}")


# ── Génération .icns (macOS uniquement) ───────────────────────────────────────

def make_icns(svg_path: Path, out_path: Path):
    from PIL import Image
    import io
    specs = [
        (16,   "icon_16x16.png"),
        (32,   "icon_16x16@2x.png"),
        (32,   "icon_32x32.png"),
        (64,   "icon_32x32@2x.png"),
        (128,  "icon_128x128.png"),
        (256,  "icon_128x128@2x.png"),
        (256,  "icon_256x256.png"),
        (512,  "icon_256x256@2x.png"),
        (512,  "icon_512x512.png"),
        (1024, "icon_512x512@2x.png"),
    ]
    iconset_dir = Path(tempfile.mkdtemp(suffix=".iconset"))
    try:
        for size, name in specs:
            img = Image.open(
                io.BytesIO(svg_to_png_bytes(svg_path, size))).convert("RGBA")
            img.save(str(iconset_dir / name))
        r = subprocess.run(
            ["iconutil", "-c", "icns", str(iconset_dir), "-o", str(out_path)],
            capture_output=True, text=True
        )
        if r.returncode != 0:
            raise RuntimeError(f"iconutil: {r.stderr}")
        print(f"  OK -> {out_path.name}")
    finally:
        shutil.rmtree(iconset_dir, ignore_errors=True)


# ── Traitement d'une cible ────────────────────────────────────────────────────

def process(svg: Path, out_path: Path, label: str) -> bool:
    if out_path.exists() and out_path.stat().st_size > 0:
        print(f"  Déjà présent : {out_path.name}")
        return True

    print(f"Génération de {label}...")
    try:
        if out_path.suffix == ".ico":
            make_ico(svg, out_path)
        elif IS_MAC and shutil.which("iconutil"):
            make_icns(svg, out_path)
        else:
            # Sur Windows ou si iconutil absent : fichier vide
            # (le .icns n'est pas utile hors macOS)
            out_path.touch()
            print(f"  Ignoré (non macOS) : {out_path.name}")
        return True
    except Exception as e:
        print(f"  AVERTISSEMENT {label} : {e}")
        # Crée un fichier vide pour satisfaire CMake OUTPUT et ne pas bloquer
        out_path.touch()
        return False


# ── Point d'entrée ────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 5:
        print("Usage: generate_icons.py <src.svg> <out.ico> <out.icns> <out-project.icns>")
        sys.exit(1)

    svg       = Path(sys.argv[1])
    ico       = Path(sys.argv[2])
    icns_app  = Path(sys.argv[3])
    icns_proj = Path(sys.argv[4])

    if not svg.exists():
        print(f"[generate_icons] SVG introuvable : {svg}", file=sys.stderr)
        for p in (ico, icns_app, icns_proj):
            p.touch()
        sys.exit(0)

    _ensure_deps()

    process(svg, ico,       "ToonTrad.ico")
    process(svg, icns_app,  "ToonTrad.icns")
    process(svg, icns_proj, "ttproject.icns")

    print("[generate_icons] Terminé.")


if __name__ == "__main__":
    main()
