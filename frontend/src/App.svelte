<script lang="ts">
  import { onMount } from 'svelte'
  import { app } from './lib/state.svelte'

  import Home from './lib/screens/Home.svelte'
  import OccasionPicker from './lib/screens/OccasionPicker.svelte'
  import AestheticPicker from './lib/screens/AestheticPicker.svelte'
  import Dressing from './lib/screens/Dressing.svelte'
  import YourLook from './lib/screens/YourLook.svelte'
  import AddPhoto from './lib/screens/AddPhoto.svelte'
  import AddDetails from './lib/screens/AddDetails.svelte'
  import AddDone from './lib/screens/AddDone.svelte'
  import Closet from './lib/screens/Closet.svelte'
  import Saved from './lib/screens/Saved.svelte'

  /* The panel is 800x480. On anything else, scale the whole stage rather than
     reflowing — the layout is pixel-placed and a reflow would break it. */
  let scale = $state(1)
  function fit() {
    scale = Math.min(window.innerWidth / 800, window.innerHeight / 480)
  }

  onMount(() => {
    fit()
    window.addEventListener('resize', fit)
    app.load()
    return () => window.removeEventListener('resize', fit)
  })
</script>

<div class="kiosk">
  <div class="stage" style="transform: scale({scale})">
    {#if app.screen === 'home'}          <Home />
    {:else if app.screen === 'occasion'} <OccasionPicker />
    {:else if app.screen === 'aesthetic'}<AestheticPicker />
    {:else if app.screen === 'dressing'} <Dressing />
    {:else if app.screen === 'look'}     <YourLook />
    {:else if app.screen === 'add-photo'}  <AddPhoto />
    {:else if app.screen === 'add-details'}<AddDetails />
    {:else if app.screen === 'add-done'}   <AddDone />
    {:else if app.screen === 'closet'}   <Closet />
    {:else if app.screen === 'saved'}    <Saved />
    {/if}

    {#if app.error}
      <div class="banner err px-8" role="alert">
        {app.error}
        <button class="x px-8" onclick={() => (app.error = null)}>&#10005;</button>
      </div>
    {/if}

    {#if app.toast}
      <div class="banner ok px-8" role="status">{app.toast}</div>
    {/if}
  </div>
</div>

<style>
  .banner {
    position: absolute; left: 50%; transform: translateX(-50%); top: 8px;
    max-width: 720px; padding: 10px 14px; display: flex; align-items: center; gap: 12px;
    border: 2px solid var(--black); z-index: 50;
  }
  .err { background: var(--pink); color: var(--white); }
  .ok  { background: var(--blue); color: var(--white); }
  .x { background: none; border: none; box-shadow: none; color: inherit; }
</style>
