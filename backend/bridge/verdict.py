"""Bye or buy: would a piece the user found online earn its place in this closet?

The question a shopper actually has is not "is this a nice top". It is "does
this go with what I already own". So we answer it the only way that question
can be answered — pair the piece against every garment on the opposite rail and
compare what comes back to what the closet already manages on its own.

That comparison is the whole idea. Judging against a fixed score would not
travel: the engine's totals are weighted by the vibe, so the same number means
different things under different vibes, and a closet full of black jeans scores
differently from one full of florals. A closet's own spread is the one yardstick
that means the same thing everywhere, which is why the engine now reports it.

Nothing here writes to the database. A candidate is never a garment until the
user says they bought it, at which point the normal POST /garments path runs.
"""

from datetime import date, timedelta

from .ingest_engine import pick_outfits

# real wardrobe ids come from AUTOINCREMENT, so they start at 1 and a negative
# one can never collide with a piece the user owns
CANDIDATE_ID = -1

# the anchored run scores one pair per garment on the opposite rail, so this is
# a ceiling on closet size rather than on anything we would want to truncate —
# the counts below are the answer, and a shortlist cannot produce them
MAX_PAIRINGS = 1000

# a piece still on the shelf has never been worn, and the engine pays a bonus
# for that: it is how it pushes things you own and ignore back into rotation.
# left alone every candidate would collect that bonus and outscore the closet on
# novelty, so the answer would always be buy. we hand it a date in the engine's
# neutral recency band instead — we want to know whether it *goes* with the
# closet, not whether it is new. what is left leans slightly against buying,
# which is the safe direction for a feature that tells someone to spend money.
SETTLED_DAYS = 14  # inside (RECENT_DAYS, STALE_DAYS] in engine/src/score.cpp

# how much of the opposite rail a piece has to work with to be worth buying.
# "works" means it beats the closet's median pairing, so a piece exactly as good
# as what the user already owns lands near 0.5 by construction — the bar is
# literally "pulls its weight", not an invented number.
#
# both are scaled by the closet's own survival rate before they are used; see
# _survival for why.
WORKS_RATIO = 0.5
MAYBE_RATIO = 0.25

# how many pairings we hand back for display; the verdict itself counts all of them
SHOWN_PAIRINGS = 6

OPPOSITE = {"Top": "Bottom", "Bottom": "Top"}


def _candidate_row(fields: dict) -> dict:
    """The candidate shaped like a wardrobe row, so it rides along with the
    closet through the same conversion every other garment goes through."""
    color = fields["color"]
    return {
        "id": CANDIDATE_ID,
        "name": fields.get("name") or "this piece",
        "category": fields["category"],
        "l": color["l"],
        "c": color["c"],
        "h": color["h"],
        "hex": fields.get("hex"),
        "pattern": fields["pattern"],
        "formality": fields["formality"],
        "fabric": fields.get("fabric") or "Cotton",
        "silhouette": fields.get("silhouette") or "Straight",
        "length": fields.get("length") or "NA",
        # nothing in a shop is dirty, and the shop does not know today's weather;
        # warmth is the one guess we carry over from the form
        "warmth": fields.get("warmth") or 3,
        "clean": True,
        "weight": fields.get("weight") or 1.0,
        "last_worn": (date.today() - timedelta(days=SETTLED_DAYS)).isoformat(),
    }


def _tier(total: float, bar: dict | None) -> str:
    """Where one pairing sits against the closet's own spread."""
    if bar is None:
        # the closet cannot pair anything on its own, so there is no spread to
        # place this against. saying "works" would be claiming something we did
        # not measure, and every one of these is a pairing the user cannot
        # currently make at all, which is the point worth making instead
        return "unrated"
    if total >= bar["p75"]:
        return "standout"
    if total >= bar["median"]:
        return "works"
    return "weak"


def _survival(run: dict) -> float:
    """What fraction of the pairs a run scored it was willing to offer.

    The engine drops a pair whose colours clash before it ranks anything, and it
    drops them out of the closet's own outfits too — so the median we compare a
    candidate against is the median of the closet's *offerable* pairs, not of
    everything it could put together. Measuring the candidate over everything it
    was tried against while the bar is drawn over survivors only would hold it to
    a standard the closet itself does not meet: on a closet that drops a seventh
    of its pairs, a piece as good as what the user already owns tops out at 0.43
    and never reads as buy. Scaling the bar by this puts them back on one scale.
    """
    scored = run.get("pairs_scored") or 0
    if not scored:
        return 1.0
    return (scored - (run.get("dropped_on_color") or 0)) / scored


