# fit-check

A closet kiosk: a C++ engine that scores outfits, a FastAPI backend that holds
the wardrobe, and a Svelte frontend to tap through.

## Running it

```sh
./start
```

That builds the engine, sets up the python and node deps, brings up the API and
the frontend together, and opens the browser. Ctrl-c stops both halves.

Each worktree gets its own pair of ports, so you can leave one branch up while
you start another and flip between the two tabs:

```sh
./start --list
```

```
WORKTREE       BRANCH                     PORTS        STATE
fit-check      main                       8000/5173    up  → http://localhost:5173
copenhagen     start-script               8012/5185    down
```

The primary checkout keeps the usual 8000/5173. Other flags: `--fresh` to
rebuild everything from scratch, `--no-engine` to skip the C++ build, `--no-open`
to stay out of the browser, `--offset N` to pick the ports yourself, `--help`
for the rest.

## Doing it by hand

```sh
cmake -S engine -B engine/build && cmake --build engine/build   # engine
uvicorn backend.api.main:app --port 8000                        # api, from the repo root
npm --prefix frontend run dev                                   # frontend on 5173
```

The frontend proxies `/garments`, `/outfits` and friends to the API, so both
have to be up. `FIT_CHECK_API_PORT` and `FIT_CHECK_WEB_PORT` move those ports —
that is how `./start` fits several branches on one machine.

The venv lives in `backend/.venv`; `./start` reuses the primary checkout's
rather than giving every worktree its own half-gig copy of rembg.
