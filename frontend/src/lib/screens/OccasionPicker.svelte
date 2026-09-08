<script lang="ts">
  import { app } from '../state.svelte'
  import Modal from '../components/Modal.svelte'
  import Dialogue from '../components/Dialogue.svelte'
  import StripFooter from '../components/StripFooter.svelte'
  import { WELCOME, occasionLine } from '../copy'
  const STEPS = ['Occasion', 'Aesthetic', 'Generate']
</script>

<div class="bg-leopard"></div>

<Modal title="OCCASION">
  <div class="grid">
    {#each app.occasions as name}
      <button class="chip px-8" class:on={name === app.occasion}
              onclick={() => { app.occasion = name }}>{name.toUpperCase()}</button>
    {/each}
  </div>
</Modal>

<button class="next px-12" onclick={() => app.go('aesthetic')}
        disabled={!app.occasion}>NEXT &#9654;</button>

<Dialogue text={app.occasion ? occasionLine(app.occasion) : WELCOME} />
<StripFooter items={STEPS} active="Occasion" />

<style>
  .grid { display: grid; grid-template-columns: repeat(4, 1fr); }
  /* 7 occasions in a 4-wide grid leaves one gap; let the last one span it */
  .chip:last-child:nth-child(4n + 3) { grid-column: span 2; }
  .chip {
    height: 48px; color: var(--ink); background: var(--face-lt);
    box-shadow: none; border-width: 1px; padding: 0 4px;
  }
  .chip.on { background: var(--pink); color: var(--ink); }
  .next {
    position: absolute; left: 50%; transform: translateX(-50%); top: 284px;
    width: 180px; height: 52px; background: var(--blue); color: var(--white);
  }
</style>
