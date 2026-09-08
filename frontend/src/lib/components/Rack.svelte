<script lang="ts">
  import type { GarmentRow } from '../types'
  import GarmentView from './GarmentView.svelte'
  import PixelIcon from './PixelIcon.svelte'

  /* One rack: a titled pane plus its transport row. The wand locks this piece
     so the generator styles the rest around it; the basket sends it to the wash. */
  let { label, garment, index, count, locked, onstep, onlock, onlaundry }: {
    label: string
    garment: GarmentRow | null
    index: number
    count: number
    locked: boolean
    onstep: (delta: number) => void
    onlock: () => void
    onlaundry: () => void
  } = $props()

  const pad = (n: number) => String(n).padStart(2, '0')
</script>

<div class="pane chrome chrome--inset">
  <div class="head px-9" class:locked>
    <span>{label}</span>
    <span>{locked ? 'LOCKED' : `${pad(count ? index + 1 : 0)} / ${pad(count)}`}</span>
  </div>
  <div class="stageish">
    <GarmentView {garment} height={120} />
  </div>
  {#if garment}<div class="name">{garment.name}</div>{/if}
</div>

<div class="transport">
  <button class="tbtn" class:on={locked} onclick={onlock}
          aria-pressed={locked} title="Style an outfit around this piece">
    <PixelIcon name="wand" size={26} />
    <span class="px-6">USE THIS</span>
  </button>
  <button class="tbtn" onclick={() => onstep(-1)} disabled={locked || count < 2}
          aria-label="Previous {label}"><span class="px-10">&#9664;</span></button>
  <button class="tbtn" onclick={() => onstep(1)} disabled={locked || count < 2}
          aria-label="Next {label}"><span class="px-10">&#9654;</span></button>
  <button class="tbtn" onclick={onlaundry} disabled={!garment}
          title="Send this piece to the laundry">
    <PixelIcon name="basket" size={26} />
    <span class="px-6">LAUNDRY</span>
  </button>
</div>

<style>
  .pane { position: absolute; left: 274px; width: 252px; height: 176px;
          background: var(--white); overflow: hidden; }
  .head {
    display: flex; justify-content: space-between; align-items: center;
    height: 26px; padding: 0 10px; background: var(--blue); color: var(--white);
  }
  .head.locked { background: var(--pink); }
  .stageish { height: 118px; display: grid; place-items: center; }
  .name { text-align: center; font-size: 7px; line-height: 1.6; color: var(--ink);
          padding: 0 6px; overflow-wrap: anywhere; }
  .transport { position: absolute; left: 274px; display: flex; }
  .tbtn {
    width: 63px; height: 48px; display: grid; place-items: center; gap: 1px;
    color: var(--ink); background: var(--face);
  }
  .tbtn.on { background: var(--pink); color: var(--white); }
</style>
