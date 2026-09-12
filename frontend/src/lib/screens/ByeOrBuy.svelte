<script lang="ts">
  import { app } from '../state.svelte'
  import { cutoutUrl } from '../types'
  import { VERDICT_HEAD, verdictLine, verdictReason } from '../copy'
  import Dialogue from '../components/Dialogue.svelte'
  import StripFooter from '../components/StripFooter.svelte'

  /* The answer to "should I buy this". The piece is never in the closet — it
     lives in app.candidate until the user says they bought it — so everything
     here comes off the one /try-on response plus the draft's cut-out. */
  const r = $derived(app.verdict)

  /* The strip re-asks the same question under one occasion. ANY is the default
     because "does this go with my closet" comes before "does it work Tuesday". */
  const ANY = 'Any'
  const strip = $derived([ANY, ...app.occasions])

  /* A bar rather than a number: works/tried is a ratio, and the closet's median
     pairing sits at roughly half by construction, so half-full literally means
     "pulls its weight". Standouts are the same cells lit brighter. */
  const CELLS = 20
  const lit = $derived(r && r.tried
    ? Math.round((r.works / r.tried) * CELLS) : 0)
  const bright = $derived(r && r.tried
    ? Math.round((r.standouts / r.tried) * CELLS) : 0)
  /* Hidden when there is nothing to measure against — an empty bar would read
     as "scored zero" when what happened is that we never scored it. */
  const showMeter = $derived(!!r && r.tried > 0 && r.closet_can_dress)

  /* The strip below is the same list whatever the answer was — the best of what
     the closet can do with it. Heading it "wear it with" after a bye would have
     the screen arguing with itself, so the heading follows the verdict. */
  const stripLabel = $derived(
    !r || !r.pairings.length ? ''
      : !r.closet_can_dress ? 'OUTFITS THIS WOULD UNLOCK'
      : r.works ? 'WEAR IT WITH'
      : "THE CLOSEST YOU'D GET")

  const TIER_LABEL: Record<string, string> = {
    standout: 'ONE OF YOUR BEST',
    works: 'WORKS',
    weak: 'NOT REALLY',
    unrated: 'NEW OUTFIT',
  }
</script>

<div class="bg-leopard"></div>

<div class="head chrome chrome--flat px-9">
  <span>BYE OR BUY?</span>
  <button class="exit px-8" onclick={() => app.go('home')}>EXIT</button>
</div>

