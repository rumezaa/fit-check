<script lang="ts">
  import { app } from '../state.svelte'
  import Dialogue from '../components/Dialogue.svelte'
  import StripFooter from '../components/StripFooter.svelte'

  const STEPS = ['Photo', 'Details', 'Done']
  const latest = $derived(app.garments[app.garments.length - 1] ?? null)
</script>

<div class="bg-leopard"></div>

<div class="panel chrome">
  <div class="head px-9">&#10003;  ADDED TO YOUR CLOSET!</div>
  <div class="body">
    <div class="shot chrome chrome--inset">
      {#if latest?.cutout_path}
        <img src={`/cutouts/${latest.cutout_path.split('/').pop()}`} alt={latest.name} />
      {/if}
    </div>
    <div class="meta">
      <h2>{latest?.name ?? 'your piece'}</h2>
      <p class="px-7 count">ITEM {app.garments.length} IN YOUR CLOSET</p>
      {#each [['TYPE', latest?.category], ['FABRIC', latest?.fabric],
              ['SHAPE', latest?.silhouette], ['LENGTH', latest?.length]] as [k, v]}
        {#if v && v !== 'NA'}
          <div class="kv chrome chrome--flat">
            <span class="px-7 k">{k}</span><span class="v">{v}</span>
          </div>
        {/if}
      {/each}
    </div>
  </div>
</div>

<button class="btn add px-10" onclick={() => { app.draft = null; app.go('add-photo') }}>
  &#10010;  ADD ANOTHER
</button>
<button class="btn done px-9" onclick={() => { app.draft = null; app.go('home') }}>
  &#9664; BACK TO CLOSET
</button>

<Dialogue text="Love that for you!! Added to your closet!!" />
<StripFooter items={STEPS} active="Done" />

<style>
  .panel { position: absolute; left: 24px; top: 12px; width: 752px; height: 264px;
           background: var(--face-lt); }
  .head { background: var(--blue); color: var(--white); padding: 9px 12px; }
  .body { display: flex; gap: 20px; padding: 14px 16px; }
  .shot { width: 190px; height: 200px; flex: none; background: var(--cream);
          display: grid; place-items: center; }
  .shot img { max-width: 92%; max-height: 92%; object-fit: contain; }
  .meta { flex: 1; }
  h2 { margin: 4px 0 10px; font-size: 13px; line-height: 1.5; color: var(--ink);
       overflow-wrap: anywhere; }
  .count { margin: 0 0 12px; color: var(--muted); }
  .kv { display: flex; align-items: center; gap: 14px; height: 34px;
        margin-bottom: 6px; padding: 0 12px; background: var(--white);
        border: 2px solid var(--black); }
  .k { color: var(--blue); width: 90px; }
  .v { font-size: 8px; color: var(--ink); }
  .btn { position: absolute; top: 286px; height: 52px; }
  .add  { left: 24px;  width: 280px; background: var(--blue); color: var(--white); }
  .done { right: 24px; width: 240px; background: var(--pink); color: var(--white); }
</style>
