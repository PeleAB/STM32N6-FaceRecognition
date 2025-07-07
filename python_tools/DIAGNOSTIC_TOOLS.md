# Streaming Quality Diagnostic Tools

This directory contains tools to diagnose streaming quality issues and compare protocols.

## 🔧 Diagnostic Scripts

### 1. `diagnose_stream.py` - Raw Data Analysis
```bash
python diagnose_stream.py
```
**Purpose**: Analyze raw serial data to identify corruption at the embedded level
- Logs all SOF markers and protocol headers  
- Validates checksums and message structure
- Saves problematic frames for analysis
- **Use when**: Suspecting embedded-side data corruption

### 2. `test_simple_protocol.py` - Protocol Quality Test  
```bash
python test_simple_protocol.py
```
**Purpose**: Test both original and enhanced protocols with quality analysis
- Works with both `pc_stream.c` (original) and `enhanced_pc_stream.c` 
- Detailed quality metrics (blur, brightness, compression)
- Saves all frames with quality annotations
- **Use when**: Comparing quality between protocols

### 3. `compare_original.py` - Original vs Current
```bash
python compare_original.py  
```
**Purpose**: Direct comparison with original text-based protocol
- Tests original simple text headers ("JPG 320 240 15234\\n")
- **Use when**: Need to isolate protocol complexity issues

### 4. `test_original_protocol.py` - Original Protocol Only
```bash
python test_original_protocol.py
```
**Purpose**: Test only the original simple protocol
- **Requires**: Embedded side using `pc_stream.c` (not enhanced version)
- **Use when**: Verifying original quality as baseline

## 🔍 How to Diagnose Quality Issues

### Step 1: Test Current Protocol
```bash
python test_simple_protocol.py
```
Look for:
- Decode failure rates 
- Size mismatches
- Quality metrics (variance < 100 = blurry)
- Compression ratios

### Step 2: Check Raw Data Integrity  
```bash
python diagnose_stream.py
```
Look for:
- Invalid checksums (embedded corruption)
- SOF marker rates
- Parse errors

### Step 3: Compare with Original (if needed)
Switch embedded to use `pc_stream.c`:
```c
// In main.c, replace:
Enhanced_PC_STREAM_SendFrame(...)
// With:
PC_STREAM_SendFrame(...)
```
Then run:
```bash
python test_original_protocol.py
```

## 📊 Understanding Output

### Quality Metrics
- **Variance**: > 100 = sharp, < 50 = very blurry
- **Brightness**: 0-255 scale, 30-225 is good range  
- **Contrast**: > 20 is good
- **Compression**: Higher ratio = better compression

### Frame Types
- **JPG**: Regular detection frames (should be grayscale, downsampled)
- **ALN**: Alignment frames (should be full-color, full resolution)

### Common Issues
- **Size mismatches**: Header vs actual frame dimensions
- **Decode failures**: Corrupted JPEG data
- **Low variance**: Blurry/corrupted frames
- **Poor compression**: Inefficient encoding

## 🛠 Troubleshooting

### High decode failure rate:
1. Check baud rate (try 7372800)
2. Run `diagnose_stream.py` to check data integrity
3. Look for invalid checksums

### Blurry frames:  
1. Check JPEG quality setting in embedded code
2. Verify correct image processing pipeline
3. Compare ALN vs JPG frame quality

### Protocol issues:
1. Test with `test_original_protocol.py` as baseline
2. Check embedded protocol implementation
3. Verify message framing and headers

## 📁 Output Directories

Each tool creates its own output directory:
- `diagnostic_frames/` - Raw diagnostic data
- `simple_test_frames/` - Quality test frames  
- `original_test_frames/` - Original protocol frames
- `original_frames/` - Comparison frames

All saved frames include metadata in filenames for easy analysis.