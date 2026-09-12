import { api, ApiError } from './api'
import { stretchNote } from './copy'
import type {
  DressCode, Draft, Formality, GarmentRow, RankedPair, Relaxed, SavedOutfit,
} from './types'
import { isClean } from './types'

export type Screen =
  | 'home' | 'occasion' | 'aesthetic' | 'dressing' | 'look'
  | 'add-photo' | 'add-details' | 'add-done'
  | 'closet' | 'saved'

/** The rack the wand has locked: that piece stays, the other is styled around it. */
export type Locked = 'Top' | 'Bottom' | null

function shelf(rows: GarmentRow[], category: 'Top' | 'Bottom') {
  // in-the-wash items stay in the closet but never reach the racks
  return rows.filter((g) => g.category === category && isClean(g))
}

class AppState {
  screen = $state<Screen>('home')
  garments = $state<GarmentRow[]>([])
  occasions = $state<string[]>([])
  vibes = $state<string[]>([])

  occasion = $state<string>('')
  vibe = $state<string>('')

  topIndex = $state(0)
  bottomIndex = $state(0)
  locked = $state<Locked>(null)

  looks = $state<RankedPair[]>([])
  /** Set when the last run was anchored — the engine echoes the piece back. */
  anchor = $state<
    { id: number; name: string; category: string; formality: Formality } | null
  >(null)
  lookIndex = $state(0)

  /** Which parts of the last run had to bend the occasion's dress code, and
      the band they were bent away from. Both come off the engine's answer. */
  relaxed = $state<Relaxed | null>(null)
  dressCode = $state<DressCode | null>(null)
  /** How many pairs the last run scored, and how many of those the colour
      floor removed — together they say why a run came back empty. */
  pairsScored = $state(0)
  droppedOnColor = $state(0)

  saved = $state<SavedOutfit[]>([])
  draft = $state<Draft | null>(null)

  loading = $state(false)
  error = $state<string | null>(null)
  toast = $state<string | null>(null)

  tops = $derived(shelf(this.garments, 'Top'))
  bottoms = $derived(shelf(this.garments, 'Bottom'))
  currentTop = $derived(this.tops[this.topIndex] ?? null)
  currentBottom = $derived(this.bottoms[this.bottomIndex] ?? null)
  /** Only meaningful once a generate run has produced looks. */
  hasLooks = $derived(this.looks.length > 0)
  /** The shuffle button is only worth showing with somewhere to shuffle to. */
  canShuffle = $derived(this.looks.length > 1)
  /** The piece the wand is holding, if any — what we style around. */
  lockedPiece = $derived(
    this.locked === 'Top' ? this.currentTop
      : this.locked === 'Bottom' ? this.currentBottom : null)
  /** Null unless the look on the racks sits outside the dress code. It
      describes a look, so it stays quiet when the run came back with none.
      The formalities go in because the note has to name which way we bent. */
  stretch = $derived(this.looks.length
    ? stretchNote(this.relaxed, this.dressCode, this.occasion, {
        top: this.currentTop?.formality,
        bottom: this.currentBottom?.formality,
        anchor: this.anchor?.formality,
      })
    : null)

  go(screen: Screen) { this.screen = screen; this.error = null }

  flash(message: string) {
    this.toast = message
    setTimeout(() => { if (this.toast === message) this.toast = null }, 2600)
  }

  private fail(e: unknown) {
    this.error = e instanceof ApiError ? e.message : String(e)
  }

  async load() {
    this.loading = true
    try {
      const [garments, options, saved] = await Promise.all([
        api.garments(), api.options(), api.savedOutfits(),
      ])
      this.garments = garments
      this.saved = saved
      this.occasions = options.occasions
      this.vibes = options.vibes
      // deliberately no default pick — an occasion lit before the user has
      // touched anything reads as already-chosen
      this.clampIndices()
    } catch (e) { this.fail(e) } finally { this.loading = false }
  }

  /** Racks shrink when items go to the laundry, so indices can fall off the end. */
  clampIndices() {
    this.topIndex = Math.min(this.topIndex, Math.max(0, this.tops.length - 1))
    this.bottomIndex = Math.min(this.bottomIndex, Math.max(0, this.bottoms.length - 1))
  }

  step(category: 'Top' | 'Bottom', delta: number) {
    if (this.locked === category) return
    const list = category === 'Top' ? this.tops : this.bottoms
    if (!list.length) return
    const cur = category === 'Top' ? this.topIndex : this.bottomIndex
    const next = (cur + delta + list.length) % list.length
    if (category === 'Top') this.topIndex = next
    else this.bottomIndex = next
  }

  /** The wand: lock this piece, or release it if it was already locked. */
  toggleLock(category: 'Top' | 'Bottom') {
    this.locked = this.locked === category ? null : category
    if (this.locked) return
    this.anchor = null
    // the stretch note says "that piece" about the lock we just released. the
    // rails it also named are still true of the looks on screen, so only the
    // anchor clause goes
    const r = this.relaxed
    if (r?.anchor) {
      this.relaxed = { ...r, anchor: false, any: r.tops || r.bottoms || r.shoes }
    }
  }

  /** The whole outfit: needs an occasion and a vibe.

      A locked piece still anchors the run. The lock means "style around this"
      whichever button fires it, and a free pair would leave the racks showing
      an outfit the engine never scored — the locked rack refuses to move. */
  async generate() {
    if (!this.occasion || !this.vibe) { this.go('occasion'); return }
    await this.run({
      occasion: this.occasion,
      vibe: this.vibe,
      anchor_id: this.lockedPiece?.id,
    })
  }

