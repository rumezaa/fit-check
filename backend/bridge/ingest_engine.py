import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
ENGINE = ROOT / "engine/build/fit-check"
FIXTURES = ROOT / "engine/fixtures"


def _catalog(filename: str, key: str) -> dict[str, dict]:
    entries = json.loads((FIXTURES / filename).read_text())[key]
    return {e["name"]: e for e in entries}


OCCASIONS = _catalog("occasions.json", "occasions")
VIBES = _catalog("aesthetics.json", "aesthetics")


def occasions() -> list[str]:
    return list(OCCASIONS)


def vibes() -> list[str]:
    return list(VIBES)


def to_engine_garment(row: dict) -> dict:
    """A wardrobe row as the engine's loader expects it: color nested, no cutout."""
    return {
        "id": row["id"],
        "name": row["name"] or "",
        "category": row["category"],
        "color": {"l": row["l"], "c": row["c"], "h": row["h"]},
        "hex": row["hex"] or "",
        "pattern": row["pattern"],
        "formality": row["formality"],
        "fabric": row["fabric"] or "Cotton",
        "silhouette": row["silhouette"] or "Straight",
        "length": row["length"] or "NA",
        "clean": bool(row["clean"]),
        "weight": row["weight"],
        "warmth": row["warmth"],
        "last_worn": row["last_worn"],
    }


def pick_outfits(
    garments: list[dict],
    occasion: str,
    vibe: str,
    temp_c: float = 18.0,
    seed: int | None = None,
    limit: int = 5,
) -> dict:
    """Run the engine over the closet. Returns its JSON, ok=False on failure."""
    if not ENGINE.exists():
        return {"ok": False, "error": f"engine binary not built: {ENGINE}"}

    # we send the definitions themselves, not names to look up - the engine
    # used to search a catalog for the entry we had already picked out. that
    # means an unknown name is ours to report now
    chosen_occasion = OCCASIONS.get(occasion)
    if chosen_occasion is None:
        return {
            "ok": False,
            "error": f"unknown occasion: {occasion!r}, expected one of {occasions()}",
        }

    chosen_vibe = VIBES.get(vibe)
    if chosen_vibe is None:
        return {
            "ok": False,
            "error": f"unknown vibe: {vibe!r}, expected one of {vibes()}",
        }

    request = json.dumps(
        {
            "occasion": chosen_occasion,
            "vibe": chosen_vibe,
            "weather": {"temp_c": temp_c},
            "seed": seed,
            "limit": limit,
            "closet": [to_engine_garment(g) for g in garments],
        }
    )

    # a hung engine would otherwise block the worker thread forever
    try:
        proc = subprocess.run(
            [str(ENGINE)], input=request, capture_output=True, text=True, timeout=10
        )
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": "engine timed out"}

    # the engine reports its own errors as json on stdout with exit 1, so we read
    # stdout first and only fall back to stderr when there is nothing there
    if proc.stdout.strip():
        try:
            return json.loads(proc.stdout)
        except json.JSONDecodeError:
            # a crash partway through writing leaves us half a document
            return {
                "ok": False,
                "error": f"engine wrote invalid json (exit {proc.returncode})",
            }
    return {"ok": False, "error": proc.stderr.strip() or "engine produced no output"}
