import { useEffect, useRef, useState } from 'react';
import './index.css';
import {
  createSetLedMsg,
  createRequestTelemetryMsg,
  createParamReadMsg,
  createParamWriteMsg,
  parseParamReadResponse,
  parseParamWriteResponse,
  generatePacket,
  parsePacket,
  extractPackets,
  getMsgTypeName,
  getMsgSubtypeName,
  SysobjUartMsgType,
  SysobjUartManageSubtype,
  SysobjUartConfigSubtype,
} from './SysobjUart';

// ---------------------------------------------------------------------------
// Param definitions — must match app_config.h app_param_id_t enum order
// ---------------------------------------------------------------------------
const PARAM_DEFS = [
  { id: 0, name: 'Conf. Threshold', unit: '%',   min: 0,  max: 100, defaultVal: 50  },
  { id: 1, name: 'Brightness',      unit: '%',   min: 0,  max: 100, defaultVal: 80  },
  { id: 2, name: 'Target FPS',      unit: 'fps', min: 1,  max: 60,  defaultVal: 30  },
] as const;

type ParamId = typeof PARAM_DEFS[number]['id'];

interface ParamState {
  value: number;
  inputValue: string;
  wasDefault: boolean;
  status: '' | 'loading' | 'ok' | 'error' | 'out-of-range';
}

interface LogEntry {
  id: number;
  type: 'in' | 'out';
  timestamp: number;
  raw: string;
  parsed?: {
    type: string;
    subtype: string;
  };
  error?: string;
}

