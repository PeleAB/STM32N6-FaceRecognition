import { contextBridge, ipcRenderer } from 'electron';

contextBridge.exposeInMainWorld('electronAPI', {
    serial: {
        list: () => ipcRenderer.invoke('serial:list'),
        connect: (path: string, baudRate: number) => ipcRenderer.invoke('serial:connect', path, baudRate),
        disconnect: () => ipcRenderer.invoke('serial:disconnect'),
        send: (data: Uint8Array) => ipcRenderer.invoke('serial:send', data),
        onData: (callback: (payload: { type: 'in' | 'out', data: Uint8Array }) => void) => {
            const listener = (_event: any, payload: { type: 'in' | 'out', data: Uint8Array }) => callback(payload);
            ipcRenderer.on('serial:data', listener);
            return () => ipcRenderer.removeListener('serial:data', listener);
        }
    }
});

window.addEventListener('DOMContentLoaded', () => {
    // Simple preload script
});
