/**
 * SysobjUart Protocol Logic for TypeScript
 * Matches the C implementation in sysobj_uart.c/h
 */

export const SysobjUartMsgType = {
    MANAGE: 0x00,
    CONFIG: 0x01,
    TEST: 0x02,
    CRITICAL: 0x03,
} as const;

export const SysobjUartManageSubtype = {
    SET_LED: 0x01,
    TELEMETRY: 0x02,
} as const;

export const SysobjUartConfigSubtype = {
    PARAM_READ: 0x01,
    PARAM_WRITE: 0x02,
    ENROLL: 0x06,
    COMMIT_ENROLL: 0x07,
    CLEAR_EMBEDDINGS: 0x08,
    GALLERY_LIST: 0x09,
    GALLERY_STATUS: 0x0A,
    GALLERY_DELETE: 0x0B,
    GALLERY_IMPORT_Q7: 0x0C,
} as const;

export interface SysobjUartMsg {
    src_id: number;
    dst_id: number;
    is_ack: number; // 4 bits
    need_ack: number; // 4 bits
    msg_type: number;
    msg_subtype: number;
    data?: Uint8Array;
}

const SYSOBJ_UART_SOF = 0xAA;
const SYSOBJ_UART_PAYLOAD_HEADER_SIZE = 5;

let crc32Table: Uint32Array | null = null;

function initCrc32Table() {
    const polynomial = 0xedb88320;
    crc32Table = new Uint32Array(256);
    for (let i = 0; i < 256; i++) {
        let crc = i;
        for (let j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >>> 1) ^ polynomial;
            } else {
                crc >>>= 1;
            }
        }
        crc32Table[i] = crc;
    }
}

export function calculateCrc32(data: Uint8Array): number {
    if (!crc32Table) {
        initCrc32Table();
    }
    let crc = 0xffffffff;
    for (let i = 0; i < data.length; i++) {
        const lookupIdx = (crc ^ data[i]) & 0xff;
        crc = (crc >>> 8) ^ crc32Table![lookupIdx];
    }
    return (crc ^ 0xffffffff) >>> 0;
}

export function generatePacket(msg: SysobjUartMsg): Uint8Array {
    const dataLen = msg.data ? msg.data.length : 0;
    const payloadSize = SYSOBJ_UART_PAYLOAD_HEADER_SIZE + dataLen;
    const totalLen = 3 + payloadSize + 4;

    const buffer = new Uint8Array(totalLen);

    // Wrapper Header
    buffer[0] = SYSOBJ_UART_SOF;
    buffer[1] = payloadSize & 0xff;
    buffer[2] = (buffer[0] + buffer[1]) & 0xff;

    // Payload Header
    const payload = buffer.subarray(3, 3 + payloadSize);
    payload[0] = msg.src_id & 0xff;
    payload[1] = msg.dst_id & 0xff;
    payload[2] = ((msg.is_ack & 0x0f) << 4) | (msg.need_ack & 0x0f);
    payload[3] = msg.msg_type & 0xff;
    payload[4] = msg.msg_subtype & 0xff;

    // Payload Data
    if (msg.data) {
        payload.set(msg.data, 5);
    }

    // CRC32
    const crc = calculateCrc32(payload);
    const crcOffset = 3 + payloadSize;
    buffer[crcOffset] = crc & 0xff;
    buffer[crcOffset + 1] = (crc >>> 8) & 0xff;
    buffer[crcOffset + 2] = (crc >>> 16) & 0xff;
    buffer[crcOffset + 3] = (crc >>> 24) & 0xff;

    return buffer;
}

export function createSetLedMsg(ledId: number, state: number): SysobjUartMsg {
    return {
        src_id: 0x01, // Usually PC/App
        dst_id: 0x02, // Usually MCU
        is_ack: 0,
        need_ack: 1,
        msg_type: SysobjUartMsgType.MANAGE,
        msg_subtype: SysobjUartManageSubtype.SET_LED,
        data: new Uint8Array([ledId, state]),
    };
}

export function createRequestTelemetryMsg(): SysobjUartMsg {
    return {
        src_id: 0x01,
        dst_id: 0x02,
        is_ack: 0,
        need_ack: 0,
        msg_type: SysobjUartMsgType.MANAGE,
        msg_subtype: SysobjUartManageSubtype.TELEMETRY,
    };
}

export function formatHex(data: Uint8Array): string {
    return Array.from(data)
        .map(b => b.toString(16).padStart(2, '0').toUpperCase())
        .join(' ');
}

