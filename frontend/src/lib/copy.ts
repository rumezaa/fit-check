import { FORMALITIES } from './types'
import type { DressCode, Formality, Relaxed } from './types'

/* The helper's dialogue. Occasion names come from engine/fixtures/occasions.json,
   but the mockups use a few others (gym, party, brunch…), so match on a
   normalised key and fall back to something that still reads naturally. */

const LINES: Record<string, string> = {
  'errands':             "Errands?? Okay — cute but comfy!!",
  'class':               "Class!! Let's look like you did the reading.",
  'picnic':              "A picnic!! Something soft and easy.",
  'day out in the city': "A day in the city!! Let's make it a whole look.",
  'coffee chat':         "Coffee chat — casual, but make it intentional.",
  'corporate':           "Corporate!! Time to look expensive.",
  'date night':          "Date night!! Ooh, let's make them stare.",
  'gym':                 "Okay — time to pump it up!!",
  'party':               "A party!! Let's turn some heads.",
  'interview':           "An interview!! Let's look unhireable-ly good.",
  'brunch':              "Brunch!! Day drinking, respectfully.",
  'school':              "School!! Effortless, obviously.",
}

const norm = (s: string) => s.trim().toLowerCase()

/** Shown once an occasion is chosen and we're asking for the vibe. */
export function occasionLine(occasion: string): string {
  if (!occasion) return "First — where are we going??"
  return LINES[norm(occasion)] ?? `${occasion}!! What are we going for??`
}

const VIBE_LINES: Record<string, string> = {
  'dark academia': "Dark academia?? Tweed and tragedy. Obsessed.",
  'fairycore':     "Fairycore!! Soft, floaty, a little feral.",
  'baddie':        "Baddie mode!! Okay, we are SERVING.",
  'coquette':      "Coquette!! Bows on bows on bows.",
  'y2k':           "Y2K!! Low rise and no regrets.",
  'grunge':        "Grunge!! Flannel and bad decisions.",
  'preppy':        "Preppy!! Plaid and knee socks, obviously.",
  'clean girl':    "Clean girl!! Slick, gold, effortless.",
  'old money':     "Old money!! Quiet luxury, loud confidence.",
  'streetwear':    "Streetwear!! Baggy everything.",
  'cottagecore':   "Cottagecore!! Florals and fresh bread.",
}

/** Shown on the vibe step before anything is picked. */
export const VIBE_PROMPT = "So... what vibe are we going for??"

/** Same prompt, but carrying the occasion through from the previous step. */
export function vibePrompt(occasion: string): string {
  return occasion ? `Cute!! ${occasion}! What's the vibe??` : VIBE_PROMPT
}

/** Shown the moment a vibe is chosen, so the picker reacts to the tap. */
export function vibeLine(vibe: string, occasion = ''): string {
  if (!vibe) return vibePrompt(occasion)
  const line = VIBE_LINES[norm(vibe)]
  if (line) return line
  return occasion
    ? `${vibe} for ${occasion.toLowerCase()}?? Great pick.`
    : `${vibe}?? Great pick.`
}

/** The very first prompt, before anything is chosen. */
export const WELCOME = "Welcome to the outfit generator!! First — where are we going??"

/* ---- when we bent the dress code ---- */

/** Reads the band back the way someone would say it out loud, or null when the
    occasion takes no position at all — there is nothing to quote then. */
function bandName(code: DressCode | null): string | null {
  if (!code) return null
  if (code.min === code.max) return code.min.toLowerCase()
  if (code.min === 'Casual' && code.max === 'Elegant') return null
  if (code.max === 'Elegant') return `${code.min.toLowerCase()} or dressier`
  return `${code.min.toLowerCase()} to ${code.max.toLowerCase()}`
}

/** Which side of the band a piece fell off, or null when we can't tell. A
    relaxed rail can be under the code (only casual pants, corporate morning)
    or over it (nothing but silk, running errands), and those read nothing
    alike — so we ask the piece rather than guess. */
type Side = 'under' | 'over'
function sideOf(f: Formality | null | undefined, code: DressCode | null): Side | null {
  if (!f || !code) return null
  const rank = FORMALITIES.indexOf(f)
  if (rank < 0) return null
  if (rank < FORMALITIES.indexOf(code.min)) return 'under'
  if (rank > FORMALITIES.indexOf(code.max)) return 'over'
  return null
}

/** What the look on the racks is wearing, so the note can name the direction. */
export interface WornFormality {
  top?: Formality | null
  bottom?: Formality | null
  anchor?: Formality | null
}

/** Why the look on screen is outside the dress code, or null when it isn't.

    The engine would rather hand back a stretched outfit than nothing at all —
    only casual pants in the closet still gets you dressed for corporate. That
    is a choice we made on the user's behalf, so it has to be said out loud
    rather than left for them to notice. */
export function stretchNote(
  relaxed: Relaxed | null,
  code: DressCode | null,
  occasion: string,
  worn: WornFormality = {},
): string | null {
  if (!relaxed?.any) return null

  // shoes are filtered but never paired, so a stretch there is nothing the
  // user can see on the racks — saying it would just be confusing
  const rails: string[] = []
  const sides: (Side | null)[] = []
  if (relaxed.tops)    { rails.push('tops');    sides.push(sideOf(worn.top, code)) }
  if (relaxed.bottoms) { rails.push('bottoms'); sides.push(sideOf(worn.bottom, code)) }

  const band = bandName(code)
  const where = occasion ? ` for ${occasion.toLowerCase()}` : ''
  const needs = occasion ? `${occasion.toLowerCase()} needs` : 'the day needs'
  const want = band ? ` — that's ${band}` : ''

  if (relaxed.anchor && !rails.length) {
    // they locked the piece themselves, so this is a heads up, not an apology
    switch (sideOf(worn.anchor, code)) {
      case 'under':
        return `That piece is under the dress code${where}${want}. Styling it up anyway!!`
      case 'over':
        return `That piece is dressier than ${needs}${want}. Styling around it anyway!!`
      default:
        return `That piece sits outside the dress code${where}${want}. Wearing it anyway!!`
    }
  }
  if (!rails.length) return null

  const list = rails.length === 2 ? 'tops or bottoms' : rails[0]
  const also = relaxed.anchor ? ' Your locked piece misses it too.' : ''
  // one direction for both rails or we say nothing about direction
  const side = sides.every((s) => s === sides[0]) ? sides[0] : null

  switch (side) {
    case 'under':
      return `Nothing in your ${list} is dressy enough${where}${want}. ` +
             `Going with what you own!!${also}`
    case 'over':
      return `Everything in your ${list} is dressier than ${needs}${want}. ` +
             `Wearing it anyway!!${also}`
    default:
      return `Nothing in your ${list} matches the dress code${where}${want}. ` +
             `Going with what you own!!${also}`
  }
}
