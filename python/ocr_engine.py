"""
ocr_engine.py — Détection du matériel et sélection automatique du moteur OCR.
"""
import os


def detect_hardware() -> dict:
    """Retourne les infos GPU/CPU disponibles."""
    hw = {"cuda_available": False, "gpus": [], "cpu_count": os.cpu_count() or 1}
    try:
        import torch
        hw["cuda_available"] = torch.cuda.is_available()
        if hw["cuda_available"]:
            for i in range(torch.cuda.device_count()):
                props = torch.cuda.get_device_properties(i)
                hw["gpus"].append({
                    "id": i,
                    "name": props.name,
                    "vram_gb": round(props.total_memory / 1e9, 1),
                })
    except ImportError:
        pass
    return hw


def resolve_device(device: str, gpu_id: int = 0) -> str:
    """
    Résout le device final.
    'auto' → cuda:N si disponible, sinon cpu
    """
    if device != "auto":
        return device
    hw = detect_hardware()
    if hw["cuda_available"] and hw["gpus"]:
        return f"cuda:{gpu_id}"
    return "cpu"


def select_engine_auto(device: str, vram_gb: float = 0.0) -> str:
    """
    Sélection automatique selon le matériel :
    - CUDA + VRAM ≥ 6 Go → trocr
    - CUDA + VRAM < 6 Go  → easyocr
    - CPU                  → tesseract
    """
    if not device.startswith("cuda"):
        return "tesseract"

    if vram_gb <= 0:
        try:
            import torch
            gid = int(device.split(":")[1]) if ":" in device else 0
            vram_gb = torch.cuda.get_device_properties(gid).total_memory / 1e9
        except Exception:
            vram_gb = 4.0

    return "trocr" if vram_gb >= 6 else "easyocr"