export function parsePacket(data: Uint8Array): { msg?: SysobjUartMsg; raw: string; error?: string } {
    const raw = formatHex(data);

    if (data.length < 8) {
        return { raw, error: 'Packet too short' };
    }

    if (data[0] !== SYSOBJ_UART_SOF) {
        return { raw, error: 'Invalid SOF' };
    }

    const payloadSize = data[1];
    const checksum = data[2];
    if (((data[0] + data[1]) & 0xff) !== checksum) {
        return { raw, error: 'Invalid checksum' };
    }

    if (data.length < 3 + payloadSize + 4) {
        return { raw, error: 'Incomplete packet' };
    }

    const payload = data.subarray(3, 3 + payloadSize);
    const receivedCrc = (data[3 + payloadSize]) |
        (data[3 + payloadSize + 1] << 8) |
        (data[3 + payloadSize + 2] << 16) |
        (data[3 + payloadSize + 3] << 24);

    const calculatedCrc = calculateCrc32(payload);
    if ((calculatedCrc >>> 0) !== (receivedCrc >>> 0)) {
        return { raw, error: `CRC mismatch (calc: ${calculatedCrc.toString(16)}, recv: ${receivedCrc.toString(16)})` };
    }

    return {
        raw,
        msg: {
            src_id: payload[0],
            dst_id: payload[1],
            is_ack: (payload[2] >> 4) & 0x0f,
            need_ack: payload[2] & 0x0f,
            msg_type: payload[3],
            msg_subtype: payload[4],
            data: payload.length > 5 ? payload.slice(5) : undefined
        }
    };
}

export function extractPackets(buffer: number[]): { msg?: SysobjUartMsg; raw: string; error?: string }[] {
    const results: { msg?: SysobjUartMsg; raw: string; error?: string }[] = [];

    while (buffer.length > 0) {
        // Find next SOF
        if (buffer[0] !== 0xAA) {
            buffer.shift();
            continue;
        }

        // Minimum header size to determine length
        if (buffer.length < 3) {
            break; // Wait for more data
        }

        const payloadSize = buffer[1];
        const checksum = buffer[2];

        // Checksum mismatch -> false SOF, drop it and resync
        if (((0xAA + payloadSize) & 0xff) !== checksum) {
            buffer.shift();
            continue;
        }

        const totalLen = 3 + payloadSize + 4; // SOF + Size + CHK + Payload + CRC32

        // Check if we received enough bytes for the declared payload size
        if (buffer.length < totalLen) {
            break; // Wait for the rest of the packet
        }

        // We have a full packet, extract it
        const packetBytes = new Uint8Array(buffer.slice(0, totalLen));
        buffer.splice(0, totalLen); // Remove it from the incoming stream

        // Use the existing strict parser
        results.push(parsePacket(packetBytes));
    }

    return results;
}

// ---------------------------------------------------------------------------
// CONFIG message helpers
// ---------------------------------------------------------------------------

/**
 * Build a PARAM_READ request.
 * Payload: param_id[0..1] LE
 */
export function createParamReadMsg(paramId: number): SysobjUartMsg {
    const data = new Uint8Array(2);
    data[0] = paramId & 0xff;
    data[1] = (paramId >> 8) & 0xff;
    return {
        src_id: 0x01,
        dst_id: 0x02,
        is_ack: 0,
        need_ack: 0,
        msg_type: SysobjUartMsgType.CONFIG,
        msg_subtype: SysobjUartConfigSubtype.PARAM_READ,
        data,
    };
}

/**
 * Build a PARAM_WRITE request.
 * Payload: param_id[0..1] LE, type[2]=0 (U32), value[3..10] LE uint64
 */
export function createParamWriteMsg(paramId: number, value: number): SysobjUartMsg {
    const data = new Uint8Array(11);
    data[0] = paramId & 0xff;
    data[1] = (paramId >> 8) & 0xff;
    data[2] = 0; // PARAM_TYPE_U32
    // 32-bit value in LE, upper 4 bytes = 0
    data[3] = (value >>> 0) & 0xff;
    data[4] = (value >>> 8) & 0xff;
    data[5] = (value >>> 16) & 0xff;
    data[6] = (value >>> 24) & 0xff;
    // data[7..10] = 0 already
    return {
        src_id: 0x01,
        dst_id: 0x02,
        is_ack: 0,
        need_ack: 0,
        msg_type: SysobjUartMsgType.CONFIG,
        msg_subtype: SysobjUartConfigSubtype.PARAM_WRITE,
        data,
    };
}

function configRequest(subtype: number, data?: Uint8Array): SysobjUartMsg {
    return { src_id: 0x01, dst_id: 0x02, is_ack: 0, need_ack: 0,
        msg_type: SysobjUartMsgType.CONFIG, msg_subtype: subtype, data };
}

