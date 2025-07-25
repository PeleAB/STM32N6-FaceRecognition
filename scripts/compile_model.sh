#!/bin/bash

# STM32EdgeAI Model Compilation Script
# Converts ONNX/TFLite models to STM32-compatible code using STM32 Edge AI

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CONFIG_FILE="$PROJECT_ROOT/stm32_tools_config.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to load configuration
load_config() {
    if [ ! -f "$CONFIG_FILE" ]; then
        print_error "Configuration file not found: $CONFIG_FILE"
        print_error "Please create and configure stm32_tools_config.json"
        exit 1
    fi
    
    # Extract STM32EdgeAI path from config
    STEDGEAI_PATH=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['tools']['stm32edgeai']['path'])" 2>/dev/null || echo "")
    
    if [ -z "$STEDGEAI_PATH" ] || [ "$STEDGEAI_PATH" = "/path/to/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-AI/10.1.0/Utilities/linux/stedgeai" ]; then
        print_error "STM32EdgeAI path not configured in $CONFIG_FILE"
        print_error "Please update the 'stm32edgeai.path' field with your actual installation path"
        exit 1
    fi
    
    if [ ! -f "$STEDGEAI_PATH" ]; then
        print_error "STM32EdgeAI tool not found at: $STEDGEAI_PATH" 
        print_error "Please verify the path in $CONFIG_FILE"
        exit 1
    fi
    
    if [ ! -x "$STEDGEAI_PATH" ]; then
        print_error "STM32EdgeAI tool is not executable: $STEDGEAI_PATH"
        exit 1
    fi
    
    print_status "Using STM32EdgeAI: $STEDGEAI_PATH"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 <model_type> <model_file>"
    echo ""
    echo "Arguments:"
    echo "  model_type    Either 'face_detection' or 'face_recognition'"
    echo "  model_file    Path to ONNX or TFLite model file"
    echo ""
    echo "Examples:"
    echo "  $0 face_detection ./input_models/centerface.tflite"
    echo "  $0 face_recognition ./input_models/mobilefacenet_int8_faces.onnx"
    echo ""
    echo "Output:"
    echo "  Generated files will be placed in ./converted_models/"
    echo "  - Code files: face_detection.c, face_detection.h, etc."
    echo "  - Binary files: face_detection_data.bin, face_detection_data.hex"
}

