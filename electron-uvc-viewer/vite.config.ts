import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import electron from 'vite-plugin-electron'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    react(),
    electron([
      {
        entry: 'electron/main.ts',
        vite: {
          build: {
            rollupOptions: {
              external: ['serialport', 'stream', 'child_process', 'path', 'url', 'http'],
            },
          },
        },
      },
      {
        entry: 'electron/preload.ts',
      },
    ]),
  ],
})
