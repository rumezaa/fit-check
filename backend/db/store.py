import sqlite3
from contextlib import closing
from pathlib import Path
from datetime import date, datetime
from typing import Literal

from pydantic import BaseModel, Field

DB = Path(__file__).resolve().parent.parent / "data/wardrobe.db"
SCHEMA = Path(__file__).parent / "schema.sql"


# these must match the enum tables in engine/src/loader.cpp exactly — the engine
# throws on anything else
Category = Literal["Top", "Bottom", "Shoes"]
Pattern = Literal["Solid", "Floral", "PolkaDot", "Stripes", "Graphic"]
Formality = Literal["Casual", "Business", "Elegant"]
Fabric = Literal[
    "Cotton", "Denim", "Knit", "Linen", "Wool", "Silk", "Satin", "Lace", "Leather"
]
Silhouette = Literal["Fitted", "Straight", "Flowy", "Oversized"]
Length = Literal["NA", "Mini", "Midi", "Maxi"]


class Color(BaseModel):
    l: float
    c: float
    h: float


class GarmentFields(BaseModel):
    """Everything the user fills in. The cutout is added by the server."""

    name: str
    category: Category
    color: Color
    hex: str | None = None
    pattern: Pattern
    formality: Formality
    fabric: Fabric
    silhouette: Silhouette = "Straight"
    # tops have no hemline, so NA is the honest default
    length: Length = "NA"
    # the engine only has bands for 1-5; anything else silently matches nothing
    warmth: int = Field(3, ge=1, le=5)
    clean: bool = True
    weight: float = 1.0
    # a date, not a str — the engine rejects anything that isn't ISO, and one
    # bad row fails the whole closet rather than just that garment
    last_worn: date | None = None


class GarmentIn(GarmentFields):
    cutout_path: str


def _conn():
    DB.parent.mkdir(parents=True, exist_ok=True)
    conn = sqlite3.connect(DB)
    conn.row_factory = sqlite3.Row
    return conn


# closing() closes the connection; the bare `c` commits/rolls back the transaction
# — sqlite3's own context manager only does the latter


def init_db():
    with closing(_conn()) as c, c:
        c.executescript(SCHEMA.read_text())
        # schema.sql only runs for a new DB; bring an existing one forward
        cols = {r["name"] for r in c.execute("PRAGMA table_info(wardrobe)")}
        for column in ("fabric", "silhouette", "length"):
            if column not in cols:
                c.execute(f"ALTER TABLE wardrobe ADD COLUMN {column} TEXT")
        # saved_outfits arrived after the first databases did
        c.execute(
            """CREATE TABLE IF NOT EXISTS saved_outfits (
                   id         INTEGER PRIMARY KEY AUTOINCREMENT,
                   top_id     INTEGER REFERENCES wardrobe(id) ON DELETE CASCADE,
                   bottom_id  INTEGER REFERENCES wardrobe(id) ON DELETE CASCADE,
                   occasion   TEXT NOT NULL,
                   vibe       TEXT NOT NULL,
                   created_at TEXT NOT NULL
               )"""
        )


def post_garment(g: GarmentIn) -> int:
    with closing(_conn()) as c, c:
        cur = c.execute(
            """INSERT INTO wardrobe
               (name, category, l, c, h, hex, pattern, formality, fabric,
                silhouette, length, warmth, clean, weight, last_worn, cutout_path)
               VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)""",
            (
                g.name,
                g.category,
                g.color.l,
                g.color.c,
                g.color.h,
                g.hex,
                g.pattern,
                g.formality,
                g.fabric,
                g.silhouette,
                g.length,
                g.warmth,
                g.clean,
                g.weight,
                g.last_worn.isoformat() if g.last_worn else None,
                g.cutout_path,
            ),
        )
        return cur.lastrowid


def get_garments() -> list[dict]:
    with closing(_conn()) as c, c:
        rows = c.execute("SELECT * FROM wardrobe").fetchall()
        return [dict(r) for r in rows]


def get_garment(garment_id: int) -> dict | None:
    with closing(_conn()) as c, c:
        row = c.execute("SELECT * FROM wardrobe WHERE id = ?", (garment_id,)).fetchone()
        return dict(row) if row else None


def update_garment(garment_id: int, last_worn: str) -> bool:
    with closing(_conn()) as c, c:
        cur = c.execute(
            "UPDATE wardrobe SET last_worn = ? WHERE id = ?", (last_worn, garment_id)
        )
        return cur.rowcount > 0


def delete_garment(garment_id: int) -> bool:
    with closing(_conn()) as c, c:
        cur = c.execute("DELETE FROM wardrobe WHERE id = ?", (garment_id,))
        return cur.rowcount > 0


class SavedOutfitIn(BaseModel):
    """A look the user kept: the pair the engine produced, plus why."""

    top_id: int | None = None
    bottom_id: int | None = None
    occasion: str
    vibe: str


def post_saved_outfit(o: SavedOutfitIn) -> int:
    with closing(_conn()) as c, c:
        cur = c.execute(
            """INSERT INTO saved_outfits (top_id, bottom_id, occasion, vibe, created_at)
               VALUES (?,?,?,?,?)""",
            (o.top_id, o.bottom_id, o.occasion, o.vibe, datetime.now().isoformat()),
        )
        return cur.lastrowid


def get_saved_outfits() -> list[dict]:
    """Newest first, with each garment inlined so the UI needs one call."""
    with closing(_conn()) as c, c:
        rows = c.execute(
            "SELECT * FROM saved_outfits ORDER BY id DESC"
        ).fetchall()
        out = []
        for r in rows:
            look = dict(r)
            for side in ("top", "bottom"):
                gid = look[f"{side}_id"]
                g = None
                if gid is not None:
                    row = c.execute(
                        "SELECT * FROM wardrobe WHERE id = ?", (gid,)
                    ).fetchone()
                    g = dict(row) if row else None
                look[side] = g
            out.append(look)
        return out


def delete_saved_outfit(outfit_id: int) -> bool:
    with closing(_conn()) as c, c:
        cur = c.execute("DELETE FROM saved_outfits WHERE id = ?", (outfit_id,))
        return cur.rowcount > 0
