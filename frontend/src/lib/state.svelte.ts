import { api, ApiError } from './api'
import type {
  Draft, GarmentFields, GarmentRow, RankedPair, SavedOutfit, TryOnResponse,
} from './types'
import { isClean } from './types'

export type Screen =
  | 'home' | 'occasion' | 'aesthetic' | 'dressing' | 'look'
  | 'add-photo' | 'add-details' | 'add-done'
  | 'bye-or-buy'
  | 'closet' | 'saved'

/** The rack the wand has locked: that piece stays, the other is styled around it. */
export type Locked = 'Top' | 'Bottom' | null

/** Why we are photographing something. The photo and details steps are the
    same either way — only what happens at the end of them differs: `closet`
    adds the piece, `try-on` asks whether it is worth buying and adds nothing. */
export type Intent = 'closet' | 'try-on'

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
  anchor = $state<{ id: number; name: string; category: string } | null>(null)
  lookIndex = $state(0)

  saved = $state<SavedOutfit[]>([])
  draft = $state<Draft | null>(null)
  intent = $state<Intent>('closet')

  /* Bye or buy. The candidate is held here rather than in the closet — it is
     not something the user owns, and it only becomes a garment if they say
     they bought it. Keeping the fields lets the verdict be re-run against a
     different occasion without walking the form again. */
  candidate = $state<GarmentFields | null>(null)
  verdict = $state<TryOnResponse | null>(null)
  /** Which occasion the verdict was measured under; '' is the open question. */
  verdictOccasion = $state<string>('')

  loading = $state(false)
  error = $state<string | null>(null)
  toast = $state<string | null>(null)

  tops = $derived(shelf(this.garments, 'Top'))
  bottoms = $derived(shelf(this.garments, 'Bottom'))
  currentTop = $derived(this.tops[this.topIndex] ?? null)
  currentBottom = $derived(this.bottoms[this.bottomIndex] ?? null)
  /** Only meaningful once a generate run has produced looks. */
  hasLooks = $derived(this.looks.length > 0)

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
    if (!this.locked) this.anchor = null
  }

  /** The whole outfit: needs an occasion and a vibe. */
  async generate() {
    if (!this.occasion || !this.vibe) { this.go('occasion'); return }
    await this.run({ occasion: this.occasion, vibe: this.vibe })
  }

  /** The wand: style around the locked piece. Occasion and vibe are optional
      here — the engine answers the open-ended "what goes with this?". */
  async styleAround() {
    const piece = this.locked === 'Top' ? this.currentTop
                : this.locked === 'Bottom' ? this.currentBottom : null
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
      // trust the engine's echo rather than our own lock — they agree, but this
      // is the side that actually decided
      this.anchor = res.anchor
        ? { id: res.anchor.id, name: res.anchor.name, category: res.anchor.category }
        : null
      this.lookIndex = 0
      if (!this.looks.length) {
        // "nothing to wear" is an answer, not a failure. Say which shelf came
        // up empty — usually the occasion's formality floor excludes it all.
        const c = res.candidates
        const n = (v: number, w: string) => `${v} ${w}${v === 1 ? '' : 's'}`
        // go() clears `error`, so it has to be set *after* the navigation
        this.go('home')
        this.error = opts.anchor_id != null
          ? `Nothing in the closet pairs with that piece yet — ` +
            `${n(c.tops, 'top')} and ${n(c.bottoms, 'bottom')} to work with.`
          : `Nothing works for ${this.occasion} yet — only ${n(c.tops, 'top')} ` +
            `and ${n(c.bottoms, 'bottom')} qualify. Add pieces or pick another occasion.`
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
      this.flash(opts.anchor_id != null
        ? `${this.looks.length} ways to wear that piece — shuffle to see them`
        : `${this.looks.length} looks ready — shuffle to see them`)
    } catch (e) {
      this.go('home')
      this.fail(e)     // after go(), which resets error
    } finally { this.loading = false }
  }

  /** Point the racks at the pair the engine chose, honouring a locked piece. */
  applyLook(index: number) {
    const look = this.looks[index]
    if (!look) return
    this.lookIndex = index
    if (this.locked !== 'Top') {
      const i = this.tops.findIndex((g) => g.id === look.top.id)
      if (i >= 0) this.topIndex = i
    }
    if (this.locked !== 'Bottom') {
      const i = this.bottoms.findIndex((g) => g.id === look.bottom.id)
      if (i >= 0) this.bottomIndex = i
    }
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

  /* ---- bye or buy ------------------------------------------------------ */

  /** Start either path through the photo → details steps. */
  startAdd(intent: Intent) {
    this.intent = intent
    this.draft = null
    this.candidate = null
    this.verdict = null
    this.verdictOccasion = ''
    this.go('add-photo')
  }

  /** Ask the engine what the closet can do with a piece we don't own. */
  async checkCandidate(fields: GarmentFields) {
    this.candidate = fields
    await this.runVerdict()
  }

  /** Re-ask under one occasion — '' goes back to the open-ended question. */
  async recheck(occasion: string) {
    if (!this.candidate) return
    this.verdictOccasion = occasion
    await this.runVerdict()
  }

  private async runVerdict() {
    if (!this.candidate) return
    this.loading = true
    try {
      // no vibe on purpose: "does this go with my closet" is a fair question to
      // ask before "does it suit fairycore", and a vibe the user picked for some
      // other outfit has no business deciding what they buy
      this.verdict = await api.tryOn(this.candidate, {
        occasion: this.verdictOccasion || undefined,
      })
      this.go('bye-or-buy')
    } catch (e) {
      this.go('bye-or-buy')
      this.fail(e)     // after go(), which resets error
    } finally { this.loading = false }
  }

  /** They bought it. The draft is still held server-side, so this is the same
      confirm step the add-to-closet path ends on. */
  async keepCandidate() {
    if (!this.draft || !this.candidate) return
    this.loading = true
    try {
      await api.create(this.draft.draft_id, this.candidate)
      await this.load()
      this.intent = 'closet'
      this.go('add-done')
    } catch (e) { this.fail(e) } finally { this.loading = false }
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
