<script lang="ts">
  import { app } from '../state.svelte'
  import Rack from '../components/Rack.svelte'
  import PixelIcon from '../components/PixelIcon.svelte'
  import StripFooter from '../components/StripFooter.svelte'
</script>

<div class="bg-leopard"></div>

<button class="corner tl" onclick={() => app.go('closet')}>
  <PixelIcon name="basket" size={40} />
  <span class="px-7">CLOSET</span>
</button>

<button class="corner add" onclick={() => app.go('add-photo')}>
  <PixelIcon name="camera" size={46} />
  <span class="px-7">ADD</span>
</button>

<button class="corner tr" onclick={() => app.go('saved')}>
  <PixelIcon name="heart" size={34} color="var(--pink)" />
  <span class="px-7">SAVED</span>
</button>

<div style="top:0">
  <Rack label="TOPS" garment={app.currentTop} index={app.topIndex}
        count={app.tops.length} locked={app.locked === 'Top'}
        onstep={(d) => app.step('Top', d)}
        onlock={() => app.toggleLock('Top')}
        onlaundry={() => app.flash('Laundry needs a backend route')} />
</div>

<div class="rack-b">
  <Rack label="BOTTOMS" garment={app.currentBottom} index={app.bottomIndex}
        count={app.bottoms.length} locked={app.locked === 'Bottom'}
        onstep={(d) => app.step('Bottom', d)}
        onlock={() => app.toggleLock('Bottom')}
        onlaundry={() => app.flash('Laundry needs a backend route')} />
</div>

<button class="dress px-17" onclick={() => app.go('dressing')}>DRESS ME!</button>

<button class="save" onclick={() => app.saveCurrentLook()} aria-label="Save this outfit">
  <PixelIcon name="heart" size={38} color="var(--white)" />
  <span class="px-7">SAVE</span>
</button>

{#if app.hasLooks}
  <!-- only once the engine has given us looks to page through -->
  <div class="dots" aria-hidden="true">
    {#each app.looks as _, i}
      <span class="dot" class:on={i === app.lookIndex}></span>
    {/each}
  </div>

  <button class="cycle" onclick={() => app.cycleLook()} title="Show the next look">
    <PixelIcon name="refresh" size={56} color="var(--white)" />
  </button>
{:else}
  <button class="generate px-17" onclick={() => app.go('occasion')}>GENERATE</button>
{/if}

<StripFooter items={app.occasions} active={app.occasion}
             onpick={(v) => {
               app.occasion = v
               // changing the occasion re-runs the engine, but only once a
               // vibe exists — otherwise send them to pick one
               if (app.vibe) app.generate(); else app.go('aesthetic')
             }} />

<style>
  .corner {
    position: absolute; width: 92px; height: 92px;
    display: grid; place-items: center; gap: 2px; color: var(--ink);
  }
  .tl  { left: 0;   top: 0; }
  .add { left: 92px; top: 0; }
  .tr  { right: 0;  top: 0; }
  .rack-b :global(.pane)      { top: 224px; }
  .rack-b :global(.transport) { top: 396px; }
  :global(.pane)      { top: 0px; }
  :global(.transport) { top: 176px; }
  .dress {
    position: absolute; left: 54px; top: 376px; width: 220px; height: 68px;
    background: var(--pink); color: var(--white);
  }
  /* bottom edge sits exactly on GENERATE's top edge (284 + 92 = 376) so the
     two stack flush, the way they do in the mockup */
  .save {
    position: absolute; left: 526px; top: 284px; width: 92px; height: 92px;
    background: var(--pink); color: var(--white);
    display: grid; place-items: center; gap: 2px;
  }
  .cycle {
    position: absolute; left: 656px; top: 356px; width: 88px; height: 88px;
    background: var(--blue); display: grid; place-items: center;
  }
  .generate {
    position: absolute; left: 526px; top: 376px; width: 220px; height: 68px;
    background: var(--blue); color: var(--white);
  }
  .dots { position: absolute; left: 674px; top: 330px; display: flex; gap: 8px; }
  .dot { width: 12px; height: 12px; background: var(--face);
         border: 2px solid var(--black); }
  .dot.on { background: var(--pink); }
</style>
