<script lang="ts">
  /* RPG-style helper: text on the left, framed portrait on the right. The
     portrait is boxed on purpose — the avatar art has an opaque background,
     so letting it float over the leopard would show a white rectangle. */
  let { text, portrait = '/bg/avatar.png' }:
    { text: string; portrait?: string } = $props()
</script>

<div class="dialogue chrome">
  <p class="px-9">{text}</p>
  <div class="portrait chrome chrome--inset" style="background-image:url({portrait})"></div>
</div>

<style>
  .dialogue {
    position: absolute; left: 24px; bottom: 40px;
    width: 752px; height: 100px;
    background: var(--white);
    display: flex; align-items: center;
  }
  p { margin: 0 0 0 20px; color: var(--ink); flex: 1; }
  .portrait {
    width: 104px; height: 92px; margin-right: 14px; flex: none;
    background-color: var(--lilac);
    background-size: cover; background-position: center top;
  }
</style>
