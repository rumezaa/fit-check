<script lang="ts">
  import { app } from '../state.svelte'
  import { deleteGarment } from '../api'
  import { cutoutUrl, isClean } from '../types'
  import type { GarmentRow } from '../types'
  import PixelIcon from '../components/PixelIcon.svelte'
  import StripFooter from '../components/StripFooter.svelte'

  let tab = $state<'Top' | 'Bottom'>('Top')
  let pendingDelete = $state<GarmentRow | null>(null)
  let busy = $state(false)

  const shown = $derived(app.garments.filter((g) => g.category === tab))

  async function confirmDelete() {
    if (!pendingDelete) return
    busy = true
    try {
      await deleteGarment(pendingDelete.id)
      app.flash(`Deleted ${pendingDelete.name}`)
      pendingDelete = null
      await app.load()
    } catch (e) {
      app.error = e instanceof Error ? e.message : String(e)
    } finally { busy = false }
  }
</script>

<div class="bg-leopard"></div>

<div class="head chrome chrome--flat px-9">
  <span>MY CLOSET</span>
  <span class="tabs">
    {#each ['Top', 'Bottom'] as t}
      <button class="tab px-8" class:on={tab === t}
              onclick={() => (tab = t as 'Top' | 'Bottom')}>
        {t.toUpperCase()}S
      </button>
    {/each}
  </span>
  <button class="exit px-8" onclick={() => app.go('home')}>EXIT WARDROBE</button>
</div>

<div class="grid">
  {#each shown as g (g.id)}
    <div class="card chrome chrome--inset" class:wash={!isClean(g)}>
      <div class="shot">
        {#if cutoutUrl(g)}<img src={cutoutUrl(g)} alt={g.name} />{/if}
      </div>
      <p class="cname">{g.name}</p>
      <button class="corner del" onclick={() => (pendingDelete = g)}
              aria-label="Delete {g.name}">
        <PixelIcon name="trash" size={14} />
      </button>
      <button class="corner wash-btn" aria-label="Laundry">
        <PixelIcon name="basket" size={14} />
      </button>
    </div>
  {:else}
    <p class="empty px-9">NOTHING IN HERE YET — TAP ADD ON THE HOME SCREEN</p>
  {/each}
</div>

{#if pendingDelete}
  <div class="scrim"></div>
  <div class="confirm chrome">
    <p class="px-9">are you sure you want to delete {pendingDelete.name}?</p>
    <div class="crow">
      <button class="btn yes px-9" onclick={confirmDelete} disabled={busy}>
        {busy ? 'DELETING…' : 'Yes, Delete'}
      </button>
      <button class="btn px-9" onclick={() => (pendingDelete = null)}>Take me back</button>
    </div>
  </div>
{/if}

<StripFooter items={app.occasions} active={app.occasion} />

<style>
  .head { position: absolute; left: 0; right: 0; top: 0; height: 30px;
          display: flex; align-items: center; gap: 16px; padding: 0 12px;
          background: var(--blue); color: var(--white); border: 0; }
  .tabs { display: flex; gap: 6px; }
  .tab { padding: 4px 10px; background: var(--deep); color: var(--lilac);
         box-shadow: none; border-width: 1px; }
  .tab.on { background: var(--pink); color: var(--white); }
  .exit { margin-left: auto; padding: 5px 10px; color: var(--ink); }
  .grid { position: absolute; left: 14px; right: 14px; top: 42px; bottom: 44px;
          display: grid; grid-template-columns: repeat(6, 1fr);
          grid-auto-rows: 121px; gap: 10px; overflow: auto; align-content: start; }
  .card { position: relative; background: var(--cream); overflow: hidden; }
  .card.wash { filter: grayscale(0.85); background: var(--disabled); }
  .shot { height: 88px; display: grid; place-items: center; }
  .shot img { max-width: 90%; max-height: 84px; object-fit: contain; }
  .cname { margin: 0; text-align: center; font-size: 6px; line-height: 1.6;
           color: var(--ink); padding: 0 3px; overflow-wrap: anywhere; }
  .corner { position: absolute; bottom: 0; width: 25px; height: 19px;
            display: grid; place-items: center; border-width: 1px; box-shadow: none; }
  .del { left: 0; }
  .wash-btn { right: 0; }
  .empty { grid-column: 1 / -1; color: var(--ink); text-align: center; padding-top: 60px; }
  .scrim { position: absolute; inset: 0; background: rgba(10, 6, 22, 0.72); }
  .confirm { position: absolute; left: 50%; top: 150px; transform: translateX(-50%);
             width: 520px; padding: 22px; background: var(--white); }
  .confirm p { margin: 0 0 18px; color: var(--ink); }
  .crow { display: flex; gap: 14px; }
  .btn { flex: 1; height: 52px; color: var(--ink); }
  .yes { background: var(--pink); color: var(--white); }
</style>
