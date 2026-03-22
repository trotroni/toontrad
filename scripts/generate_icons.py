"""
scripts/generate_icons.py
Génère ToonTrad.ico (Windows), ToonTrad.icns et ttproject.icns (macOS) depuis un SVG.

Dépendances auto-installées : cairosvg, Pillow
Fallback si cairosvg absent : tente Inkscape en ligne de commande.

Usage (CMake uniquement) :
    python3 generate_icons.py <src.svg> <out.ico> <out.icns> <out-project.icns>
"""
import sys
import os
import subprocess
import shutil
import tempfile
from pathlib import Path


def _ensure_deps():
    for pkg in ("cairosvg", "Pillow"):
        try:
            __import__(pkg.lower().replace("-", "_"))
        except ImportError:
            print(f"  Installation de {pkg}...")
            subprocess.run(
                [sys.executable, "-m", "pip", "install", pkg, "--quiet"],
                check=False
            )


def svg_to_png_bytes(svg_path: Path, size: int) -> bytes:
    try:
        import cairosvg
        return cairosvg.svg2png(url=str(svg_path), output_width=size, output_height=size)
    except Exception:
        pass

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
        "Installez cairosvg (pip install cairosvg) ou Inkscape."
    )


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


def make_icns(svg_path: Path, out_path: Path):
    from PIL import Image
    import io
    specs = [
        (16, "icon_16x16.png"),       (32,  "icon_16x16@2x.png"),
        (32, "icon_32x32.png"),        (64,  "icon_32x32@2x.png"),
        (128, "icon_128x128.png"),     (256, "icon_128x128@2x.png"),
        (256, "icon_256x256.png"),     (512, "icon_256x256@2x.png"),
        (512, "icon_512x512.png"),     (1024,"icon_512x512@2x.png"),
    ]
    iconset_dir = Path(tempfile.mkdtemp(suffix=".iconset"))
    try:
        for size, name in specs:
            img = Image.open(io.BytesIO(svg_to_png_bytes(svg_path, size))).convert("RGBA")
            img.save(str(iconset_dir / name))
        r = subprocess.run(
            ["iconutil", "-c", "icns", str(iconset_dir), "-o", str(out_path)],
            capture_output=True, text=True
        )
        if r.returncode != 0:
            raise RuntimeError(f"iconutil a echoue : {r.stderr}")
        print(f"  OK -> {out_path.name}")
    finally:
        shutil.rmtree(iconset_dir, ignore_errors=True)


def make_icns_fallback(svg_path: Path, out_path: Path):
    """Linux/Windows : PNG renomme en .icns (approximation)."""
    from PIL import Image
    import io
    img = Image.open(io.BytesIO(svg_to_png_bytes(svg_path, 512))).convert("RGBA")
    img.save(str(out_path.with_suffix(".png")))
    shutil.copy(str(out_path.with_suffix(".png")), str(out_path))
    print(f"  OK (PNG fallback) -> {out_path.name}")


def process(svg, out_path, label):
    if out_path.exists():
        print(f"  Deja present : {out_path.name}")
        return True
    print(f"Generation de {label}...")
    try:
        if out_path.suffix == ".ico":
            make_ico(svg, out_path)
        elif shutil.which("iconutil"):
            make_icns(svg, out_path)
        else:
            make_icns_fallback(svg, out_path)
        return True
    except Exception as e:
        print(f"  AVERTISSEMENT {label} : {e}")
        # Cree un fichier vide pour que CMake ne relance pas la cible
        out_path.touch()
        return False


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
        # Cree des fichiers vides pour satisfaire CMake OUTPUT
        for p in (ico, icns_app, icns_proj):
            p.touch()
        sys.exit(0)

    _ensure_deps()
    process(svg, ico,       "ToonTrad.ico")
    process(svg, icns_app,  "ToonTrad.icns")
    process(svg, icns_proj, "ttproject.icns")
    print("[generate_icons] Termine.")


if __name__ == "__main__":
    main()