def _decide(
    tried: int, works: int, standouts: int, closet_can_dress: bool, survival: float
) -> str:
    if tried == 0:
        # nothing on the opposite rail, so there is no outfit to be had either way
        return "bye"
    if not closet_can_dress:
        # the closet cannot put an outfit together at all right now, so this
        # piece is not competing with anything — it is the thing that unlocks it
        return "buy"

    ratio = works / tried
    if standouts and ratio >= WORKS_RATIO * survival:
        return "buy"
    if standouts or ratio >= MAYBE_RATIO * survival:
        return "maybe"
    return "bye"


def try_on(
    closet: list[dict],
    fields: dict,
    occasion: str | None = None,
    vibe: str | None = None,
    temp_c: float = 18.0,
) -> dict:
    """Two engine runs: the closet alone, then the closet with the candidate in it.

    Leaving occasion and vibe out is the default on purpose — a shopper wants to
    know whether a piece works at all before they want to know whether it works
    for Tuesday. Naming either one narrows the same question.

    Returns the engine's own error payload unchanged when a run fails, so the
    caller can report it the way it reports every other engine failure.
    """
    candidate = _candidate_row(fields)

    # the closet on its own. limit is 1 because we only want `totals`, which the
    # engine computes over every pair regardless of how many it hands back
    baseline = pick_outfits(closet, occasion, vibe, temp_c, limit=1)
    if not baseline.get("ok"):
        return baseline

    # the closet with the candidate anchored into every pair. the anchor is
    # exempt from the filter, so a summer dress still gets an answer in january
    trial = pick_outfits(
        closet + [candidate],
        occasion,
        vibe,
        temp_c,
        limit=MAX_PAIRINGS,
        anchor_id=CANDIDATE_ID,
    )
    if not trial.get("ok"):
        return trial

    bar = baseline.get("totals")
    by_id = {g["id"]: g for g in closet}

    pairings = []
    for pair in trial.get("ranked") or []:
        # one side of an anchored pair is the candidate; the other is what the
        # user already owns, and that is the one worth showing them
        partner = pair["bottom"] if pair["top"]["id"] == CANDIDATE_ID else pair["top"]
        row = by_id.get(partner["id"])
        if row is None:
            continue  # the rail it was filtered onto is the closet, so this cannot fire
        pairings.append(
            {
                "total": pair["total"],
                "tier": _tier(pair["total"], bar),
                "scores": pair["scores"],
                "garment": row,
            }
        )

    works = sum(1 for p in pairings if p["tier"] in ("works", "standout"))
    standouts = sum(1 for p in pairings if p["tier"] == "standout")
    closet_can_dress = bool(bar)

    # the denominator is everything it was held up against, not everything that
    # came back. the engine drops a pair whose colours it has nothing good to
    # say about, so `ranked` is already the survivors — counting the ratio over
    # those would delete a piece's clashes from its own record and turn the
    # closet it agrees with least into the one it scores best against
    tried = trial.get("pairs_scored") or len(pairings)
    clashes = trial.get("dropped_on_color") or 0

    # the closet's own clash rate, which is what the bar gets scaled by
    survival = _survival(baseline)

    return {
        "ok": True,
        "verdict": _decide(tried, works, standouts, closet_can_dress, survival),
        "category": candidate["category"],
        # the rail it was tried against, so the UI can name it without guessing
        "pairs_with": OPPOSITE.get(candidate["category"], "Bottom"),
        "tried": tried,
        "works": works,
        "standouts": standouts,
        # pieces the engine refuses to pair it with at all, on colour alone
        "clashes": clashes,
        "closet_can_dress": closet_can_dress,
        "occasion": trial.get("occasion"),
        "vibe": trial.get("vibe"),
        # kept in the payload for the same reason the engine keeps the per-axis
        # breakdown: a verdict that looks wrong is unarguable without the bar it
        # was measured against
        "baseline": bar,
        # what the bar was scaled by, so a verdict that looks wrong can be
        # argued with rather than just disbelieved
        "closet_survival": survival,
        "candidate_totals": trial.get("totals"),
        "pairings": pairings[:SHOWN_PAIRINGS],
    }
