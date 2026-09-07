from collections import OrderedDict
from datetime import date
from io import BytesIO
from uuid import uuid4

from fastapi import FastAPI, HTTPException, Response, UploadFile
from fastapi.concurrency import run_in_threadpool
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

from ..bridge.ingest_engine import occasions, pick_outfits, vibes
from ..db import store
from ..ingestion.pipeline import CUTOUT_DIR, ingest, save_cutout

MAX_UPLOAD = 25 * 1024 * 1024  # bytes
MAX_DRAFTS = 8  # unconfirmed cutouts held in memory

app = FastAPI()

store.init_db()

# serve cutouts so the frontend can display them
CUTOUT_DIR.mkdir(parents=True, exist_ok=True)  # StaticFiles raises if it is missing
app.mount("/cutouts", StaticFiles(directory=CUTOUT_DIR), name="cutouts")

# draft_id -> (cutout image, encoded png)
_drafts: OrderedDict[str, tuple] = OrderedDict()


class GarmentCreate(store.GarmentFields):
    draft_id: str


class OutfitRequest(BaseModel):
    occasion: str
    vibe: str
    temp_c: float = 18.0
    seed: int | None = None
    limit: int = 5


class LastWornUpdate(BaseModel):
    last_worn: date  # ISO 8601; pydantic rejects anything it cannot parse


@app.post("/garments/ingest")
async def ingest_garment(photo: UploadFile):
    if photo.size is not None and photo.size > MAX_UPLOAD:
        raise HTTPException(413, f"upload too large (max {MAX_UPLOAD // 1_048_576}MB)")

    draft, cutout = await run_in_threadpool(ingest, BytesIO(await photo.read()))

    draft_id = uuid4().hex
    buf = BytesIO()
    cutout.save(buf, "PNG")
    _drafts[draft_id] = (cutout, buf.getvalue())
    while len(_drafts) > MAX_DRAFTS:
        _drafts.popitem(last=False)

    return {"draft_id": draft_id, "preview": f"/drafts/{draft_id}.png", **draft}


@app.get("/drafts/{draft_id}.png")
def draft_preview(draft_id: str):
    entry = _drafts.get(draft_id)
    if entry is None:
        raise HTTPException(404, "draft expired")
    return Response(entry[1], media_type="image/png")


@app.post("/garments")
def create_garment(garment: GarmentCreate):
    # the cutout reaches disk only here, once every required field has validated
    entry = _drafts.pop(garment.draft_id, None)
    if entry is None:
        raise HTTPException(404, "draft expired or already saved")

    fields = garment.model_dump(exclude={"draft_id"})
    cutout_path = save_cutout(entry[0], garment.draft_id)
    new_id = store.post_garment(store.GarmentIn(**fields, cutout_path=cutout_path))
    return {"id": new_id, "cutout_path": cutout_path, **fields}


@app.patch("/garments/{garment_id}/last-worn")
def set_last_worn(garment_id: int, body: LastWornUpdate):
    worn = body.last_worn.isoformat()
    if not store.update_garment(garment_id, worn):
        raise HTTPException(404, f"no garment with id {garment_id}")
    return {"id": garment_id, "last_worn": worn}


@app.get("/garments")
def list_garments():
    return store.get_garments()


@app.get("/garments/{garment_id}")
def read_garment(garment_id: int):
    row = store.get_garment(garment_id)
    if row is None:
        raise HTTPException(404, f"no garment with id {garment_id}")
    return row


@app.get("/options")
def options():
    """Valid occasion and vibe names, for the pickers the user chooses from."""
    return {"occasions": occasions(), "vibes": vibes()}


@app.post("/outfits")
def pick(req: OutfitRequest):
    result = pick_outfits(
        store.get_garments(), req.occasion, req.vibe, req.temp_c, req.seed, req.limit
    )
    if not result.get("ok"):
        raise HTTPException(400, result.get("error", "engine failed"))
    return result
