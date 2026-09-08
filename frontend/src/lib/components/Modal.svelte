<script lang="ts">
  import type { Snippet } from 'svelte'
  /* Game-style overlay: dim the screen behind, float one titled panel. */
  let { title, width = 478, children }:
    { title: string; width?: number; children: Snippet } = $props()
</script>

<div class="scrim"></div>
<div class="panel chrome" style="width:{width}px">
  <div class="head px-9">{title}</div>
  <div class="body">{@render children()}</div>
</div>

<style>
  .scrim { position: absolute; inset: 0; background: rgba(10, 6, 22, 0.72); }
  .panel {
    position: absolute; left: 50%; top: 151px; transform: translateX(-50%);
    background: var(--white);
  }
  .head {
    background: var(--blue); color: var(--white);
    padding: 8px 10px; border-bottom: 2px solid var(--black);
  }
  .body { padding: 2px; }
</style>