# Function to generate memory pool file dynamically
generate_memory_pool() {
    local model_type="$1"
    local mpool_file="$2"
    
    # Get memory address for this model type
    local address=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['models']['$model_type']['address'])")
    
    print_status "Generating memory pool for $model_type at address $address"
    
    cat > "$mpool_file" << EOF
{
	"params": {
		"param": [
			{ "paramname": "max_onchip_sram_size", "value": "1024", "magnitude": "KBYTES" }
		]
	},
	"memory": {
		"cacheinfo": [
			{
				"nlines": 512,
				"linesize": 64,
				"associativity": 8,
				"bypass_enable": 1,
				"prop": { "rights": "ACC_WRITE",  "throughput": "MID",   "latency": "MID", "byteWidth": 8, "freqRatio": 2.50, "read_power": 13.584, "write_power": 12.645 }
			}
		],
		"mem_file_prefix": "atonbuf",
		"mempools": [
			{
				"fname": "AXIFLEXMEM",
				"name":  "flexMEM",
				"fformat": "FORMAT_RAW",
				"prop":	  { "rights": "ACC_WRITE", "throughput": "MID",  "latency": "MID", "byteWidth": 8, "freqRatio": 2.50, "read_power": 9.381,  "write_power": 8.569 },
				"offset": { "value": "0x34000000", "magnitude":  "BYTES" },
				"size":   { "value": "0",        "magnitude": "KBYTES" }
			},
			{
				"fname": "AXISRAM1",
				"name":  "cpuRAM1",
				"fformat": "FORMAT_RAW",
				"prop":	  { "rights": "ACC_WRITE", "throughput": "MID",  "latency": "MID", "byteWidth": 8, "freqRatio": 2.50, "read_power": 16.616, "write_power": 14.522 },
				"offset": { "value": "0x34080000", "magnitude":  "BYTES" },
				"size":   { "value": "0",        "magnitude": "KBYTES" }
			},
			{
				"fname": "AXISRAM2",
				"name":  "cpuRAM2",
				"fformat": "FORMAT_RAW",
				"prop":	  { "rights": "ACC_WRITE", "throughput": "MID",  "latency": "MID", "byteWidth": 8, "freqRatio": 2.50, "read_power": 17.324, "write_power": 15.321 },
				"offset": { "value": "0x34100000", "magnitude":  "BYTES" },
				"size":   { "value": "1024",       "magnitude": "KBYTES" }
			},
			{
				"fname": "AXISRAM3",
				"name":  "npuRAM3",
				"fformat": "FORMAT_RAW",
				"prop":	  { "rights": "ACC_WRITE", "throughput": "HIGH", "latency": "LOW", "byteWidth": 8, "freqRatio": 1.25, "read_power": 18.531, "write_power": 16.201 },
				"offset": { "value": "0x34200000", "magnitude":  "BYTES" },
				"size":   { "value": "448",        "magnitude": "KBYTES" }
			},
			{
				"fname": "AXISRAM4",
				"name":  "npuRAM4",
				"fformat": "FORMAT_RAW",
				"prop":	  { "rights": "ACC_WRITE", "throughput": "HIGH", "latency": "LOW", "byteWidth": 8, "freqRatio": 1.25, "read_power": 18.531, "write_power": 16.201 },
				"offset": { "value": "0x34270000", "magnitude":  "BYTES" },
				"size":   { "value": "448",        "magnitude": "KBYTES" }
			},
			{
				"fname": "AXISRAM5",
				"name":	 "npuRAM5",
				"fformat": "FORMAT_RAW",
				"prop":	  { "rights": "ACC_WRITE", "throughput": "HIGH", "latency": "LOW", "byteWidth": 8, "freqRatio": 1.25, "read_power": 18.531, "write_power": 16.201 },
				"offset": { "value": "0x342e0000", "magnitude":  "BYTES" },
				"size":   { "value": "448",        "magnitude": "KBYTES" }
			},
			{
				"fname": "AXISRAM6",
				"name":  "npuRAM6",
				"fformat": "FORMAT_RAW",
				"prop":	  { "rights": "ACC_WRITE", "throughput": "HIGH", "latency": "LOW", "byteWidth": 8, "freqRatio": 1.25, "read_power": 19.006, "write_power": 15.790 },
				"offset": { "value": "0x34350000", "magnitude":  "BYTES" },
				"size":   { "value": "448",        "magnitude": "KBYTES" }
			},
			{
				"fname": "xSPI1",
				"name":  "hyperRAM",
				"fformat": "FORMAT_RAW",
				"prop":	  { "rights": "ACC_WRITE", "throughput": "MID", "latency": "HIGH", "byteWidth": 2, "freqRatio": 5.00, "cacheable": "CACHEABLE_ON","read_power": 380, "write_power": 340.0, "constants_preferred": "true" },
				"offset": { "value": "0x90000000", "magnitude":  "BYTES" },
				"size":   { "value": "16",         "magnitude": "MBYTES" }
			},
			{
				"fname": "xSPI2",
				"name":  "octoFlash",
				"fformat": "FORMAT_RAW",
				"prop":	  { "rights": "ACC_READ",  "throughput": "MID", "latency": "HIGH", "byteWidth": 1, "freqRatio": 6.00, "cacheable": "CACHEABLE_ON", "read_power": 110, "write_power": 400.0, "constants_preferred": "true" },
				"offset": { "value": "$address", "magnitude":  "BYTES" },
				"size":   { "value": "61",         "magnitude": "MBYTES" }
			}
		]
	}
}
EOF
    
    print_status "Generated memory pool file: $mpool_file"
}

# Function to create neural art configuration
create_neural_art_config() {
    local model_type="$1"
    local config_file="$2"
    local mpool_file="/tmp/${model_type}.mpool"
    
    # Generate dynamic memory pool file
    generate_memory_pool "$model_type" "$mpool_file"
    
    # Get model configuration from main config
    local options=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['models']['$model_type']['stedgeai_options'])")
    
    cat > "$config_file" << EOF
{
    "Globals": {},
    "Profiles": {
        "$model_type": {
            "memory_pool": "$mpool_file",
            "options": "$options"
        }
    }
}
EOF
    
    print_status "Created neural art configuration: $config_file"
}

# Function to convert model using STM32EdgeAI
convert_model() {
    local model_type="$1"
    local model_file="$2"
    local output_dir="$PROJECT_ROOT/converted_models"
    local config_file="/tmp/${model_type}_config.json"
    
    # Create output directory
    mkdir -p "$output_dir"
    
    # Create neural art configuration
    create_neural_art_config "$model_type" "$config_file"
    
    # Get model configuration
    local output_name=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['models']['$model_type']['name'])")
    local target=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['models']['$model_type']['target'])")
    local input_data_type=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['models']['$model_type']['input_data_type'])")
    
    print_status "Converting $model_type model: $(basename "$model_file")"
    print_status "Output directory: $output_dir"
    
    # Run STM32EdgeAI conversion
    cd "$PROJECT_ROOT"
    
    local cmd=(
        "$STEDGEAI_PATH" "generate"
        "--name" "$output_name"
        "--model" "$model_file"
        "--target" "$target"
        "--st-neural-art" "${model_type}@${config_file}"
        "--input-data-type" "$input_data_type"
        "--output" "$output_dir"
    )
    
    print_status "Running: ${cmd[*]}"
    
    # Run STM32EdgeAI and capture output and exit code
    local conversion_exit_code=0
    "${cmd[@]}" || conversion_exit_code=$?
    
    # Check if essential files were generated despite any warnings/errors
    local essential_files_exist=true
    for pattern in "${model_type}*.c" "${model_type}*.h"; do
        if ! ls "$output_dir"/$pattern 1> /dev/null 2>&1; then
            essential_files_exist=false
            break
        fi
    done
    
    if [ "$essential_files_exist" = "true" ]; then
        if [ "$conversion_exit_code" -eq 0 ]; then
            print_status "Model conversion successful"
        else
            print_warning "Model conversion completed with warnings (exit code: $conversion_exit_code)"
            print_status "Essential files were generated, continuing..."
        fi
        
        # Clean up temp config and memory pool files
        rm -f "$config_file"
        rm -f "/tmp/${model_type}.mpool"
        
        return 0
    else
        print_error "Model conversion failed - essential files not generated"
        print_error "Exit code: $conversion_exit_code"
        rm -f "$config_file"
        rm -f "/tmp/${model_type}.mpool"
        return 1
    fi
}

