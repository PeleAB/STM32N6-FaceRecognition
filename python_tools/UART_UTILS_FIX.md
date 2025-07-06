# UART Utils Infinite Loop Fix

## Problem Description

The UI was getting stuck forever in the `_search_header()` function in `pc_uart_utils.py`. This happened because:

### 🚫 **Root Cause**
```python
def _search_header(ser, prefixes):
    while True:  # ← INFINITE LOOP!
        raw = ser.readline()  # ← Blocks indefinitely if no data
        if not raw:
            return None
        # ... process line
```

**Issues:**
1. **No timeout protection** - `ser.readline()` blocks indefinitely waiting for data
2. **No maximum attempts limit** - Could loop forever if wrong data arrives  
3. **No escape mechanism** - Once stuck, UI becomes completely unresponsive
4. **Multiple infinite loops** - Several functions had similar issues

## ✅ **Solution Implemented**

### **1. Timeout-Protected Header Search**
```python
def _search_header_with_timeout(ser, prefixes, timeout=2.0, max_attempts=50):
    start_time = time.time()
    attempts = 0
    
    # Set short timeout for non-blocking reads
    original_timeout = ser.timeout
    ser.timeout = 0.1  # 100ms timeout
    
    try:
        while attempts < max_attempts and (time.time() - start_time) < timeout:
            attempts += 1
            raw = ser.readline()
            
            if not raw:
                time.sleep(0.01)  # Prevent busy waiting
                continue
            
            # Process and check for matching prefix
            line = raw.decode('ascii', errors='ignore').strip()
            if line.startswith(prefixes):
                return line
                
        return None  # Timeout reached
    finally:
        ser.timeout = original_timeout  # Restore original timeout
```

### **2. Key Improvements**

**Timeout Control:**
- ✅ **Maximum time limit**: 2 seconds default timeout
- ✅ **Maximum attempts**: 50 attempts to prevent endless loops  
- ✅ **Non-blocking reads**: 100ms timeout per readline
- ✅ **Graceful timeout**: Returns None instead of hanging

**Error Handling:**
- ✅ **Exception safety**: Catches and handles read errors
- ✅ **Resource cleanup**: Always restores original timeout
- ✅ **Sanity checks**: Validates data sizes and formats
- ✅ **Logging**: Debug info for troubleshooting

**Performance:**
- ✅ **Prevents busy waiting**: Small sleeps when no data
- ✅ **Chunked reading**: Reads large data in 1KB chunks
- ✅ **Early termination**: Stops on first valid match

### **3. All Functions Fixed**

**Core Functions:**
- `_search_header()` → `_search_header_with_timeout()`
- `read_frame()` → `read_frame_with_timeout()`
- `read_detections()` → `read_detections_with_timeout()`
- `read_embedding()` → `read_embedding_with_timeout()`
- `send_image()` → `send_image_with_timeout()`

**Added Safety Limits:**
- **Frame size**: Max 10MB per frame
- **Detection count**: Max 100 detections
- **Embedding size**: Max 1024 values
- **Loop iterations**: Max attempts for all operations

### **4. Backward Compatibility**

All original function names preserved as wrappers:
```python
def read_frame(ser):
    """Legacy wrapper for compatibility"""
    return read_frame_with_timeout(ser, timeout=3.0)
```

**No code changes needed** in existing UI files!

## 📊 **Timeout Settings**

| Function | Timeout | Max Attempts | Purpose |
|----------|---------|--------------|---------|
| `_search_header` | 2.0s | 50 | Find protocol headers |
| `read_frame` | 3.0s | - | Read image data |
| `read_detections` | 2.0s | - | Read detection results |
| `read_embedding` | 2.0s | - | Read embedding vectors |
| `send_image` | 10.0s | 10 frames | Send/receive cycle |

## 🎯 **Results**

### **Before (Original)**
- ❌ **Infinite loops** in `_search_header()` 
- ❌ **UI completely frozen** when no matching data
- ❌ **Unresponsive disconnect** button
- ❌ **No error recovery** mechanism

### **After (Fixed)**
- ✅ **2-second timeout** for all header searches
- ✅ **UI remains responsive** even with bad data
- ✅ **Clean disconnect/reconnect** functionality  
- ✅ **Automatic error recovery** with logging

## 🛠 **Technical Details**

### **Timeout Strategy**
```python
# Set short timeout for individual reads
ser.timeout = 0.1  # 100ms

# Overall operation timeout
while (time.time() - start_time) < timeout:
    # Attempt read with short timeout
    raw = ser.readline()
    if not raw:
        time.sleep(0.01)  # Small delay, don't busy wait
        continue
```

### **Chunked Data Reading**
```python
# Read large frames in chunks to prevent timeout
while bytes_read < size and (time.time() - start_time) < read_timeout:
    chunk = ser.read(min(remaining, 1024))  # 1KB chunks
    if chunk:
        frame_data.extend(chunk)
        bytes_read += len(chunk)
    else:
        time.sleep(0.01)  # Wait for more data
```

### **Resource Management**
```python
try:
    # Change timeout for operation
    original_timeout = ser.timeout
    ser.timeout = 0.1
    # ... perform operation
finally:
    # Always restore original timeout
    ser.timeout = original_timeout
```

## 🔄 **Migration Guide**

### **For Users**
- ✅ **No changes required** - existing code works unchanged
- ✅ **Better reliability** - no more infinite hangs
- ✅ **Faster error recovery** - 2-second max wait time

### **For Developers**
- ✅ **New timeout functions available** for fine control
- ✅ **Legacy functions preserved** for compatibility
- ✅ **Enhanced logging** for debugging

## 🎉 **Summary**

The infinite loop issue in `pc_uart_utils.py` has been completely resolved:

1. **All `while True` loops** now have timeout protection
2. **UI remains responsive** even with problematic serial data
3. **Automatic error recovery** prevents permanent hangs
4. **Backward compatibility** maintained for existing code
5. **Enhanced debugging** with detailed logging

**🎯 The UI will no longer get stuck forever and can handle any serial data gracefully!**

---

Generated with Claude Code - STM32N6 Object Detection Project