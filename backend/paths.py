"""Where the wardrobe lives.

The database and the cutouts are one thing — rows carry a cutout_path — and
neither belongs in the repo. They are your clothes, not the project's, and a
worktree per branch would otherwise mean a wardrobe per branch: add a garment
while working on one and it is missing from the next.

So they sit outside the checkout, in one directory every worktree resolves to
the same way. Set FIT_CHECK_DATA_DIR to put them somewhere else — a test run
that should not touch the real closet wants a temp dir here.
"""

import os
from pathlib import Path

DEFAULT_DATA_DIR = Path.home() / ".fit-check" / "data"

DATA_DIR = Path(
    os.environ.get("FIT_CHECK_DATA_DIR") or DEFAULT_DATA_DIR
).expanduser().resolve()

CUTOUT_DIR = DATA_DIR / "cutouts"
