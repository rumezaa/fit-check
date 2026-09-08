<script lang="ts">
  /* The scanning grid behind the mannequin. One repeating-gradient, no nodes. */
  let { width = 360, height = 440 }: { width?: number; height?: number } = $props()
</script>

<div class="grid" style="width:{width}px;height:{height}px"></div>

<style>
  .grid {
    position: absolute; left: 50%; top: 20px; transform: translateX(-50%);
    background-color: #0e0614;
    background-image:
      repeating-linear-gradient(to right,  rgba(224,76,255,.2) 0 1px, transparent 1px 24px),
      repeating-linear-gradient(to bottom, rgba(224,76,255,.2) 0 1px, transparent 1px 24px);
  }
</style>
