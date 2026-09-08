<script lang="ts">
  import { app } from '../state.svelte'
  import Modal from '../components/Modal.svelte'
  import Dialogue from '../components/Dialogue.svelte'
  import StripFooter from '../components/StripFooter.svelte'
  import { confirmLine, vibeLine } from '../copy'

  /* Same panel twice: pick the vibe, then confirm and fire the engine. */
  let { confirm = false }: { confirm?: boolean } = $props()
  const STEPS = ['Occasion', 'Aesthetic', 'Generate']
</script>

<div class="bg-hangers"></div>

<Modal title="AESTHETIC">
  <div class="grid">
    {#each app.vibes as name}
      <button class="chip px-8" class:on={name === app.vibe}
              onclick={() => { app.vibe = name }}>{name.toUpperCase()}</button>
    {/each}
  </div>
</Modal>

<div class="row">
  <button class="btn px-10" onclick={() => app.go(confirm ? 'aesthetic' : 'occasion')}>
    &#9664; BACK
  </button>
  {#if confirm}
    <button class="btn go px-10" onclick={() => app.generate()}>GENERATE</button>
  {:else}
    <button class="btn go px-10" onclick={() => app.go('confirm')}
            disabled={!app.vibe}>NEXT &#9654;</button>
  {/if}
</div>

<!-- reacts to the tap: prompt -> the chosen vibe's line -> confirmation -->
<Dialogue text={confirm
  ? confirmLine(app.occasion, app.vibe)
  : vibeLine(app.vibe, app.occasion)} />
<StripFooter items={STEPS} active={confirm ? 'Generate' : 'Aesthetic'} />

<style>
  .grid { display: grid; grid-template-columns: repeat(3, 1fr); }
  .chip {
    height: 48px; color: var(--ink); background: var(--face-lt);
    box-shadow: none; border-width: 1px; padding: 0 4px;
  }
  .chip.on { background: var(--pink); color: var(--ink); }
  .row {
    position: absolute; left: 50%; transform: translateX(-50%); top: 284px;
    display: flex; gap: 16px;
  }
  .btn { width: 180px; height: 52px; background: var(--face); color: var(--ink); }
  .btn.go { background: var(--blue); color: var(--white); }
</style>
