<script lang="ts">
  /* The footer is plain spaced words on a dark bar, never buttons — the
     current one is simply coloured. Matches Cher's category strip. */
  let { items, active = '', onpick }:
    { items: string[]; active?: string; onpick?: (v: string) => void } = $props()
</script>

<div class="strip">
  {#each items as item}
    {#if onpick}
      <button class="word px-8" class:on={item === active}
              onclick={() => onpick(item)}>{item.toUpperCase()}</button>
    {:else}
      <span class="word px-8" class:on={item === active}>{item.toUpperCase()}</span>
    {/if}
  {/each}
</div>

<style>
  .strip {
    position: absolute; left: 0; right: 0; bottom: 0; height: 36px;
    background: var(--deep);
    display: flex; align-items: center; justify-content: center; gap: 22px;
    border-top: 2px solid var(--black);
  }
  .word {
    color: var(--lilac);
    background: none; border: none; box-shadow: none; padding: 6px 2px;
    white-space: nowrap;
  }
  .word.on { color: var(--pink); }
</style>
