<script lang="ts">
  import { ICONS, toPath, type IconName } from '../icons'

  // currentColor by default so an icon greys out with its button
  let { name, size = 26, color = 'currentColor' }:
    { name: IconName; size?: number; color?: string } = $props()

  const grid = $derived(ICONS[name].length)
  const d = $derived(toPath(ICONS[name]))
</script>

<svg width={size} height={size} viewBox="0 0 {grid} {grid}"
     shape-rendering="crispEdges" aria-hidden="true">
  <path {d} fill={color} />
</svg>
