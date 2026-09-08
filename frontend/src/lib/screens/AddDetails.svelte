<script lang="ts">
  import { app } from '../state.svelte'
  import { api } from '../api'
  import Dialogue from '../components/Dialogue.svelte'
  import StripFooter from '../components/StripFooter.svelte'
  import { FABRICS, LENGTHS, PATTERNS, FORMALITIES } from '../types'
  import type { Category, Fabric, Length, Pattern, Formality } from '../types'

  const STEPS = ['Photo', 'Details', 'Done']
  const d = app.draft

  /* The ingest step *guesses* these from the photo, but a guess outside the
     engine's enum tables would be written straight to the DB and then poison
     every future /outfits call for the whole closet. Only accept known values. */
  const pick = <T,>(allowed: readonly T[], guess: unknown, fallback: T): T =>
    (allowed as readonly unknown[]).includes(guess) ? guess as T : fallback

  let name      = $state(d?.name ?? '')
  let category  = $state<Category>(pick(['Top', 'Bottom'] as const, d?.category, 'Top'))
  let formality = $state<Formality>(pick(FORMALITIES, d?.formality, 'Casual'))
  let fabric    = $state<Fabric>(pick(FABRICS, d?.fabric, 'Cotton'))
  let pattern   = $state<Pattern>(pick(PATTERNS, d?.pattern, 'Solid'))
  let length    = $state<Length>(pick(LENGTHS, d?.length, 'NA' as Length))
  let busy      = $state(false)

  /* Length is a hemline, so it only applies to bottoms. Tops must send NA —
     the engine rejects anything else. */
  const needsLength = $derived(category === 'Bottom')
  const canSave = $derived(name.trim().length > 0 && !busy &&
                           (!needsLength || length !== 'NA'))

  $effect(() => { if (!needsLength) length = 'NA' })

  async function save() {
    if (!app.draft || !canSave) return
    busy = true
    app.error = null
    try {
      await api.create(app.draft.draft_id, {
        name: name.trim(), category, fabric, pattern, formality,
        length: needsLength ? length : 'NA',
        color: (d?.color as any) ?? { l: 60, c: 20, h: 300 },
        hex: d?.hex ?? null,
      })
      await app.load()
      app.go('add-done')
    } catch (err) {
      app.error = err instanceof Error ? err.message : String(err)
    } finally { busy = false }
  }
</script>

<div class="bg-leopard"></div>

<div class="panel chrome">
  <div class="head px-9">GARMENT DETAILS</div>

  <div class="body">
    <div class="shot chrome chrome--inset">
      {#if app.draft?.preview}
        <img src={app.draft.preview} alt="cut-out preview" />
      {:else}
        <span class="px-6">NO PREVIEW</span>
      {/if}
    </div>

    <div class="form">
      <div class="row">
        <span class="label px-7">NAME</span>
        <input class="input chrome chrome--inset" bind:value={name}
               placeholder="plaid mini skirt" maxlength="34" />
      </div>

      <div class="row">
        <span class="label px-7">TYPE</span>
        <div class="chips">
          {#each ['Top', 'Bottom'] as c}
            <button class="chip px-7" class:on={category === c}
                    onclick={() => (category = c as Category)}>{c.toUpperCase()}</button>
          {/each}
        </div>
      </div>

      <div class="row">
        <span class="label px-7">FORMALITY</span>
        <div class="chips">
          {#each FORMALITIES as f}
            <button class="chip px-7" class:on={formality === f}
                    onclick={() => (formality = f)}>{f.toUpperCase()}</button>
          {/each}
        </div>
      </div>

      <div class="row">
        <span class="label px-7">FABRIC</span>
        <div class="chips">
          {#each FABRICS as f}
            <button class="chip px-7" class:on={fabric === f}
                    onclick={() => (fabric = f)}>{f.toUpperCase()}</button>
          {/each}
        </div>
      </div>

      <div class="row">
        <span class="label px-7">PATTERN</span>
        <div class="chips">
          {#each PATTERNS as p}
            <button class="chip px-7" class:on={pattern === p}
                    onclick={() => (pattern = p)}>{p.toUpperCase()}</button>
          {/each}
        </div>
      </div>

      <div class="row" class:hidden={!needsLength}>
        <span class="label px-7">LENGTH</span>
        <div class="chips">
          {#each LENGTHS as l}
            <button class="chip px-7" class:on={length === l}
                    onclick={() => (length = l)}>{l.toUpperCase()}</button>
          {/each}
        </div>
      </div>
    </div>
  </div>
</div>

<button class="btn back px-9" onclick={() => app.go('add-photo')}>&#9664; BACK</button>
<button class="btn go px-9" onclick={save} disabled={!canSave}>
  {busy ? 'SAVING...' : 'SAVE'}
</button>

<Dialogue text={needsLength
  ? "Cute!! What is it, what's it made of, and how long??"
  : "Cute!! What is it, and what's it made of??"} />
<StripFooter items={STEPS} active="Details" />

<style>
  .panel { position: absolute; left: 24px; top: 12px; width: 752px; height: 264px;
           background: var(--face-lt); }
  .head { background: var(--blue); color: var(--white); padding: 8px 12px; }
  .body { display: flex; gap: 12px; padding: 10px 12px; height: 230px; }

  .shot { width: 150px; flex: none; background: var(--cream);
          display: grid; place-items: center; color: var(--muted); }
  .shot img { max-width: 92%; max-height: 208px; object-fit: contain; }

  .form { flex: 1; display: flex; flex-direction: column; gap: 6px; min-width: 0; }
  .row { display: flex; align-items: center; gap: 8px; }
  /* kept in the DOM so the rows above never shift when type changes */
  .row.hidden { visibility: hidden; }
  .label { width: 70px; flex: none; color: var(--ink); }

  .input { flex: 1; height: 30px; min-width: 0; border: 2px solid var(--black);
           background: var(--white); color: var(--ink); font-size: 9px; padding: 0 8px; }

  .chips { display: flex; flex-wrap: wrap; gap: 4px; }
  .chip { padding: 6px 6px; background: var(--white); color: var(--ink);
          box-shadow: none; border-width: 1px; }
  .chip.on { background: var(--pink); color: var(--white); }

  .btn { position: absolute; top: 286px; height: 48px; }
  .back { left: 24px; width: 150px; color: var(--ink); }
  .go { right: 24px; width: 200px; background: var(--pink); color: var(--white); }
</style>
