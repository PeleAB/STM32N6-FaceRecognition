export interface IElectronAPI {
    serial: {
        list: () => Promise<Array<{ path: string, manufacturer?: string, pnpId?: string }>>;
        connect: (path: string, baudRate: number) => Promise<{ success: boolean; error?: string }>;
        disconnect: () => Promise<{ success: boolean; error?: string }>;
        send: (data: Uint8Array) => Promise<{ success: boolean; error?: string }>;
        onData: (callback: (payload: { type: 'in' | 'out', data: Uint8Array }) => void) => () => void;
    }
}

declare global {
    interface Window {
        electronAPI: IElectronAPI;
    }
}