  /** The wand: style around the locked piece. Occasion and vibe are optional
      here — the engine answers the open-ended "what goes with this?". */
  async styleAround() {
    const piece = this.lockedPiece
    if (!piece) { this.flash('Lock a piece first with USE THIS'); return }
    await this.run({
      anchor_id: piece.id,
      // whatever is already chosen still narrows it; neither is required
      occasion: this.occasion || undefined,
      vibe: this.vibe || undefined,
    })
  }

  private async run(opts: {
    occasion?: string; vibe?: string; anchor_id?: number
  }) {
    this.loading = true
    try {
      const res = await api.outfits(opts)
      this.looks = res.ranked ?? []
      // the engine bends the dress code rather than hand back nothing, so the
      // answer carries what it bent. keep it beside the looks it belongs to
      this.relaxed = res.candidates?.relaxed_formality ?? null
      this.dressCode = res.dress_code ?? null
      this.pairsScored = res.pairs_scored ?? 0
      this.droppedOnColor = res.dropped_on_color ?? 0
      // trust the engine's echo rather than our own lock — they agree, but this
      // is the side that actually decided
      this.anchor = res.anchor
        ? {
            id: res.anchor.id, name: res.anchor.name,
            category: res.anchor.category, formality: res.anchor.formality,
          }
        : null
      this.lookIndex = 0
      if (!this.looks.length) {
        // "nothing to wear" is an answer, not a failure. Say which shelf came
        // up empty — usually the occasion's formality floor excludes it all.
        const c = res.candidates
        const n = (v: number, w: string) => `${v} ${w}${v === 1 ? '' : 's'}`
        // go() clears `error`, so it has to be set *after* the navigation
        this.go('home')
        if (this.pairsScored > 0 && this.droppedOnColor === this.pairsScored) {
          // the pieces qualified, they just clash — saying "nothing qualifies"
          // here would send you off to add clothes you already own
          this.error = opts.anchor_id != null
            ? `Nothing in the closet goes with that piece colour-wise — ` +
              `${n(this.pairsScored, 'pairing')} tried, all of them clash.`
            : `Everything that fits ${this.occasion} clashes on colour — ` +
              `${n(this.pairsScored, 'pairing')} tried. Try another occasion.`
        } else {
          this.error = opts.anchor_id != null
            ? `Nothing in the closet pairs with that piece yet — ` +
              `${n(c.tops, 'top')} and ${n(c.bottoms, 'bottom')} to work with.`
            : `Nothing works for ${this.occasion} yet — only ${n(c.tops, 'top')} ` +
              `and ${n(c.bottoms, 'bottom')} qualify. Add pieces or pick another occasion.`
        }
        return
      }
      // the generated fit loads onto the racks; DRESS ME is what takes you
      // to the try-on screens from here.
      //
      // we open on the engine's pick, not on rank 1. it samples that from the
      // top of the list rather than always taking the best score, which is the
      // only reason the same closet and occasion dont hand back the same outfit
      // every time. the shortlist is guaranteed to contain it
      const pick = res.pick
      const start = pick ? this.looks.findIndex((l) => l.rank === pick.rank) : -1
      this.applyLook(start < 0 ? 0 : start)
      this.go('home')
      // one look is a real answer here now that the colour floor cuts pairs,
      // and pointing someone at shuffle when there is nowhere to shuffle to
      // reads as the kiosk being broken
      const count = this.looks.length
      this.flash(count === 1
        ? (opts.anchor_id != null
            ? 'One way to wear that piece — the rest clashed'
            : `One look for ${this.occasion || 'that'} — the rest clashed`)
        : opts.anchor_id != null
          ? `${count} ways to wear that piece — shuffle to see them`
          : `${count} looks ready — shuffle to see them`)
    } catch (e) {
      // a failed run has no dress code to report, and the last one's would be
      // describing looks that are no longer on screen
      this.relaxed = null
      this.dressCode = null
      this.go('home')
      this.fail(e)     // after go(), which resets error
    } finally { this.loading = false }
  }

  /** Point the racks at the pair the engine chose — both sides, always.

      Holding a rack back for the lock is what used to make the racks disagree
      with the look: every run that can see a lock now anchors on it, so the
      engine's pair already has the locked piece on that side. */
  applyLook(index: number) {
    const look = this.looks[index]
    if (!look) return
    this.lookIndex = index
    const top = this.tops.findIndex((g) => g.id === look.top.id)
    if (top >= 0) this.topIndex = top
    const bottom = this.bottoms.findIndex((g) => g.id === look.bottom.id)
    if (bottom >= 0) this.bottomIndex = bottom
  }

  /** The circular button: cycle through the three generated looks. */
  cycleLook() {
    if (!this.looks.length) { this.go('occasion'); return }
    this.applyLook((this.lookIndex + 1) % this.looks.length)
  }

  /** Persists the look to wardrobe.db so it survives a reload. */
  async saveCurrentLook() {
    const top = this.currentTop, bottom = this.currentBottom
    if (!top && !bottom) { this.flash('Pick something first!'); return }
    try {
      await api.saveOutfit({
        top_id: top?.id ?? null,
        bottom_id: bottom?.id ?? null,
        occasion: this.occasion || 'Any',
        vibe: this.vibe || 'Any',
      })
      this.saved = await api.savedOutfits()
      this.flash('Saved to your outfits!')
    } catch (e) { this.fail(e) }
  }

  async deleteSavedLook(id: number) {
    try {
      await api.deleteSavedOutfit(id)
      this.saved = await api.savedOutfits()
      this.flash('Removed from your outfits')
    } catch (e) { this.fail(e) }
  }

  async wearCurrentLook() {
    const ids = [this.currentTop?.id, this.currentBottom?.id].filter(
      (v): v is number => typeof v === 'number')
    try {
      await Promise.all(ids.map((id) => api.markWorn(id)))
      await this.load()
    } catch (e) { this.fail(e) }
  }
}

export const app = new AppState()