<div class="shot chrome chrome--inset">
  {#if app.draft?.preview}
    <img src={app.draft.preview} alt={app.candidate?.name ?? 'the piece'} />
  {:else}
    <span class="px-6">NO PREVIEW</span>
  {/if}
  <p class="cname">{app.candidate?.name ?? '—'}</p>
</div>

{#if r}
  <div class="verdict chrome">
    <div class="call px-17 {r.verdict}">{VERDICT_HEAD[r.verdict]}</div>
    <div class="why">
      <p class="px-8 reason">{verdictReason(r)}</p>
      {#if showMeter}
        <div class="meter" aria-hidden="true">
          {#each { length: CELLS } as _, i}
            <span class="cell" class:on={i < lit} class:top={i < bright}></span>
          {/each}
        </div>
        <p class="px-6 scale">
          MEASURED AGAINST THE {r.baseline?.count} OUTFITS YOUR CLOSET ALREADY MAKES
          {r.occasion && r.occasion !== 'Anything' ? `· ${r.occasion.toUpperCase()}` : ''}
        </p>
      {/if}
    </div>
  </div>

  <p class="px-7 strip-label">{stripLabel}</p>
  <div class="pairs">
    {#each r.pairings.slice(0, 4) as p (p.garment.id)}
      <div class="card chrome chrome--inset">
        <div class="pshot">
          {#if cutoutUrl(p.garment)}
            <img src={cutoutUrl(p.garment)} alt={p.garment.name} />
          {/if}
        </div>
        <p class="pname">{p.garment.name}</p>
        <p class="px-6 tier {p.tier}">{TIER_LABEL[p.tier]}</p>
      </div>
    {:else}
      <p class="empty px-9">
        NOTHING IN YOUR CLOSET TO WEAR IT WITH — ADD SOME
        {r.pairs_with === 'Top' ? 'TOPS' : 'BOTTOMS'} FIRST
      </p>
    {/each}
  </div>
{:else}
  <p class="empty px-9 nores">NO VERDICT YET</p>
{/if}

<Dialogue text={app.loading
  ? 'Hold on!! Going through your whole closet...'
  : r ? verdictLine(r) : "Something went wrong — try that one again??"}>
  {#snippet actions()}
    <!-- offered whatever the verdict was: it is advice, not a lock, and the
         piece still has to become a real garment the ordinary way -->
    <button class="dlg-btn dlg-btn--leopard" onclick={() => app.keepCandidate()}
            disabled={app.loading || !app.draft || !app.candidate}>
      &#10010; I BOUGHT IT
    </button>
    <button class="dlg-btn" onclick={() => app.startAdd('try-on')}
            disabled={app.loading}>Check another &#9654;</button>
  {/snippet}
</Dialogue>

<StripFooter items={strip} active={app.verdictOccasion || ANY}
             onpick={(v) => app.recheck(v === ANY ? '' : v)} />

<style>
  .head { position: absolute; left: 0; right: 0; top: 0; height: 30px;
          display: flex; align-items: center; gap: 16px; padding: 0 12px;
          background: var(--blue); color: var(--white); border: 0; }
  .exit { margin-left: auto; padding: 5px 10px; color: var(--ink); }

  .shot { position: absolute; left: 14px; top: 40px; width: 180px; height: 292px;
          background: var(--cream); display: grid;
          grid-template-rows: 1fr auto; place-items: center; padding: 8px; }
  .shot img { max-width: 96%; max-height: 240px; object-fit: contain; }
  .cname { margin: 0; text-align: center; font-size: 7px; line-height: 1.6;
           color: var(--ink); overflow-wrap: anywhere; }

  .verdict { position: absolute; left: 206px; top: 40px; width: 580px; height: 108px;
             background: var(--white); display: flex; }
  .call { width: 170px; flex: none; display: grid; place-items: center;
          color: var(--white); border-right: 2px solid var(--black); }
  .call.buy   { background: var(--pink); }
  .call.maybe { background: var(--blue); }
  /* not the disabled grey — "bye" is a real answer, not a dead control */
  .call.bye   { background: var(--dark); }
  .why { flex: 1; min-width: 0; display: flex; flex-direction: column;
         justify-content: center; gap: 10px; padding: 0 14px; }
  .reason { margin: 0; color: var(--ink); line-height: 1.7; overflow-wrap: anywhere; }
  .scale { margin: 0; color: var(--muted); line-height: 1.6; }

  .meter { display: flex; gap: 2px; }
  .cell { width: 15px; height: 12px; background: var(--face-lt);
          border: 1px solid var(--black); }
  .cell.on  { background: var(--blue); }
  .cell.top { background: var(--pink); }

  .strip-label { position: absolute; left: 206px; top: 156px; margin: 0;
                 color: var(--ink); }
  .pairs { position: absolute; left: 206px; top: 172px; width: 580px; height: 160px;
           display: flex; gap: 12px; }
  .card { width: 136px; flex: none; background: var(--cream); padding: 6px;
          display: grid; grid-template-rows: 1fr auto auto; }
  .pshot { display: grid; place-items: center; overflow: hidden; }
  .pshot img { max-width: 92%; max-height: 100%; object-fit: contain; }
  .pname { margin: 4px 0 0; text-align: center; font-size: 6px; line-height: 1.6;
           color: var(--ink); overflow-wrap: anywhere; }
  .tier { margin: 4px 0 0; text-align: center; color: var(--white);
          padding: 3px 0; background: var(--muted); }
  .tier.standout { background: var(--pink); }
  .tier.works    { background: var(--blue); }
  .tier.unrated  { background: var(--dark); }

  .empty { color: var(--ink); align-self: center; padding: 0 20px; line-height: 1.8; }
  .nores { position: absolute; left: 206px; top: 140px; }
</style>
