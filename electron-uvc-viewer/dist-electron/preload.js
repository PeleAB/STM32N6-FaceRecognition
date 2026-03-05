import { contextBridge, ipcRenderer } from "electron";
contextBridge.exposeInMainWorld("electronAPI", { serial: {
	list: () => ipcRenderer.invoke("serial:list"),
	connect: (path, baudRate) => ipcRenderer.invoke("serial:connect", path, baudRate),
	disconnect: () => ipcRenderer.invoke("serial:disconnect"),
	send: (data) => ipcRenderer.invoke("serial:send", data),
	onData: (callback) => {
		const listener = (_event, payload) => callback(payload);
		ipcRenderer.on("serial:data", listener);
		return () => ipcRenderer.removeListener("serial:data", listener);
	}
} });
window.addEventListener("DOMContentLoaded", () => {});
