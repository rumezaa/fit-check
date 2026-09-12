import type {
  Draft, GarmentFields, GarmentRow, Options, OutfitResponse, SavedOutfit,
  TryOnResponse,
} from './types'

export class ApiError extends Error {
  constructor(public status: number, message: string) {
    super(message)
    this.name = 'ApiError'
  }
}

async function req<T>(path: string, init?: RequestInit): Promise<T> {
  let res: Response
  try {
    res = await fetch(path, init)
  } catch {
    // the kiosk and the API die together often enough that this needs its own message
    throw new ApiError(0, 'Cannot reach the wardrobe service')
  }
  if (!res.ok) {
    // FastAPI puts the reason in `detail`; fall back to the status line
    let detail = res.statusText
    try {
      const body = await res.json()
      if (typeof body?.detail === 'string') detail = body.detail
    } catch { /* non-JSON error body */ }
    throw new ApiError(res.status, detail)
  }
  if (res.status === 204) return undefined as T
  // A path missing from the dev proxy returns Vite's index.html with a 200,
  // and res.json() then throws a bare SyntaxError that says nothing useful.
  const body = await res.text()
  try {
    return JSON.parse(body) as T
  } catch {
    throw new ApiError(res.status, `${path} did not return JSON — is it proxied?`)
  }
}

/** How many looks a run hands back. Matches kPoolSize in the engine, so the set
    it samples the opening look from is exactly the set refresh can cycle. */
export const LOOKS_PER_RUN = 5

export const api = {
  options: () => req<Options>('/options'),

  garments: () => req<GarmentRow[]>('/garments'),

  garment: (id: number) => req<GarmentRow>(`/garments/${id}`),

  /** Upload a photo; the server segments it and holds the cutout as a draft. */
  ingest(photo: Blob, filename = 'capture.png') {
    const body = new FormData()
    body.append('photo', photo, filename)
    return req<Draft>('/garments/ingest', { method: 'POST', body })
  },

  /** Confirm a draft. Only now does the cutout reach disk. */
  create(draft_id: string, fields: GarmentFields) {
    return req<GarmentRow>('/garments', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ draft_id, ...fields }),
    })
  },

  markWorn(id: number, last_worn = new Date().toISOString().slice(0, 10)) {
    return req<{ id: number; last_worn: string }>(`/garments/${id}/last-worn`, {
      method: 'PATCH',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ last_worn }),
    })
  },

  /* Bye or buy: score a piece the user is thinking about buying against the
     closet without adding it. Nothing is written, so the same draft can still
     be confirmed through create() afterwards if the answer is buy. Leaving
     occasion and vibe out asks the general question. */
  tryOn(fields: GarmentFields, opts: {
    occasion?: string | null; vibe?: string | null; temp_c?: number
  } = {}) {
    const body: Record<string, unknown> = { ...fields, temp_c: opts.temp_c ?? 18 }
    if (opts.occasion) body.occasion = opts.occasion
    if (opts.vibe) body.vibe = opts.vibe
    return req<TryOnResponse>('/try-on', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    })
  },

  savedOutfits: () => req<SavedOutfit[]>('/saved-outfits'),

  saveOutfit(body: {
    top_id: number | null; bottom_id: number | null
    occasion: string; vibe: string
  }) {
    return req<{ id: number }>('/saved-outfits', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    })
  },

  deleteSavedOutfit(id: number) {
    return req<void>(`/saved-outfits/${id}`, { method: 'DELETE' })
  },

  /* One endpoint answers both questions. Name an occasion and a vibe to build
     an outfit; name an anchor_id to ask what goes with a piece you already
     picked — the engine reads a missing key as "no opinion", so they are only
     sent when set. */
  outfits(opts: {
    occasion?: string | null
    vibe?: string | null
    anchor_id?: number | null
    temp_c?: number
    limit?: number
    seed?: number
  }) {
    const body: Record<string, unknown> = {
      temp_c: opts.temp_c ?? 18,
      limit: opts.limit ?? LOOKS_PER_RUN,
    }
    if (opts.occasion) body.occasion = opts.occasion
    if (opts.vibe) body.vibe = opts.vibe
    if (opts.anchor_id != null) body.anchor_id = opts.anchor_id
    if (opts.seed != null) body.seed = opts.seed
    return req<OutfitResponse>('/outfits', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    })
  },
}

/** Removes the row and its cutout file. */
export function deleteGarment(id: number) {
  return req<void>(`/garments/${id}`, { method: 'DELETE' })
}
