<script lang="ts">
  /* The dressing-room mannequin. Body is always the neon wireframe; when a
     look is passed we lay flat garment shapes over the torso and hips. */
  let { topColor = null, bottomColor = null }:
    { topColor?: string | null; bottomColor?: string | null } = $props()
</script>

<svg class="figure" viewBox="0 0 180 380" fill="none" aria-hidden="true">
  <g stroke="var(--neon)" stroke-width="1.3" stroke-linejoin="round">
    <ellipse cx="90" cy="32" rx="18" ry="23" />
    <path d="M74 20 H106" /><path d="M72 32 H108" /><path d="M76 44 H104" />
    <path d="M90 9 V55" /><path d="M81 11 V52" /><path d="M99 11 V52" />
    <path d="M84 53 V68" /><path d="M96 53 V68" />
    <path d="M74 68 C62 72 58 88 60 106 C61 120 66 132 70 142 C60 152 56 176 60 200
             L120 200 C124 176 120 152 110 142 C114 132 119 120 120 106
             C122 88 118 72 106 68 Z" />
    <path d="M74 70 C56 80 44 110 44 142 C44 160 48 176 52 190" />
    <path d="M74 70 C62 84 54 112 54 142 C54 160 56 174 58 188" />
    <path d="M52 190 C47 199 55 205 61 198" />
    <path d="M106 70 C124 80 136 110 136 142 C136 160 132 176 128 190" />
    <path d="M106 70 C118 84 126 112 126 142 C126 160 124 174 122 188" />
    <path d="M128 190 C133 199 125 205 119 198" />
    <path d="M46 100 H62" /><path d="M44 120 H56" /><path d="M44 140 H55" /><path d="M47 162 H57" />
    <path d="M118 100 H134" /><path d="M124 120 H136" /><path d="M125 140 H136" /><path d="M123 162 H133" />
    <path d="M60 200 C56 236 58 268 62 300 C64 322 64 340 64 356" />
    <path d="M88 200 C86 236 84 268 82 300 C81 322 82 340 82 356" />
    <path d="M92 200 C94 236 96 268 98 300 C99 322 98 340 98 356" />
    <path d="M120 200 C124 236 122 268 118 300 C116 322 116 340 116 356" />
    <path d="M58 244 H85" /><path d="M59 266 H84" /><path d="M61 288 H83" />
    <path d="M63 310 H82" /><path d="M63 332 H82" />
    <path d="M95 244 H122" /><path d="M96 266 H121" /><path d="M97 288 H119" />
    <path d="M98 310 H117" /><path d="M98 332 H117" />
    <path d="M73 240 V355" /><path d="M107 240 V355" />
    <path d="M62 356 V367 H80 L82 356" /><path d="M116 356 V367 H98 L96 356" />
  </g>

  {#if topColor}
    <path d="M74 68 C62 72 58 88 60 106 C61 120 66 132 70 144 L110 144
             C114 132 119 120 120 106 C122 88 118 72 106 68 Z"
          fill={topColor} fill-opacity="0.6" stroke={topColor} stroke-width="1.6" />
  {/if}
  {#if bottomColor}
    <path d="M70 144 L110 144 C119 164 124 188 122 214 L58 214
             C56 188 61 164 70 144 Z"
          fill={bottomColor} fill-opacity="0.8" stroke={bottomColor} stroke-width="1.6" />
  {/if}
</svg>

<style>
  .figure {
    width: 190px; height: 400px;
    filter: drop-shadow(0 0 9px rgba(224, 76, 255, 0.9))
            drop-shadow(0 0 24px rgba(224, 76, 255, 0.45));
  }
</style>