function App() {
  const videoRef = useRef<HTMLVideoElement>(null);
  const [devices, setDevices] = useState<MediaDeviceInfo[]>([]);
  const [selectedDeviceId, setSelectedDeviceId] = useState<string>('');
  const [stream, setStream] = useState<MediaStream | null>(null);
  const [ffmpegUrl, setFfmpegUrl] = useState<string>('');
  const [error, setError] = useState<string>('');

  // Serial Port State
  const [ports, setPorts] = useState<any[]>([]);
  const [selectedPort, setSelectedPort] = useState<string>('');
  const [isConnected, setIsConnected] = useState(false);
  const [ledStates, setLedStates] = useState<Record<number, boolean>>({ 1: false, 2: false, 3: false });
  const [logs, setLogs] = useState<LogEntry[]>([]);
  const logEndRef = useRef<HTMLDivElement>(null);
  const rxBufferRef = useRef<number[]>([]);
  const [telemetry, setTelemetry] = useState({ cpuLoad: 0, inferenceTime: 0, nbDetect: 0 });

  // Config param state — keyed by param ID
  const [params, setParams] = useState<Record<ParamId, ParamState>>(
    () => Object.fromEntries(
      PARAM_DEFS.map(d => [d.id, { value: d.defaultVal, inputValue: String(d.defaultVal), wasDefault: true, status: '' as const }])
    ) as Record<ParamId, ParamState>
  );

  // Discover video devices
  useEffect(() => {
    async function getDevices() {
      try {
        try {
          const s = await navigator.mediaDevices.getUserMedia({ video: true, audio: false });
          s.getTracks().forEach(t => t.stop());
        } catch (e) {
          console.warn('Initial getUserMedia failed', e);
        }

        const deviceList = await navigator.mediaDevices.enumerateDevices();
        const videoDevices = deviceList.filter(device => device.kind === 'videoinput');
        setDevices(videoDevices);

        if (videoDevices.length > 0) {
          const targetDevice = videoDevices.find(d => d.label.toLowerCase().includes('stm32 uvc'));
          setSelectedDeviceId(targetDevice ? targetDevice.deviceId : videoDevices[0].deviceId);
        }
      } catch (err: any) {
        console.error('Error fetching devices', err);
      }
    }
    getDevices();
  }, []);

  // Serial Port Refresh
  useEffect(() => {
    const refreshPorts = async () => {
      if (window.electronAPI?.serial) {
        const portList = await window.electronAPI.serial.list();
        setPorts(portList);
        if (portList.length > 0 && !selectedPort) {
          setSelectedPort(portList[0].path);
        }
      }
    };
    refreshPorts();
    const interval = setInterval(refreshPorts, 5000);
    return () => clearInterval(interval);
  }, [selectedPort]);

  // Update video stream
  useEffect(() => {
    if (!selectedDeviceId) return;
    let activeStream: MediaStream | null = null;
    async function startStream() {
      try {
        if (stream) {
          stream.getTracks().forEach(track => track.stop());
        }
        activeStream = await navigator.mediaDevices.getUserMedia({
          video: {
            deviceId: { exact: selectedDeviceId },
            width: { ideal: 1280 },
            height: { ideal: 720 },
            frameRate: { ideal: 10, max: 10 },
          },
          audio: false
        });
        setStream(activeStream);
        setFfmpegUrl('');
        if (videoRef.current) {
          videoRef.current.srcObject = activeStream;
        }
        setError('');
      } catch (err: any) {
        console.warn('WebRTC fallback to FFmpeg...', err);
        const targetDevice = devices.find(d => d.deviceId === selectedDeviceId);
        if (targetDevice?.label) {
          const cleanLabel = targetDevice.label.replace(/\s*\([^)]*\)$/, '').trim();
          const proxyUrl = `http://127.0.0.1:8089/stream?camera=${encodeURIComponent(cleanLabel)}&timestamp=${Date.now()}`;
          setFfmpegUrl(proxyUrl);
          setStream(null);
          setError('');
        } else {
          setError('Failed to start video stream.');
        }
      }
    }
    startStream();
    return () => {
      if (activeStream) activeStream.getTracks().forEach(track => track.stop());
      setFfmpegUrl('');
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [selectedDeviceId, devices]);

  const toggleConnection = async () => {
    if (isConnected) {
      await window.electronAPI.serial.disconnect();
      setIsConnected(false);
    } else if (selectedPort) {
      const res = await window.electronAPI.serial.connect(selectedPort, 115200);
      if (res.success) {
        setIsConnected(true);
      } else {
        alert('Failed to connect: ' + res.error);
      }
    }
  };

  const toggleLed = async (ledId: number) => {
    if (!isConnected) return;
    const newState = !ledStates[ledId];
    const msg = createSetLedMsg(ledId, newState ? 1 : 0);
    const packet = generatePacket(msg);
    const res = await window.electronAPI.serial.send(packet);
    if (res.success) {
      setLedStates(prev => ({ ...prev, [ledId]: newState }));
    }
  };

  const requestTelemetry = async () => {
    if (!isConnected) return;
    const msg = createRequestTelemetryMsg();
    const packet = generatePacket(msg);
    await window.electronAPI.serial.send(packet);
  };

  const readParam = async (paramId: ParamId) => {
    if (!isConnected) return;
    setParams(prev => ({ ...prev, [paramId]: { ...prev[paramId], status: 'loading' } }));
    const packet = generatePacket(createParamReadMsg(paramId));
    await window.electronAPI.serial.send(packet);
  };

  const writeParam = async (paramId: ParamId) => {
    if (!isConnected) return;
    const raw = params[paramId].inputValue;
    const v = parseInt(raw, 10);
    const def = PARAM_DEFS.find(d => d.id === paramId)!;
    if (isNaN(v) || v < def.min || v > def.max) {
      setParams(prev => ({ ...prev, [paramId]: { ...prev[paramId], status: 'out-of-range' } }));
      return;
    }
    setParams(prev => ({ ...prev, [paramId]: { ...prev[paramId], status: 'loading' } }));
    const packet = generatePacket(createParamWriteMsg(paramId, v));
    await window.electronAPI.serial.send(packet);
  };

  const readAllParams = async () => {
    for (const def of PARAM_DEFS) {
      await readParam(def.id as ParamId);
    }
  };

  // Log Subscription
  useEffect(() => {
    if (!window.electronAPI?.serial) return;

    const cleanup = window.electronAPI.serial.onData((payload) => {
      let newLogs: LogEntry[] = [];

      if (payload.type === 'out') {
        const parsed = parsePacket(payload.data);
        newLogs.push({
          id: Date.now() + Math.random(),
          type: 'out',
          timestamp: Date.now(),
          raw: parsed.raw,
          error: parsed.error,
          parsed: parsed.msg ? {
            type: getMsgTypeName(parsed.msg.msg_type),
            subtype: getMsgSubtypeName(parsed.msg.msg_type, parsed.msg.msg_subtype)
          } : undefined
        });
      } else {
        // IN packets: push to stream buffer and extract
        for (let i = 0; i < payload.data.length; i++) {
          rxBufferRef.current.push(payload.data[i]);
        }

        const extracted = extractPackets(rxBufferRef.current);
        extracted.forEach(parsed => {
          if (parsed.msg && parsed.msg.msg_type === SysobjUartMsgType.MANAGE && parsed.msg.msg_subtype === SysobjUartManageSubtype.TELEMETRY) {
            if (parsed.msg.data && parsed.msg.data.length >= 4) {
              const cpuLoad = parsed.msg.data[0];
              const inferenceTime = parsed.msg.data[1] | (parsed.msg.data[2] << 8);
              const nbDetect = parsed.msg.data[3];
              setTelemetry({ cpuLoad, inferenceTime, nbDetect });
            }
          }

          if (parsed.msg && parsed.msg.msg_type === SysobjUartMsgType.CONFIG) {
            if (parsed.msg.msg_subtype === SysobjUartConfigSubtype.PARAM_READ && parsed.msg.data) {
              const resp = parseParamReadResponse(parsed.msg.data);
              if (resp && PARAM_DEFS.some(d => d.id === resp.paramId)) {
                setParams(prev => ({
                  ...prev,
                  [resp.paramId]: {
                    value: resp.value,
                    inputValue: String(resp.value),
                    wasDefault: resp.wasDefault,
                    status: resp.status === 0 ? 'ok' : 'error',
                  },
                }));
              }
            } else if (parsed.msg.msg_subtype === SysobjUartConfigSubtype.PARAM_WRITE && parsed.msg.data) {
              const resp = parseParamWriteResponse(parsed.msg.data);
              if (resp && PARAM_DEFS.some(d => d.id === resp.paramId)) {
                setParams(prev => ({
                  ...prev,
                  [resp.paramId]: {
                    ...prev[resp.paramId as ParamId],
                    status: resp.status === 0 ? 'ok' : (resp.status === 3 ? 'out-of-range' : 'error'),
                  },
                }));
              }
            }
          }

          newLogs.push({
            id: Date.now() + Math.random(),
            type: 'in',
            timestamp: Date.now(),
            raw: parsed.raw,
            error: parsed.error,
            parsed: parsed.msg ? {
              type: getMsgTypeName(parsed.msg.msg_type),
              subtype: getMsgSubtypeName(parsed.msg.msg_type, parsed.msg.msg_subtype)
            } : undefined
          });
        });
      }

      if (newLogs.length > 0) {
        setLogs(prev => [...prev, ...newLogs].slice(-99));
      }
    });

    return cleanup;
  }, []);

  // Auto-scroll log
  useEffect(() => {
    logEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [logs]);

  return (
    <div className="app-container">
      <aside className="side-panel">
        <div className="panel-section">
          <h2>Connection</h2>
          <div className="control-group">
            <select
              className="select-input"
              value={selectedPort}
              onChange={e => setSelectedPort(e.target.value)}
              disabled={isConnected}
            >
              {ports.length === 0 && <option>No ports found</option>}
              {ports.map(p => (
                <option key={p.path} value={p.path}>{p.path} ({p.manufacturer || 'Unknown'})</option>
              ))}
            </select>
            <button
              className={`btn ${isConnected ? 'btn-danger' : 'btn-primary'}`}
              onClick={toggleConnection}
            >
              {isConnected ? 'Disconnect' : 'Connect'}
            </button>
          </div>
        </div>

        <div className="panel-section">
          <h2>LED Control</h2>
          <div className="led-grid">
            {[1, 2, 3].map(id => (
              <div
                key={id}
                className={`led-card ${ledStates[id] ? 'active' : ''}`}
                onClick={() => toggleLed(id)}
                style={{ cursor: isConnected ? 'pointer' : 'not-allowed', opacity: isConnected ? 1 : 0.5 }}
              >
                <div className="led-indicator"></div>
                <span className="led-label">LED {id}</span>
              </div>
            ))}
          </div>
        </div>

        <div className="panel-section">
          <h2>Telemetry</h2>
          <div className="telemetry-card">
            <div className="telemetry-item">
              <span className="telemetry-label">CPU Load</span>
              <span className="telemetry-value">{telemetry.cpuLoad}%</span>
            </div>
            <div className="telemetry-item">
              <span className="telemetry-label">Inference Time</span>
              <span className="telemetry-value">{telemetry.inferenceTime} ms</span>
            </div>
            <div className="telemetry-item">
              <span className="telemetry-label">Detected Persons</span>
              <span className="telemetry-value">{telemetry.nbDetect}</span>
            </div>
            <button
              className="btn btn-primary"
              onClick={requestTelemetry}
              disabled={!isConnected}
              style={{ marginTop: '0.5rem' }}
            >
              Refresh Telemetry
            </button>
          </div>
        </div>

        <div className="panel-section">
          <div className="section-header">
            <h2>Config Params</h2>
            <button
              className="btn-text"
              onClick={readAllParams}
              disabled={!isConnected}
            >
              Read All
            </button>
          </div>
          <div className="param-list">
            {PARAM_DEFS.map(def => {
              const p = params[def.id as ParamId];
              return (
                <div key={def.id} className="param-row">
                  <div className="param-header">
                    <span className="param-name">{def.name}</span>
                    {p.status === '' && p.wasDefault && (
                      <span className="param-badge default">default</span>
                    )}
                    {p.status === 'ok' && p.wasDefault && (
                      <span className="param-badge default">default</span>
                    )}
                    {p.status === 'ok' && !p.wasDefault && (
                      <span className="param-badge ok">saved</span>
                    )}
                    {p.status === 'loading' && (
                      <span className="param-badge loading">…</span>
                    )}
                    {p.status === 'error' && (
                      <span className="param-badge error">error</span>
                    )}
                    {p.status === 'out-of-range' && (
                      <span className="param-badge error">out of range</span>
                    )}
                  </div>
                  <div className="param-controls">
                    <input
                      className="param-input"
                      type="text"
                      inputMode="numeric"
                      value={p.inputValue}
                      onWheel={e => e.currentTarget.blur()}
                      onClick={e => (e.target as HTMLInputElement).select()}
                      onChange={e => {
                        const v = e.target.value;
                        setParams(prev => ({
                          ...prev,
                          [def.id]: { ...prev[def.id as ParamId], inputValue: v, status: '' },
                        }));
                      }}
                    />
                    <span className="param-unit">{def.unit}</span>
                    <button
                      className="btn btn-small btn-secondary"
                      onClick={() => readParam(def.id as ParamId)}
                      disabled={!isConnected}
                      title={`Read ${def.name} from device`}
                    >R</button>
                    <button
                      className="btn btn-small btn-primary"
                      onClick={() => writeParam(def.id as ParamId)}
                      disabled={!isConnected}
                      title={`Write ${def.name} to device`}
                    >W</button>
                  </div>
                </div>
              );
            })}
          </div>
        </div>

        <div className="panel-section">
          <h2>Camera Setup</h2>
          <div className="control-group">
            <select
              className="select-input"
              value={selectedDeviceId}
              onChange={(e) => setSelectedDeviceId(e.target.value)}
            >
              {devices.length === 0 && <option value="">No cameras found</option>}
              {devices.map((device) => (
                <option key={device.deviceId} value={device.deviceId}>
                  {device.label || `Camera ${device.deviceId.substring(0, 5)}`}
                </option>
              ))}
            </select>
          </div>
        </div>
      </aside>

      <main className="main-content">
        <header className="header">
          <h1>UVC Stream Viewer</h1>
          <div className={`status-badge`}>
            <div className={`status-indicator ${(stream || ffmpegUrl) ? 'active' : 'inactive'}`}></div>
            {(stream || ffmpegUrl) ? 'Live' : 'Offline'}
          </div>
        </header>

        <section className="video-container">
          {error && <div className="placeholder" style={{ color: 'var(--error)' }}><p>{error}</p></div>}

          <video
            ref={videoRef}
            className="video-element"
            autoPlay
            playsInline
            muted
            style={{ display: stream ? 'block' : 'none' }}
          />

          {ffmpegUrl && (
            <img
              src={ffmpegUrl}
              className="video-element"
              alt="Camera Stream Proxy"
            />
          )}

          {(!stream && !ffmpegUrl && !error) && (
            <div className="placeholder">
              <p>Waiting for camera stream...</p>
            </div>
          )}
        </section>

        <section className="log-panel">
          <div className="log-panel-header">
            <span className="log-panel-title">Message Log</span>
            <button className="btn-text" onClick={() => setLogs([])}>Clear</button>
          </div>
          <div className="log-container">
            {logs.length === 0 && <div className="log-placeholder">No messages yet...</div>}
            {logs.map(log => (
              <div key={log.id} className={`log-entry ${log.type}`}>
                <div className="log-meta">
                  <span className="log-time">{new Date(log.timestamp).toLocaleTimeString([], { hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit' })}</span>
                  <span className="log-direction">{log.type === 'in' ? '→ RECV' : '← SENT'}</span>
                </div>
                <div className="log-payload">
                  {log.parsed && (
                    <div className="log-message-info">
                      <strong>{log.parsed.type}</strong>: {log.parsed.subtype}
                    </div>
                  )}
                  <div className="log-raw">{log.raw}</div>
                  {log.error && <div className="log-error">{log.error}</div>}
                </div>
              </div>
            ))}
            <div ref={logEndRef} />
          </div>
        </section>
      </main>
    </div>
  );
}

export default App;