export function createEnrollMsg(name: string): SysobjUartMsg {
    return configRequest(SysobjUartConfigSubtype.ENROLL,
        new TextEncoder().encode(name).slice(0, 15));
}
export function createCommitEnrollMsg(): SysobjUartMsg {
    return configRequest(SysobjUartConfigSubtype.COMMIT_ENROLL);
}
export function createClearGalleryMsg(): SysobjUartMsg {
    return configRequest(SysobjUartConfigSubtype.CLEAR_EMBEDDINGS);
}
export function createGalleryListMsg(): SysobjUartMsg {
    return configRequest(SysobjUartConfigSubtype.GALLERY_LIST);
}
export function createGalleryStatusMsg(): SysobjUartMsg {
    return configRequest(SysobjUartConfigSubtype.GALLERY_STATUS);
}
export function createGalleryDeleteMsg(slot: number): SysobjUartMsg {
    return configRequest(SysobjUartConfigSubtype.GALLERY_DELETE,
        new Uint8Array([slot]));
}
export function createGalleryImportMsg(name: string, embeddingQ7: Int8Array): SysobjUartMsg {
    const encodedName = new TextEncoder().encode(name).slice(0, 15);
    if (embeddingQ7.length !== 128) throw new Error('Expected a 128-value embedding');
    const data = new Uint8Array(1 + encodedName.length + embeddingQ7.length);
    data[0] = encodedName.length;
    data.set(encodedName, 1);
    data.set(new Uint8Array(embeddingQ7.buffer, embeddingQ7.byteOffset,
        embeddingQ7.byteLength), 1 + encodedName.length);
    return configRequest(SysobjUartConfigSubtype.GALLERY_IMPORT_Q7, data);
}

export interface GalleryStatus {
    result: number;
    active: boolean;
    samples: number;
    required: number;
    count: number;
    name: string;
}
export interface GalleryEntry { slot: number; name: string }

export function parseGalleryStatus(data: Uint8Array): GalleryStatus | null {
    if (data.length < 6 || data.length < 6 + data[5]) return null;
    return { result: data[0], active: data[1] !== 0, samples: data[2],
        required: data[3], count: data[4],
        name: new TextDecoder().decode(data.slice(6, 6 + data[5])) };
}

export function parseGalleryList(data: Uint8Array): { result: number; entries: GalleryEntry[] } | null {
    if (data.length < 2) return null;
    const entries: GalleryEntry[] = [];
    let pos = 2;
    for (let i = 0; i < data[1]; i++) {
        if (pos + 2 > data.length) return null;
        const slot = data[pos++], len = data[pos++];
        if (pos + len > data.length) return null;
        entries.push({ slot, name: new TextDecoder().decode(data.slice(pos, pos + len)) });
        pos += len;
    }
    return { result: data[0], entries };
}

export interface ParamReadResponse {
    status: number;   // params_status_t (0 = OK)
    paramId: number;
    type: number;     // param_type_t (0 = U32)
    value: number;    // uint32 value
    wasDefault: boolean;
}

/**
 * Parse a CONFIG/PARAM_READ response payload (17 bytes):
 * [0]=status, [1..2]=param_id LE, [3]=type, [4..11]=value LE u64,
 * [12..15]=entry_crc32, [16]=was_default
 */
export function parseParamReadResponse(data: Uint8Array): ParamReadResponse | null {
    if (!data || data.length < 17) return null;
    const status = data[0];
    const paramId = data[1] | (data[2] << 8);
    const type = data[3];
    const value = ((data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24)) >>> 0);
    const wasDefault = data[16] !== 0;
    return { status, paramId, type, value, wasDefault };
}

export interface ParamWriteResponse {
    status: number;   // params_status_t (0 = OK)
    paramId: number;
}

/**
 * Parse a CONFIG/PARAM_WRITE response payload (3 bytes):
 * [0]=status, [1..2]=param_id LE
 */
export function parseParamWriteResponse(data: Uint8Array): ParamWriteResponse | null {
    if (!data || data.length < 3) return null;
    const status = data[0];
    const paramId = data[1] | (data[2] << 8);
    return { status, paramId };
}

export function getMsgTypeName(type: number): string {
    for (const [name, value] of Object.entries(SysobjUartMsgType)) {
        if (value === type) return name;
    }
    return `UNKNOWN(0x${type.toString(16).toUpperCase()})`;
}

export function getMsgSubtypeName(type: number, subtype: number): string {
    if (type === SysobjUartMsgType.MANAGE) {
        for (const [name, value] of Object.entries(SysobjUartManageSubtype)) {
            if (value === subtype) return name;
        }
    }
    if (type === SysobjUartMsgType.CONFIG) {
        for (const [name, value] of Object.entries(SysobjUartConfigSubtype)) {
            if (value === subtype) return name;
        }
    }
    return `0x${subtype.toString(16).toUpperCase()}`;
}
