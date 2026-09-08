import { mount } from 'svelte'
import '@fontsource/press-start-2p'
import './app.css'
import App from './App.svelte'

const app = mount(App, {
  target: document.getElementById('app')!,
})

export default app
