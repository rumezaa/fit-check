from collections import OrderedDict
from datetime import date
from io import BytesIO
from uuid import uuid4

from fastapi import FastAPI, HTTPException, Response, UploadFile
from fastapi.concurrency import run_in_threadpool
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

from ..bridge.ingest_engine import occasions, pick_outfits, vibes
from ..bridge.verdict import try_on
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
    # all three are optional so one endpoint answers both questions: name an
    # occasion and a vibe to build an outfit, or name an anchor_id to ask what
    # goes with a piece you already picked
    occasion: str | None = None
    vibe: str | None = None
    anchor_id: int | None = None
    temp_c: float = 18.0
    seed: int | None = None
    limit: int = 5


class TryOnRequest(store.GarmentFields):
    """A piece the user is thinking about buying, described the same way a
    garment is — it just never reaches the database unless they buy it.

    occasion and vibe stay optional here for the same reason they are on
    OutfitRequest: "does this go with my closet" is a fair question to ask
    without naming either one.
    """

    occasion: str | None = None
    vibe: str | None = None
    temp_c: float = 18.0


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


@app.delete("/garments/{garment_id}")
def remove_garment(garment_id: int):
    # take the cutout with the row; nothing else references it
    row = store.get_garment(garment_id)
    if row is None:
        raise HTTPException(404, f"no garment with id {garment_id}")
    if not store.delete_garment(garment_id):
        raise HTTPException(404, f"no garment with id {garment_id}")
    cutout = row.get("cutout_path")
    if cutout:
        path = CUTOUT_DIR.parent / cutout
        path.unlink(missing_ok=True)
    return {"id": garment_id, "deleted": True}


@app.get("/saved-outfits")
def list_saved_outfits():
    return store.get_saved_outfits()


@app.post("/saved-outfits")
def create_saved_outfit(outfit: store.SavedOutfitIn):
    # a look needs at least one real piece, or there is nothing to show later
    if outfit.top_id is None and outfit.bottom_id is None:
        raise HTTPException(400, "a saved outfit needs a top or a bottom")
    for gid in (outfit.top_id, outfit.bottom_id):
        if gid is not None and store.get_garment(gid) is None:
            raise HTTPException(404, f"no garment with id {gid}")
    new_id = store.post_saved_outfit(outfit)
    return {"id": new_id, **outfit.model_dump()}


@app.delete("/saved-outfits/{outfit_id}")
def remove_saved_outfit(outfit_id: int):
    if not store.delete_saved_outfit(outfit_id):
        raise HTTPException(404, f"no saved outfit with id {outfit_id}")
    return {"id": outfit_id, "deleted": True}


@app.post("/try-on")
def bye_or_buy(req: TryOnRequest):
    """Bye or buy: score a piece the user found online against their closet.

    Nothing is written. The cutout it was described from stays a draft, so
    buying it afterwards is the ordinary POST /garments with the same draft_id.
    """
    # the engine can only style around a top or a bottom, and a shoe reaching it
    # would come back as a generic bad request instead of naming what went wrong
    if req.category == "Shoes":
        raise HTTPException(400, "bye or buy only works on tops and bottoms")

    fields = req.model_dump(exclude={"occasion", "vibe", "temp_c"})
    result = try_on(
        store.get_garments(), fields, req.occasion, req.vibe, req.temp_c
    )
    if not result.get("ok"):
        raise HTTPException(400, result.get("error", "engine failed"))
    return result


@app.get("/options")
def options():
    """Valid occasion and vibe names, for the pickers the user chooses from."""
    return {"occasions": occasions(), "vibes": vibes()}


@app.post("/outfits")
def pick(req: OutfitRequest):
    # caught here rather than in the engine so the 404 names the garment the
    # user tapped, instead of coming back as a generic bad request
    if req.anchor_id is not None and store.get_garment(req.anchor_id) is None:
        raise HTTPException(404, f"no garment with id {req.anchor_id}")

    result = pick_outfits(
        store.get_garments(),
        req.occasion,
        req.vibe,
        req.temp_c,
        req.seed,
        req.limit,
        req.anchor_id,
    )
    if not result.get("ok"):
        raise HTTPException(400, result.get("error", "engine failed"))
    return result
