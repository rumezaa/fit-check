<script lang="ts">
  import { app } from '../state.svelte'
  import Modal from '../components/Modal.svelte'
  import Dialogue from '../components/Dialogue.svelte'
  import StripFooter from '../components/StripFooter.svelte'
  import { vibeLine } from '../copy'

  /* Picking a vibe is the last decision, so this screen also fires the engine:
     Generate replaces Next the moment something is selected. */
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

<Dialogue text={vibeLine(app.vibe, app.occasion)}>
  {#snippet actions()}
    <button class="dlg-btn" onclick={() => app.go('occasion')}>&#9664; Back</button>
    {#if app.vibe}
      <button class="dlg-btn dlg-btn--leopard" onclick={() => app.generate()}>
        Generate
      </button>
    {/if}
  {/snippet}
</Dialogue>

<StripFooter items={STEPS} active={app.vibe ? 'Generate' : 'Aesthetic'} />

<style>
  .grid { display: grid; grid-template-columns: repeat(3, 1fr); }
  .chip {
    height: 48px; color: var(--ink); background: var(--face-lt);
    box-shadow: none; border-width: 1px; padding: 0 4px;
  }
  .chip.on { background: var(--pink); color: var(--ink); }
</style>
