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

<button class="corner add" onclick={() => app.startAdd('closet')}>
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
        onlock={() => {
          app.toggleLock('Top')
          // locking asks the engine what goes with it; unlocking just releases
          if (app.locked === 'Top') app.styleAround()
        }}
        onlaundry={() => app.flash('Laundry needs a backend route')} />
</div>

<div class="rack-b">
  <Rack label="BOTTOMS" garment={app.currentBottom} index={app.bottomIndex}
        count={app.bottoms.length} locked={app.locked === 'Bottom'}
        onstep={(d) => app.step('Bottom', d)}
        onlock={() => {
          app.toggleLock('Bottom')
          // locking asks the engine what goes with it; unlocking just releases
          if (app.locked === 'Bottom') app.styleAround()
        }}
        onlaundry={() => app.flash('Laundry needs a backend route')} />
</div>

<!-- the third verb on this screen: DRESS ME wears the closet, GENERATE fills
     it in, this one asks whether something outside it is worth owning -->
<button class="byebuy px-12" onclick={() => app.startAdd('try-on')}>
  BYE OR BUY?
</button>

{#if app.anchor}
  <div class="anchored chrome">
    <span class="px-7 k">STYLING AROUND</span>
    <span class="px-8 v">{app.anchor.name}</span>
  </div>
{/if}

<button class="dress px-17" onclick={() => app.go('dressing')}>DRESS ME!</button>

<button class="save" onclick={() => app.saveCurrentLook()} aria-label="Save this outfit">
  <PixelIcon name="heart" size={38} color="var(--white)" />
</button>

{#if app.hasLooks}
  <!-- appears beside the heart once the engine has looks to page through -->
  <button class="shuffle" onclick={() => app.cycleLook()}
          title="Shuffle to the next generated outfit ({app.lookIndex + 1} of {app.looks.length})">
    <PixelIcon name="refresh" size={52} color="var(--white)" />
  </button>
{/if}

<button class="generate px-17" onclick={() => app.go('occasion')}>GENERATE</button>

<!-- picking here only sets the occasion; GENERATE is what runs the engine,
     so a tap never regenerates or navigates behind your back -->
<StripFooter items={app.occasions} active={app.occasion}
             onpick={(v) => { app.occasion = v }} />

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
  .byebuy {
    position: absolute; left: 54px; top: 196px; width: 220px; height: 68px;
    background: var(--dark); color: var(--white);
  }
  .anchored {
    position: absolute; left: 54px; top: 286px; width: 220px; height: 72px;
    background: var(--pink); color: var(--white);
    display: grid; align-content: center; gap: 6px; padding: 0 12px;
  }
  .anchored .k { opacity: 0.85; }
  .anchored .v { overflow-wrap: anywhere; line-height: 1.6; }
  .dress {
    position: absolute; left: 54px; top: 376px; width: 220px; height: 68px;
    background: var(--pink); color: var(--white);
  }
  /* bottom edge sits exactly on GENERATE's top edge (284 + 92 = 376) so the
     two stack flush, the way they do in the mockup */
  /* heart and shuffle sit side by side, flush on top of GENERATE */
  .save {
    position: absolute; left: 526px; top: 284px; width: 92px; height: 92px;
    background: var(--pink); color: var(--white);
    display: grid; place-items: center;
  }
  .shuffle {
    position: absolute; left: 622px; top: 284px; width: 92px; height: 92px;
    background: var(--blue); display: grid; place-items: center;
  }
  .generate {
    position: absolute; left: 526px; top: 376px; width: 220px; height: 68px;
    background: var(--blue); color: var(--white);
  }
</style>
