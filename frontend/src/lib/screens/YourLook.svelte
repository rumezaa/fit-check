<script lang="ts">
  import { app } from '../state.svelte'
  import Wireframe from '../components/Wireframe.svelte'
  import MeshGrid from '../components/MeshGrid.svelte'
  import PixelIcon from '../components/PixelIcon.svelte'
</script>

<div class="dark"></div>
<MeshGrid />
<div class="fig">
  <Wireframe topColor={app.currentTop?.hex ?? 'var(--pink)'}
             bottomColor={app.currentBottom?.hex ?? 'var(--dark)'} />
</div>

<div class="left">
  <h1 class="px-12">THIS IS YOU</h1>
  <p class="px-9 vibe">{app.vibe.toUpperCase()}</p>
  <p class="px-9 occ">{app.occasion.toUpperCase()}</p>
  {#if app.looks.length > 1}
    <p class="px-8 count">LOOK {app.lookIndex + 1} OF {app.looks.length}</p>
  {/if}
</div>

<div class="right">
  <p class="px-8 k">TOP</p>
  <p class="v">{app.currentTop?.name ?? '—'}</p>
  <p class="px-8 k">BOTTOM</p>
  <p class="v">{app.currentBottom?.name ?? '—'}</p>
</div>

<button class="btn back px-10" onclick={() => app.go('home')}>&#9664; BACK TO MAIN</button>

<!-- the engine hands back three; this is what pages through them -->
<button class="shuffle" onclick={() => app.cycleLook()}
        disabled={app.looks.length < 2}
        title="Shuffle to the next recommendation">
  <PixelIcon name="refresh" size={44} />
  <span class="px-8">{app.lookIndex + 1} / {app.looks.length}</span>
</button>

<button class="btn save px-10" onclick={() => { app.saveCurrentLook(); app.wearCurrentLook() }}>
  &#9825; SAVE LOOK
</button>

<style>
  .dark { position: absolute; inset: 0; background: #050308; }
  .fig { position: absolute; left: 50%; top: 18px; transform: translateX(-50%); }
  .left  { position: absolute; left: 24px;  top: 40px; width: 190px; }
  .right { position: absolute; right: 10px; top: 56px; width: 190px; text-align: right; }
  h1 { margin: 0 0 14px; color: #f9d9ff; letter-spacing: 2px;
       text-shadow: 0 0 10px rgba(224,76,255,.85); }
  .vibe  { margin: 0 0 8px; color: var(--pink); }
  .occ   { margin: 0 0 12px; color: #9adcff; }
  .count { margin: 0; color: var(--lilac); }
  .k { margin: 0 0 4px; color: #b38df2; }
  .v { margin: 0 0 18px; font-size: 8px; line-height: 1.6; color: #fbe8ff;
       text-shadow: 0 0 8px rgba(224,76,255,.5); overflow-wrap: anywhere; }
  .btn {
    position: absolute; top: 392px; height: 64px; width: 200px;
    background: #14082180; border-color: var(--neon); color: #fbe0ff;
    box-shadow: 0 0 12px rgba(224,76,255,.7);
    display: grid; grid-auto-flow: column; align-items: center;
    justify-content: center; gap: 8px;
  }
  .btn:active:not(:disabled) { box-shadow: 0 0 4px rgba(224,76,255,.7); }
  /* on the dark stage, grey out by draining the glow rather than the alpha */
  .btn:disabled {
    background: #241b30 !important; border-color: #5b4a66;
    color: #7d7192 !important; box-shadow: none !important;
  }
  .back { left: 12px; }
  .shuffle {
    position: absolute; left: 50%; transform: translateX(-50%); top: 380px;
    width: 96px; height: 88px; display: grid; place-items: center; gap: 2px;
    background: #14082180; border: 2px solid var(--neon); color: #fbe0ff;
    box-shadow: 0 0 14px rgba(224, 76, 255, 0.75);
  }
  .shuffle:active:not(:disabled) { box-shadow: 0 0 4px rgba(224, 76, 255, 0.7); }
  .shuffle:disabled {
    background: #241b30 !important; border-color: #5b4a66;
    color: #7d7192 !important; box-shadow: none !important;
  }
  .save { right: 12px; background: #5a0f3c; color: #ffc0dd; }
</style>
