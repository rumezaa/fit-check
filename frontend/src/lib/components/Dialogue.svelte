<script lang="ts">
  import type { Snippet } from 'svelte'
  /* RPG-style helper: text on the left, framed portrait on the right. The
     portrait is boxed on purpose — the avatar art has an opaque background,
     so letting it float over the leopard would show a white rectangle.
     `actions` puts this screen's buttons inside the bar, under the line. */
  let { text, portrait = '/bg/avatar.png', actions }:
    { text: string; portrait?: string; actions?: Snippet } = $props()
</script>

<div class="dialogue chrome">
  <div class="content">
    <p class="px-9">{text}</p>
    {#if actions}
      <div class="actions">{@render actions()}</div>
    {/if}
  </div>
  <div class="portrait chrome chrome--inset" style="background-image:url({portrait})"></div>
</div>

<style>
  .dialogue {
    position: absolute; left: 24px; bottom: 40px;
    width: 752px; height: 100px;
    background: var(--white);
    display: flex; align-items: stretch;
  }
  .content {
    flex: 1; min-width: 0; display: flex; flex-direction: column;
    justify-content: center; gap: 10px; padding: 0 0 0 20px;
  }
  p { margin: 0; color: var(--ink); }
  .actions { display: flex; gap: 14px; }
  .portrait {
    width: 104px; height: 92px; margin: 4px 14px 4px 0; flex: none;
    background-color: var(--lilac);
    background-size: cover; background-position: center top;
  }
</style>
