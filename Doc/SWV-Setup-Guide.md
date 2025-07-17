# STM32CubeIDE SWV Setup Guide

## How to View Educational Pipeline Output

The educational pipeline in `main.c` contains detailed printf statements that show each stage of the face detection and recognition process. Here's how to view them:

## Option 1: SWV (Serial Wire Viewer) - Recommended ✅

### Step 1: Enable SWV in STM32CubeIDE
1. **Build the project** in Debug mode
2. **Start Debug session**: Click Debug button or press F11
3. **Open SWV Console**: 
   - Go to `Window` → `Show View` → `SWV` → `SWV ITM Data Console`
   - Or press `Ctrl+3` and type "SWV ITM Data Console"

### Step 2: Configure SWV Settings
1. **Configure ITM**:
   - In SWV ITM Data Console, click the **Configure** button (gear icon)
   - Set **Core Clock**: `600000000` (600MHz for STM32N6)
   - Set **SWO Clock**: `2000000` (2MHz)
   - **Enable Port 0**: Check the box for Port 0
   - Click **OK**

### Step 3: Start SWV Trace
1. **Start the trace**: Click the **Start Trace** button (red record button)
2. **Resume execution**: Press F8 or click Resume button
3. **View output**: You should see the educational pipeline output in the console

### Expected Output:
```
🚀 Initializing Camera and Display Systems
✅ Systems initialized, starting pipeline
═══════════════════════════════════════════════════════════
🔄 STARTING FRAME 1 PROCESSING PIPELINE
📸 PIPELINE STAGE 1: Frame Capture
   🔄 Converting RGB to CHW format for neural network...
   🧠 Preparing 49152 bytes for neural network input...
✅ Frame captured and preprocessed (128x128 → 49152 bytes)
🧠 PIPELINE STAGE 2: Face Detection Network
   🚀 Running face detection neural network inference...
   🧹 Cleaning up neural network resources...
✅ Face detection completed in 45 ms (4 outputs ready)
⚙️ PIPELINE STAGE 3: Post-Processing
   🔍 Processing 4 neural network outputs...
   📦 Extracted 1 face bounding boxes
   📍 Face 1: confidence=0.892, center=(0.45,0.52), size=0.25x0.31
✅ Post-processing completed: 1 faces detected
🔍 PIPELINE STAGE 4: Face Recognition
✅ Face recognition completed: detected=YES, verified=YES, similarity=0.847
💡 PIPELINE STAGE 5: System Status Update
✅ System status updated
📊 PIPELINE STAGE 6: Output and Metrics
✅ Frame processing completed: 12.5 FPS, 80 ms total
═══════════════════════════════════════════════════════════
```

## Option 2: UART Output (Alternative)

If SWV doesn't work, you can also redirect printf to UART:

### Step 1: Enable UART in STM32CubeMX
1. Open your `.ioc` file
2. Enable UART (e.g., USART1) 
3. Configure pins and settings
4. Generate code

### Step 2: Modify syscalls.c
Replace the `__io_putchar` function with UART output:

```c
extern UART_HandleTypeDef huart1;  // Add this line

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
```

### Step 3: Connect Serial Monitor
1. Connect a USB-to-Serial adapter to the UART pins
2. Use a serial terminal (PuTTY, TeraTerm, etc.)
3. Set baud rate to match your UART configuration (usually 115200)

## Option 3: PC Stream Output (Already Implemented)

The workshop already has PC streaming capability through `enhanced_pc_stream.c`. Students can use the Python tools in `python_tools/` to view the output.

## Troubleshooting

### SWV Not Working?
1. **Check Debug Configuration**:
   - Ensure you're using ST-Link debugger
   - Check that SWO is enabled in debugger settings
2. **Verify Clock Settings**:
   - Core Clock should match your system clock (600MHz)
   - SWO Clock should be reasonable (1-2MHz)
3. **Check Port Settings**:
   - Ensure Port 0 is enabled in ITM configuration

### No Output Visible?
1. **Most Common Issue - Missing DEBUG Define**:
   - Right-click project → Properties
   - C/C++ Build → Settings → Tool Settings
   - MCU GCC Compiler → Preprocessor
   - Add `DEBUG` to the defined symbols
   - Clean and rebuild project
2. **Check Trace Status**: Ensure trace is started and running
3. **Verify ITM Initialize**: Make sure ITM is properly initialized
4. **Check SWO Pin**: Ensure SWO pin is correctly configured in your debug configuration

### Alternative Debug Methods
If SWV still doesn't work, the system includes:
1. **LED Visual Indicators**: LED1 blinks during Stage 1 (Frame Capture)
2. **PC Stream Output**: Use Python tools in `python_tools/` directory
3. **UART Fallback**: Can redirect printf to UART (see guide above)

### Performance Impact
- SWV output will slow down execution slightly
- For production code, remove printf statements or use conditional compilation
- The educational output is designed for learning, not production use

## Educational Benefits

This setup allows students to:
- **See real-time pipeline execution** step by step
- **Understand data flow** through the system
- **Monitor performance metrics** (timing, FPS, etc.)
- **Debug issues** by seeing exactly where problems occur
- **Learn embedded debugging** techniques using professional tools

The detailed logging makes the face detection and recognition pipeline transparent and educational, helping students understand complex computer vision concepts on embedded systems.