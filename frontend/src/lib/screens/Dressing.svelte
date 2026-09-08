<script lang="ts">
  import { onMount } from 'svelte'
  import { app } from '../state.svelte'
  import Wireframe from '../components/Wireframe.svelte'
  import MeshGrid from '../components/MeshGrid.svelte'

  /* Reached from DRESS ME (show me wearing this) and from GENERATE (pick for
     me). If nothing has been generated yet we run the engine on the way in. */
  onMount(() => {
    if (app.hasLooks) {
      const t = setTimeout(() => app.go('look'), 1500)
      return () => clearTimeout(t)
    }
    app.generate()
  })
</script>

<div class="dark"></div>
<MeshGrid />
<div class="glow"></div>
<div class="fig"><Wireframe /></div>
<p class="caption px-12">GENERATING OUTFIT . . .</p>

<style>
  .dark { position: absolute; inset: 0; background: #050308; }
  .glow {
    position: absolute; left: 180px; top: 196px; width: 440px; height: 30px;
    background: var(--neon); opacity: 0.75; filter: blur(26px);
  }
  .fig { position: absolute; left: 50%; top: 18px; transform: translateX(-50%); }
  .caption {
    position: absolute; left: 0; right: 0; bottom: 22px;
    text-align: center; color: #f2bfff; letter-spacing: 3px;
    text-shadow: 0 0 12px rgba(224, 76, 255, 0.9);
  }
</style>
