"""Bring cutouts already on disk up to the form ingestion writes today.

Cutouts saved before pipeline.for_display existed are whole photo frames with
the garment somewhere inside, so the app draws them off-centre. This rewrites
each one through the same function a new capture goes through. It reads only
the png's own alpha, so it needs neither the model nor the original photo.

    python -m backend.ingestion.recrop            # rewrite backend/data/cutouts
    python -m backend.ingestion.recrop --dry-run  # just say what would change

Safe to re-run: a cropped cutout's mask already fills its frame, so a second
pass finds nothing to take off.
"""

import argparse
import os
import sys
from pathlib import Path

from PIL import Image

from .pipeline import CUTOUT_DIR, for_display


def recrop_file(path, dry_run=False):
    """Returns (before, after) sizes, or None if the file was already tight."""
    with Image.open(path) as img:
        before = img.size
        cutout = for_display(img.convert("RGBA"))
        after = cutout.size
        if after == before:
            return None
        if not dry_run:
            # via a temp file so an interrupted run cannot leave a half-written
            # png where the app expects a garment
            tmp = path.with_suffix(".png.tmp")
            cutout.save(tmp, "PNG")  # named, since .tmp tells Pillow nothing
            os.replace(tmp, path)
    return before, after


def main(argv=None):
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--dry-run", action="store_true", help="report, do not write")
    ap.add_argument("--dir", type=Path, default=CUTOUT_DIR, help="cutout directory")
    args = ap.parse_args(argv)

    if not args.dir.is_dir():
        sys.exit(f"no such directory: {args.dir}")
    files = sorted(p for p in args.dir.iterdir() if p.suffix == ".png")
    if not files:
        sys.exit(f"no cutouts in {args.dir}")

    changed = 0
    for path in files:
        result = recrop_file(path, args.dry_run)
        if result is None:
            print(f"  {path.name}  already tight")
            continue
        changed += 1
        (bw, bh), (aw, ah) = result
        print(f"  {path.name}  {bw}x{bh} -> {aw}x{ah}")

    verb = "would rewrite" if args.dry_run else "rewrote"
    print(f"{verb} {changed} of {len(files)} cutouts")


if __name__ == "__main__":
    main()
