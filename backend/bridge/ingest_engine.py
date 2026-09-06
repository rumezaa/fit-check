import json
import subprocess
from pathlib import Path

ENGINE = Path(__file__).parent.parent.parent / "engine/build/engine"


def pick_outfits(garments: list[dict], occasion: str, weights: dict) -> list[dict]:
    request = json.dumps(
        {
            "garments": garments,
            "occasion": occasion,
            "weights": weights,
        }
    )
    proc = subprocess.run(
        [str(ENGINE)],
        input=request,
        capture_output=True,
        text=True,
        check=True,
    )
    return json.loads(proc.stdout)["outfits"]
