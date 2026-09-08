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

/** Shown the moment a vibe is chosen, so the picker reacts to the tap. */
export function vibeLine(vibe: string, occasion = ''): string {
  if (!vibe) return VIBE_PROMPT
  const line = VIBE_LINES[norm(vibe)]
  if (line) return line
  return occasion
    ? `${vibe} for ${occasion.toLowerCase()}?? Great pick.`
    : `${vibe}?? Great pick.`
}

/** Shown on the confirm step, once both are chosen. */
export function confirmLine(occasion: string, vibe: string): string {
  if (!occasion || !vibe) return "Pick a vibe and I'll put something together!!"
  return `Amazing!! Let's build you a ${vibe} look for ${occasion.toLowerCase()}.`
}

/** The very first prompt, before anything is chosen. */
export const WELCOME = "Welcome to the outfit generator!! First — where are we going??"
