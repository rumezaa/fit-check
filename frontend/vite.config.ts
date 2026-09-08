import { svelte } from '@sveltejs/vite-plugin-svelte'
import { defineConfig } from 'vite'

// The kiosk talks to the FastAPI backend on :8000. Proxying in dev keeps the
// frontend origin-relative, so the same fetch paths work when the Pi serves
// the built bundle from behind the API.
const api = { target: 'http://127.0.0.1:8000', changeOrigin: true }

export default defineConfig({
  plugins: [svelte()],
  server: {
    port: 5173,
    proxy: {
      '/garments': api,
      '/outfits': api,
      '/saved-outfits': api,
      '/options': api,
      '/cutouts': api,
      '/drafts': api,
    },
  },
})
