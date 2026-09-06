import json
import sys
from pathlib import Path

from PIL import Image, ImageOps
from pillow_heif import register_heif_opener
from rembg import remove

from .color import extract_dominant_lch, lch_to_hex

register_heif_opener()  # teaches Image.open to read iphone .heic photos

DATA_DIR = Path(__file__).resolve().parent.parent / "data"
CUTOUT_DIR = DATA_DIR / "cutouts"
MAX_EDGE = 2000  # cutouts are ~6x smaller at this cap, with no meaningful color shift


def ingest(photo):
    """photo: a path or file-like object. Returns (draft, cutout).

    Nothing is written to disk — call save_cutout once the user has confirmed.
    """
    img = Image.open(photo)
    img = ImageOps.exif_transpose(img).convert("RGB")  # honour phone orientation
    img.thumbnail((MAX_EDGE, MAX_EDGE))  # shrinks only; preserves aspect ratio

    cutout = remove(img)
    lch = extract_dominant_lch(cutout)

    draft = {
        "color": {"l": lch.l, "c": lch.c, "h": lch.h},
        "hex": lch_to_hex(lch),  # for display
        # these are left blank / guessed — the USER confirms them:
        "category": None,
        "fabric": None,
        "pattern": None,
        "formality": None,
        "clean": True,
    }
    return draft, cutout


def save_cutout(cutout, garment_id) -> str:
    """Persist a confirmed garment's cutout; returns its path relative to data/."""
    CUTOUT_DIR.mkdir(parents=True, exist_ok=True)
    path = CUTOUT_DIR / f"{garment_id}.png"
    cutout.save(path)
    return str(path.relative_to(DATA_DIR))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: python -m backend.ingestion.pipeline <photo> [garment_id]")

    photo = sys.argv[1]
    garment_id = sys.argv[2] if len(sys.argv) > 2 else Path(photo).stem
    draft, cutout = ingest(photo)
    draft["cutout"] = save_cutout(cutout, garment_id)  # the CLI always saves
    print(json.dumps(draft, indent=2))
