import json
import sys
from pathlib import Path

from color import extract_dominant_lch, lch_to_hex
from PIL import Image
from rembg import remove

CUTOUT_DIR = Path("cutouts")


def ingest(photo_path, garment_id):
    img = Image.open(photo_path).convert("RGB")

    cutout = remove(img)

    CUTOUT_DIR.mkdir(parents=True, exist_ok=True)
    cutout_path = CUTOUT_DIR / f"{garment_id}.png"
    cutout.save(cutout_path)

    lch = extract_dominant_lch(cutout)

    # assemble the draft garment
    return {
        "id": garment_id,
        "color": {"l": lch.l, "c": lch.c, "h": lch.h},
        "hex": lch_to_hex(lch),  # for display
        "cutout": str(cutout_path),
        # these are left blank / guessed — the USER confirms them:
        "category": None,
        "fabric": None,
        "pattern": None,
        "formality": None,
        "clean": True,
    }


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(f"usage: python {Path(__file__).name} <photo> [garment_id]")

    photo = sys.argv[1]
    garment_id = sys.argv[2] if len(sys.argv) > 2 else Path(photo).stem
    print(json.dumps(ingest(photo, garment_id), indent=2))