# Function to organize output files
organize_output_files() {
    local model_type="$1"
    local output_dir="$PROJECT_ROOT/converted_models"
    
    print_status "Organizing output files for $model_type"
    
    # Create subdirectories
    local code_dir="$output_dir/code"
    local binaries_dir="$output_dir/binaries"
    mkdir -p "$code_dir" "$binaries_dir"
    
    cd "$output_dir"
    
    # Move code files
    for pattern in "${model_type}*.c" "${model_type}*.h" "${model_type}*_ecblobs.h"; do
        for file in $pattern; do
            if [ -f "$file" ]; then
                mv "$file" "$code_dir/"
                print_status "Moved $file to code/"
            fi
        done
    done
    
    # Handle binary files
    local binary_file=$(find . -maxdepth 1 -name "${model_type}*.raw" | head -1)
    if [ -n "$binary_file" ] && [ -f "$binary_file" ]; then
        local bin_output="$binaries_dir/${model_type}_data.bin"
        local hex_output="$binaries_dir/${model_type}_data.hex"
        
        cp "$binary_file" "$bin_output"
        print_status "Copied binary: $(basename "$binary_file") to binaries/${model_type}_data.bin"
        
        # Get memory address for this model type
        local address=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['models']['$model_type']['address'])")
        
        # Convert to Intel HEX format
        if command -v arm-none-eabi-objcopy &> /dev/null; then
            if arm-none-eabi-objcopy -I binary "$bin_output" --change-addresses "$address" -O ihex "$hex_output"; then
                print_status "Generated HEX file: ${model_type}_data.hex at address $address"
            else
                print_warning "HEX generation failed for $model_type"
            fi
        else
            print_warning "arm-none-eabi-objcopy not found, skipping HEX generation"
        fi
        
        # Clean up original raw file
        rm -f "$binary_file"
    else
        print_warning "No binary file found for $model_type"
    fi
}

# Function to copy files to project
copy_to_project() {
    local model_type="$1"
    local output_dir="$PROJECT_ROOT/converted_models"
    
    # Ensure target directories exist
    local models_dir="$PROJECT_ROOT/embedded/Models"
    local binary_dir="$PROJECT_ROOT/embedded/Binary"
    mkdir -p "$models_dir" "$binary_dir"
    
    # Copy generated C files to embedded Models directory
    local code_dir="$output_dir/code"  
    
    if [ -d "$code_dir" ]; then
        for file in "$code_dir"/${model_type}*.c "$code_dir"/${model_type}*.h; do
            if [ -f "$file" ]; then
                cp "$file" "$models_dir/"
                print_status "Copied $(basename "$file") to embedded/Models/"
            fi
        done
    fi
    
    # Copy HEX file to embedded Binary directory
    local binaries_dir="$output_dir/binaries"
    local hex_file="$binaries_dir/${model_type}_data.hex"
    
    if [ -f "$hex_file" ]; then
        cp "$hex_file" "$binary_dir/"
        print_status "Copied ${model_type}_data.hex to embedded/Binary/"
    fi
}

# Main execution
main() {
    print_status "STM32EdgeAI Model Compilation Script"
    print_status "======================================"
    
    # Check arguments
    if [ $# -ne 2 ]; then
        show_usage
        exit 1
    fi
    
    local model_type="$1"  
    local model_file="$2"
    
    # Validate model type
    if [ "$model_type" != "face_detection" ] && [ "$model_type" != "face_recognition" ]; then
        print_error "Invalid model type: $model_type"
        print_error "Must be either 'face_detection' or 'face_recognition'"
        exit 1
    fi
    
    # Validate model file
    if [ ! -f "$model_file" ]; then
        print_error "Model file not found: $model_file"
        exit 1
    fi
    
    # Load configuration
    load_config
    
    # Convert model
    if convert_model "$model_type" "$model_file"; then
        # Organize output files
        organize_output_files "$model_type"
        
        # Copy to project directories
        copy_to_project "$model_type"
        
        print_status "Model compilation completed successfully!"
        print_status "Next steps:"
        print_status "1. Build your STM32CubeIDE project in embedded/STM32CubeIDE/"
        print_status "2. Sign the application: ./scripts/sign_binary.sh (uses configured path)" 
        print_status "3. Flash firmware: ./scripts/flash_firmware.sh all"
    else
        print_error "Model compilation failed!"
        exit 1
    fi
}

# Execute main function
main "$@"