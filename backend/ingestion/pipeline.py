import json
import sys
from functools import lru_cache
from pathlib import Path

from PIL import Image, ImageOps
from pillow_heif import register_heif_opener
from rembg import new_session, remove

from .color import extract_dominant_lch, lch_to_hex

register_heif_opener()  # teaches Image.open to read iphone .heic photos

# re-exported: api.main imports CUTOUT_DIR from here, and both point outside
# the checkout so a wardrobe is not per-branch. see backend/paths.py
from ..paths import CUTOUT_DIR, DATA_DIR
MODEL = "u2netp"  # 4.6MB, against the 1GB bria-rmbg rembg now defaults to
WORK_EDGE = 1536  # below ~1024 the dominant colour drifts light (dL +7 at 768):
# downsampling blends a garment's dark folds into their lighter neighbours, and
# the cluster extract_dominant_lch settles on moves with them.
CUTOUT_EDGE = 768  # display only, and nothing renders above 200px


@lru_cache(maxsize=1)
def _session():
    """Built once and reused.

    remove() with no session= builds a fresh ONNX session per call, and that
    rebuild is most of the cost of a photo. Lazy rather than module-level so
    that importing this file never reaches for the network - the model is
    fetched and cached on the first ingest.
    """
    return new_session(MODEL)


def ingest(photo):
    """photo: a path or file-like object. Returns (draft, cutout).

    Nothing is written to disk — call save_cutout once the user has confirmed.
    """
    img = Image.open(photo)
    img = ImageOps.exif_transpose(img).convert("RGB")  # honour phone orientation
    img.thumbnail((WORK_EDGE, WORK_EDGE))  # shrinks only; preserves aspect ratio

    # post_process_mask cleans up the background u2netp leaves haloed around
    # shoulders and shadows; it costs ~40ms and shrinks the png
    cutout = remove(img, session=_session(), post_process_mask=True)
    lch = extract_dominant_lch(cutout)  # read the colour before shrinking
    cutout.thumbnail((CUTOUT_EDGE, CUTOUT_EDGE))

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
