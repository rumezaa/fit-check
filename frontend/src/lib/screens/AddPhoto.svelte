<script lang="ts">
  import { app } from '../state.svelte'
  import { api } from '../api'
  import Dialogue from '../components/Dialogue.svelte'
  import StripFooter from '../components/StripFooter.svelte'
  import PixelIcon from '../components/PixelIcon.svelte'

  /* Shared by both paths through the camera: adding a piece to the closet, and
     asking whether a piece in a shop is worth buying. The steps are identical
     up to the point where one saves and the other doesn't. */
  const tryOn = $derived(app.intent === 'try-on')
  const STEPS = $derived(['Photo', 'Details', tryOn ? 'Verdict' : 'Done'])
  let file: HTMLInputElement
  let busy = $state(false)

  /* The Pi has a camera, but a plain file input with `capture` works on the
     kiosk and on a laptop, and needs no permissions dance. */
  async function upload(e: Event) {
    const picked = (e.target as HTMLInputElement).files?.[0]
    if (!picked) return
    busy = true
    app.error = null
    try {
      app.draft = await api.ingest(picked, picked.name)
      app.go('add-details')
    } catch (err) {
      app.error = err instanceof Error ? err.message : String(err)
    } finally {
      busy = false
      if (file) file.value = ''   // let the same file be re-picked after a failure
    }
  }
</script>

<div class="bg-leopard"></div>

<button class="back px-9" onclick={() => app.go('home')}>&#9664; BACK</button>

<div class="viewfinder chrome chrome--inset">
  <div class="vhead px-8"><span>CAMERA</span><span>&#9679; LIVE</span></div>
  <div class="bracket tl"></div><div class="bracket tr"></div>
  <div class="bracket bl"></div><div class="bracket br"></div>
  <div class="ghost px-7">{tryOn ? 'BYE OR BUY?' : 'LAY IT FLAT'}</div>
  <p class="hint">fill the frame &middot; plain background</p>
</div>

<button class="shutter" onclick={() => file.click()} disabled={busy}
        aria-label="Take a picture">
  <PixelIcon name="camera" size={52} color="var(--white)" />
</button>
<p class="snap px-8">{busy ? 'UPLOADING…' : 'TAP TO SNAP'}</p>

<input bind:this={file} type="file" accept="image/*" capture="environment"
       onchange={upload} hidden />

<Dialogue text={tryOn
  ? "Show me the thing you're eyeing!! A photo off the shop page works too."
  : "Take a picture of the piece you want to add. Put it on a solid, well-lit background."} />
<StripFooter items={STEPS} active="Photo" />

<style>
  .back { position: absolute; left: 20px; top: 40px; width: 150px; height: 52px;
          color: var(--ink); }
  .viewfinder { position: absolute; left: 190px; top: 24px;
                width: 420px; height: 300px; background: var(--deep); }
  .vhead { display: flex; justify-content: space-between; height: 26px;
           padding: 8px 10px; background: var(--blue); color: var(--white); }
  .bracket { position: absolute; width: 48px; height: 48px; border: 5px solid var(--pink); }
  .tl { left: 16px; top: 42px;  border-right: 0; border-bottom: 0; }
  .tr { right: 16px; top: 42px; border-left: 0;  border-bottom: 0; }
  .bl { left: 16px; bottom: 44px;  border-right: 0; border-top: 0; }
  .br { right: 16px; bottom: 44px; border-left: 0;  border-top: 0; }
  .ghost {
    position: absolute; left: 50%; top: 84px; transform: translateX(-50%);
    width: 150px; height: 150px; background: var(--dark); color: var(--lilac);
    display: grid; place-items: center; text-align: center;
  }
  .hint { position: absolute; left: 0; right: 0; bottom: 8px; margin: 0;
          text-align: center; font-size: 7px; color: var(--lilac); }
  .shutter { position: absolute; left: 641px; top: 96px; width: 108px; height: 108px;
             background: var(--pink); border-width: 4px; display: grid; place-items: center; }
  .snap { position: absolute; left: 610px; top: 222px; width: 180px;
          text-align: center; color: var(--ink); }
</style>
