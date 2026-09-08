import { api, ApiError } from './api'
import type { Draft, GarmentRow, RankedPair, SavedOutfit } from './types'
import { isClean } from './types'

export type Screen =
  | 'home' | 'occasion' | 'aesthetic' | 'confirm' | 'dressing' | 'look'
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
  lookIndex = $state(0)

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
  }

  async generate() {
    if (!this.occasion || !this.vibe) { this.go('occasion'); return }
    this.loading = true
    this.go('dressing')
    try {
      const res = await api.outfits(this.occasion, this.vibe, 18, 3)
      this.looks = res.ranked ?? []
      this.lookIndex = 0
      if (!this.looks.length) {
        // "nothing to wear" is an answer, not a failure. Say which shelf came
        // up empty — usually the occasion's formality floor excludes it all.
        const c = res.candidates
        const n = (v: number, w: string) => `${v} ${w}${v === 1 ? '' : 's'}`
        // go() clears `error`, so it has to be set *after* the navigation
        this.go('home')
        this.error =
          `Nothing works for ${this.occasion} yet — only ${n(c.tops, 'top')} ` +
          `and ${n(c.bottoms, 'bottom')} qualify. Add pieces or pick another occasion.`
        return
      }
      this.applyLook(0)
      this.go('look')
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
