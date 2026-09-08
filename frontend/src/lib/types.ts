/* Mirrors backend/db/store.py and engine/src/loader.cpp. These unions must stay
   in step with the engine's enum tables — it throws on anything else. */

export type Category   = 'Top' | 'Bottom' | 'Shoes'
export type Pattern    = 'Solid' | 'Floral' | 'PolkaDot' | 'Stripes' | 'Graphic'
export type Formality  = 'Casual' | 'Business' | 'Elegant'
export type Fabric     = 'Cotton' | 'Denim' | 'Knit' | 'Linen' | 'Wool'
                       | 'Silk' | 'Satin' | 'Lace' | 'Leather'
export type Silhouette = 'Fitted' | 'Straight' | 'Flowy' | 'Oversized'
/* tops have no hemline, so NA is the honest default */
export type Length     = 'NA' | 'Mini' | 'Midi' | 'Maxi'

export const FABRICS: Fabric[] = ['Cotton', 'Denim', 'Knit', 'Linen', 'Wool',
                                  'Silk', 'Satin', 'Lace', 'Leather']
export const PATTERNS: Pattern[] = ['Solid', 'Floral', 'PolkaDot', 'Stripes', 'Graphic']
export const FORMALITIES: Formality[] = ['Casual', 'Business', 'Elegant']
export const SILHOUETTES: Silhouette[] = ['Fitted', 'Straight', 'Flowy', 'Oversized']
export const LENGTHS: Exclude<Length, 'NA'>[] = ['Mini', 'Midi', 'Maxi']

export interface Color { l: number; c: number; h: number }

export interface GarmentFields {
  name: string
  category: Category
  color: Color
  hex?: string | null
  pattern: Pattern
  formality: Formality
  fabric: Fabric
  silhouette?: Silhouette
  length?: Length
  warmth?: number          // 1-5; the engine has no bands outside that
  clean?: boolean
  weight?: number
  last_worn?: string | null
}

/** A row as it comes back from the DB — colour is flattened into l/c/h. */
export interface GarmentRow {
  id: number
  name: string
  category: Category
  l: number; c: number; h: number
  hex: string | null
  pattern: Pattern
  formality: Formality
  fabric: Fabric | null
  silhouette: Silhouette | null
  length: Length | null
  warmth: number
  clean: number | boolean
  weight: number
  last_worn: string | null
  cutout_path: string
}

/** What /garments/ingest guesses from the photo before the user confirms it. */
export interface Draft extends Partial<GarmentFields> {
  draft_id: string
  preview: string
}

export interface EngineGarment {
  id: number
  name: string
  category: Category
  hex?: string
  fabric?: Fabric
  length?: Length
  [k: string]: unknown
}

/** The engine pairs a top with a bottom — shoes are filtered but not paired. */
export interface RankedPair {
  rank: number
  total: number
  top: EngineGarment
  bottom: EngineGarment
  scores: Record<string, number>
}

export interface OutfitResponse {
  ok: boolean
  error?: string
  occasion: string
  vibe: string
  weather: { temp_c: number; warmth_min: number; warmth_max: number }
  candidates: { tops: number; bottoms: number; shoes: number; complete: boolean }
  pairs_scored: number
  pick: RankedPair | null
  ranked: RankedPair[]
}

export interface Options { occasions: string[]; vibes: string[] }

/** A look kept by the user. The server inlines both garments. */
export interface SavedOutfit {
  id: number
  top_id: number | null
  bottom_id: number | null
  occasion: string
  vibe: string
  created_at: string
  top: GarmentRow | null
  bottom: GarmentRow | null
}

/** cutout_path is absolute on the server; /cutouts is mounted as static. */
export function cutoutUrl(row: { cutout_path?: string | null }): string | null {
  if (!row.cutout_path) return null
  const file = row.cutout_path.split('/').pop()
  return file ? `/cutouts/${file}` : null
}

export function isClean(row: GarmentRow): boolean {
  return row.clean === true || row.clean === 1
}
