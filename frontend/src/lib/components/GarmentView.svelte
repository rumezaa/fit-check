<script lang="ts">
  import type { GarmentRow } from '../types'
  import { cutoutUrl } from '../types'

  /* Shows the server's cut-out photo. Until one exists we draw a flat block in
     the garment's own colour so the rack never renders empty. */
  let { garment, height = 120 }:
    { garment: GarmentRow | null; height?: number } = $props()

  const src = $derived(garment ? cutoutUrl(garment) : null)
  const swatch = $derived(garment?.hex || 'var(--dark)')
</script>

{#if !garment}
  <div class="empty px-8" style="height:{height}px">NOTHING HERE YET</div>
{:else if src}
  <img {src} alt={garment.name} style="max-height:{height}px" />
{:else}
  <div class="block" style="height:{height * 0.7}px; background:{swatch}"></div>
{/if}

<style>
  img { display: block; margin: 0 auto; max-width: 90%; object-fit: contain;
        image-rendering: auto; }
  .block { width: 60%; margin: 0 auto; border: 2px solid var(--black); }
  .empty { display: grid; place-items: center; color: var(--muted); text-align: center; }
</style>
