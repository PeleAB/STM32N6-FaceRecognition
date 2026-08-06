export interface IElectronAPI {
    gallery: {
        createFromPhotos: () => Promise<{ success: boolean; canceled?: boolean;
            error?: string; embeddingQ7?: number[]; photos?: Array<{ photo: string; confidence: number }> }>;
    };
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
