# fit-check — frontend

The kiosk UI. Svelte 5 + Vite + TypeScript, built for a **Raspberry Pi 7"
touch display at 800×480, landscape**.

## Run

```bash
# terminal 1 — the API (from the repo root)
backend/.venv/bin/uvicorn backend.api.main:app --port 8000

# terminal 2 — the UI
cd frontend && npm run dev      # http://localhost:5173
```

Vite proxies `/garments`, `/outfits`, `/options`, `/cutouts` and `/drafts` to
`127.0.0.1:8000`, so every fetch is origin-relative and the built bundle works
unchanged when the Pi serves it from behind the API.

`npm run check` type-checks. `npm run build` emits `dist/`.

## Layout

The panel is exactly 800×480 and the design is pixel-placed, so `App.svelte`
scales the whole stage to fit the window rather than reflowing. On the Pi that
scale is 1:1.

```
src/
  app.css                 design tokens, Win95 bevel chrome, backgrounds
  lib/
    types.ts              mirrors backend/db/store.py + the engine's enums
    api.ts                typed client, one place that knows about HTTP
    icons.ts              pixel-art bitmaps -> merged SVG paths
    state.svelte.ts       app state (runes), screen routing, engine calls
    components/           Rack, Dialogue, Modal, StripFooter, Wireframe, …
    screens/              one file per screen in the Figma flows
```

## Flows

| Section | Screens |
|---|---|
| Generate an outfit | Home → Occasion → Aesthetic → Confirm → Dressing → Your look |
| Add a garment | Photo → Details → Added |
| My closet | Closet (+ delete confirm) |
| Saved outfits | My outfits |

### Rules worth knowing

- **Tops and bottoms only.** The engine ranks top+bottom pairs; shoes are
  filtered as candidates but never paired.
- **The wand locks a rack.** With a piece locked, `generate()` keeps it and only
  moves the other rack. Browsing is disabled on a locked rack.
- **Length is a hemline**, so it only shows for bottoms. Tops must send `"NA"` —
  the engine rejects anything else.
- **Laundry items stay in the closet but leave the racks** (`clean = false`).
- Every `Category`/`Fabric`/`Length` union in `types.ts` must match
  `engine/src/loader.cpp` exactly; the engine throws on unknown values.

## Endpoints used

| call | endpoint |
|---|---|
| load closet / options / saved | `GET /garments`, `GET /options`, `GET /saved-outfits` |
| add a garment | `POST /garments/ingest` then `POST /garments` |
| delete a garment | `DELETE /garments/{id}` |
| GENERATE | `POST /outfits` (spawns the C++ engine) |
| save a look | `POST /saved-outfits` |
| mark worn | `PATCH /garments/{id}/last-worn` |

Anything the frontend fetches must also be listed in `vite.config.ts`'s proxy,
or the dev server answers with `index.html` and the JSON parse fails.

## Known gaps

- **The laundry button has no endpoint** — toggling `clean` needs a PATCH on
  the garment. The UI shows a notice instead of pretending it worked.
- `public/bg/avatar.png` has no alpha channel, which is why the helper portrait
  is framed in a box. A cut-out with transparency would allow the full-bleed
  avatar from the Figma mockups.
