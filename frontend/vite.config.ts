import { svelte } from '@sveltejs/vite-plugin-svelte'
import { defineConfig } from 'vite'

// The kiosk talks to the FastAPI backend on :8000. Proxying in dev keeps the
// frontend origin-relative, so the same fetch paths work when the Pi serves
// the built bundle from behind the API.
//
// ./start overrides both ports so more than one branch can be up at a time; on
// its own `npm run dev` stays on the usual pair.
const apiPort = process.env.FIT_CHECK_API_PORT ?? '8000'
const webPort = Number(process.env.FIT_CHECK_WEB_PORT ?? 5173)
const api = { target: `http://127.0.0.1:${apiPort}`, changeOrigin: true }

export default defineConfig({
  plugins: [svelte()],
  server: {
    port: webPort,
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
