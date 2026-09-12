<script lang="ts">
  import { app } from '../state.svelte'
  import { cutoutUrl } from '../types'
  import StripFooter from '../components/StripFooter.svelte'
  import PixelIcon from '../components/PixelIcon.svelte'
</script>

<div class="bg-leopard"></div>

<div class="head chrome chrome--flat px-9">
  <span>MY OUTFITS</span>
  <button class="exit px-8" onclick={() => app.go('home')}>EXIT SAVED</button>
</div>

<div class="grid">
  {#each app.saved as look (look.id)}
    <div class="card chrome chrome--inset">
      <div class="pair">
        {#each [look.top, look.bottom] as piece, i}
          <div class="half" class:upper={i === 0}>
            {#if piece && cutoutUrl(piece)}
              <img src={cutoutUrl(piece)} alt={piece.name} />
            {:else}
              <div class="blank"></div>
            {/if}
          </div>
        {/each}
      </div>
      <p class="px-7 tag">{look.vibe.toUpperCase()} &middot; {look.occasion.toUpperCase()}</p>
      <button class="del" onclick={() => app.deleteSavedLook(look.id)}
              aria-label="Remove this outfit">
        <PixelIcon name="trash" size={14} />
      </button>
    </div>
  {:else}
    <p class="empty px-9">NO SAVED OUTFITS YET — TAP THE HEART ON THE HOME SCREEN</p>
  {/each}
</div>

<StripFooter items={app.occasions} active={app.occasion} />

<style>
  .head { position: absolute; left: 0; right: 0; top: 0; height: 30px;
          display: flex; align-items: center; gap: 16px; padding: 0 12px;
          background: var(--blue); color: var(--white); border: 0; }
  .exit { margin-left: auto; padding: 5px 10px; color: var(--ink); }
  .grid { position: absolute; left: 14px; right: 14px; top: 42px; bottom: 44px;
          display: grid; grid-template-columns: repeat(4, 1fr);
          grid-auto-rows: 330px; gap: 12px; overflow: auto; align-content: start; }
  /* a card is a top stacked on a bottom, so it wants to be tall: a row nearly
     fills the grid, and a fifth outfit scrolls rather than shrinking the rest */
  .card { position: relative; background: var(--cream); padding: 6px;
          display: flex; flex-direction: column; }
  /* top-right, clear of the occasion/vibe caption along the bottom */
  .del { position: absolute; right: 0; top: 0; width: 25px; height: 19px;
         display: grid; place-items: center; border-width: 1px; box-shadow: none; }
  /* takes whatever the caption leaves, so the card's height is the only knob */
  .pair { flex: 1; min-height: 0;
          display: grid; grid-template-rows: 1fr 1fr; gap: 4px; }
  /* flex, not grid: a grid row here is content-sized, so the image's
     max-height:100% has nothing definite to resolve against and gets dropped —
     the garment then overflows its half and `overflow: hidden` cuts it off */
  .half { display: flex; align-items: center; justify-content: center;
          min-height: 0; overflow: hidden; }
  .half img { max-width: 92%; max-height: 100%; object-fit: contain; }
  /* a top is a shorter garment than a pair of trousers, so filling both halves
     edge to edge reads top-heavy. Giving the upper one a little less height
     balances the pair; both stay centred, so the card stays symmetric. */
  .half.upper img { max-height: 86%; }
  .blank { width: 60%; height: 80%; background: var(--face); border: 2px solid var(--black); }
  .tag { margin: 6px 0 0; text-align: center; color: var(--ink); }
  .empty { grid-column: 1 / -1; color: var(--ink); text-align: center; padding-top: 60px; }
</style>
