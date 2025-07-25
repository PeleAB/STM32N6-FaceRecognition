#!/bin/bash

# STM32 Binary Signing Script
# Signs and converts application binary for STM32N6 deployment

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
    
    # Extract tool paths from config
    SIGNING_TOOL_PATH=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['tools']['stm32signingtool']['path'])" 2>/dev/null || echo "")
    ARM_OBJCOPY_PATH=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['tools']['arm_gcc_toolchain']['path'])" 2>/dev/null || echo "")
    APPLICATION_ADDRESS=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['memory_layout']['application_address'])" 2>/dev/null || echo "0x70100000")
    DEFAULT_BINARY_PATH=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['build']['application_binary_path'])" 2>/dev/null || echo "")
    
    # Validate signing tool path
    if [ -z "$SIGNING_TOOL_PATH" ] || [ "$SIGNING_TOOL_PATH" = "/path/to/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_SigningTool_CLI" ]; then
        print_error "STM32 Signing Tool path not configured in $CONFIG_FILE"
        print_error "Please update the 'stm32signingtool.path' field with your actual installation path"
        exit 1
    fi
    
    if [ ! -f "$SIGNING_TOOL_PATH" ]; then
        print_error "STM32 Signing Tool not found at: $SIGNING_TOOL_PATH"
        print_error "Please verify the path in $CONFIG_FILE"
        exit 1
    fi
    
    # Validate ARM objcopy (try both configured path and system PATH)
    local arm_objcopy_cmd=""
    if [ -n "$ARM_OBJCOPY_PATH" ] && [ "$ARM_OBJCOPY_PATH" != "/path/to/arm-none-eabi-gcc" ]; then
        if [ -d "$ARM_OBJCOPY_PATH" ]; then
            arm_objcopy_cmd="$ARM_OBJCOPY_PATH/arm-none-eabi-objcopy"
        else
            arm_objcopy_cmd="$ARM_OBJCOPY_PATH"
        fi
    else
        arm_objcopy_cmd="arm-none-eabi-objcopy"
    fi
    
    if ! command -v "$arm_objcopy_cmd" &> /dev/null; then
        print_error "arm-none-eabi-objcopy not found"
        print_error "Please configure the ARM GCC toolchain path in $CONFIG_FILE"
        exit 1
    fi
    
    ARM_OBJCOPY="$arm_objcopy_cmd"
    
    print_status "Using STM32 Signing Tool: $SIGNING_TOOL_PATH"
    print_status "Using ARM objcopy: $ARM_OBJCOPY"
    print_status "Application address: $APPLICATION_ADDRESS"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS] [input_binary]"
    echo ""
    echo "Arguments:"
    echo "  input_binary    Path to the application binary file (.bin)"
    echo "                  If not provided, uses path from stm32_tools_config.json"
    echo ""
    echo "Options:"
    echo "  -o, --output    Output directory (default: ./embedded/Binary)"
    echo "  -n, --name      Output filename prefix (default: derived from input)"
    echo "  -h, --help      Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                                              # Use configured binary path"
    echo "  $0 ./embedded/STM32CubeIDE/Debug/STM32N6_GettingStarted_ObjectDetection.bin"
    echo "  $0 -o ./build -n MyApp ./path/to/application.bin"
    echo ""
    echo "Configuration:"
    echo "  Default binary path is set in stm32_tools_config.json under 'build.application_binary_path'"
    echo ""
    echo "Output:"
    echo "  - <name>_signed.bin: Signed binary file"
    echo "  - <name>_signed.hex: Intel HEX file for programming"
}

# Function to sign binary
sign_binary() {
    local input_bin="$1"
    local output_dir="$2"
    local output_name="$3"
    
    local signed_bin="$output_dir/${output_name}_signed.bin"
    
    print_status "Signing binary: $(basename "$input_bin")"
    
    local cmd=(
        "$SIGNING_TOOL_PATH"
        "-bin" "$input_bin"
        "-nk"
        "-t" "ssbl"
        "-hv" "2.3"
        "--silent"
        "-o" "$signed_bin"
    )
    
    print_status "Running: ${cmd[*]}"
    
    # Run the signing command and capture exit code
    if "${cmd[@]}"; then
        print_status "Binary signed successfully: $signed_bin"
        
        # Verify the signed binary was actually created
        if [ ! -f "$signed_bin" ]; then
            print_error "Signing tool claimed success but signed binary not found: $signed_bin"
            return 1
        fi
        
        return 0
    else
        print_error "Binary signing failed"
        return 1
    fi
}

# Function to convert to Intel HEX
convert_to_hex() {
    local signed_bin="$1"
    local output_dir="$2"
    local output_name="$3"
    
    local signed_hex="$output_dir/${output_name}_signed.hex"
    
    print_status "Converting to Intel HEX format"
    
    # Verify input file exists
    if [ ! -f "$signed_bin" ]; then
        print_error "Signed binary file not found: $signed_bin"
        return 1
    fi
    
    local cmd=(
        "$ARM_OBJCOPY"
        "-I" "binary"
        "-O" "ihex"
        "--change-addresses=$APPLICATION_ADDRESS"
        "$signed_bin"
        "$signed_hex"
    )
    
    print_status "Running: ${cmd[*]}"
    
    # Capture both stdout and stderr for objcopy
    local temp_output=$(mktemp)
    local exit_code=0
    "${cmd[@]}" > "$temp_output" 2>&1 || exit_code=$?
    
    # Show any error output from objcopy
    if [ -s "$temp_output" ]; then
        print_warning "objcopy output:"
        cat "$temp_output"
    fi
    rm -f "$temp_output"
    
    if [ "$exit_code" -eq 0 ]; then
        print_status "HEX conversion successful: $signed_hex"
        return 0
    else
        print_error "HEX conversion failed (exit code: $exit_code)"
        return 1
    fi
}

# Function to validate binary file
validate_binary() {
    local binary_file="$1"
    
    if [ ! -f "$binary_file" ]; then
        print_error "Binary file not found: $binary_file"
        return 1
    fi
    
    local file_size=$(stat -c%s "$binary_file")
    if [ "$file_size" -eq 0 ]; then
        print_error "Binary file is empty: $binary_file"
        return 1
    fi
    
    print_status "Binary file validation passed: $(basename "$binary_file") (${file_size} bytes)"
    return 0
}

# Main execution
main() {
    print_status "STM32 Binary Signing Script"
    print_status "==========================="
    
    # Parse command line arguments
    local input_binary=""
    local output_dir="$PROJECT_ROOT/embedded/Binary"
    local output_name=""
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -o|--output)
                output_dir="$2"
                shift 2
                ;;
            -n|--name)
                output_name="$2"
                shift 2
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            -*)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
            *)
                if [ -z "$input_binary" ]; then
                    input_binary="$1"
                else
                    print_error "Multiple input files specified"
                    show_usage
                    exit 1
                fi
                shift
                ;;
        esac
    done
    
    # Check if input binary is provided, use default if not 
    if [ -z "$input_binary" ]; then
        # Try to get default binary path from config
        local default_path=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['build']['application_binary_path'])" 2>/dev/null || echo "")
        
        if [ -n "$default_path" ]; then
            # Use configured path, resolve relative to project root
            local resolved_path="$PROJECT_ROOT/$default_path"
            
            if [ -f "$resolved_path" ]; then
                input_binary="$resolved_path"
                print_status "Using configured binary path: $default_path"
            else
                print_error "Configured binary path does not exist: $resolved_path"
                print_error "Please either:"
                print_error "1. Build the application first"
                print_error "2. Provide a binary path as argument: $0 <path_to_binary>"
                print_error "3. Update 'build.application_binary_path' in $CONFIG_FILE"
                exit 1
            fi
        else
            print_error "No input binary specified and no default path configured"
            print_error "Please either:"
            print_error "1. Provide a binary path as argument: $0 <path_to_binary>"
            print_error "2. Configure 'build.application_binary_path' in $CONFIG_FILE"
            show_usage
            exit 1
        fi
    fi
    
    # Convert to absolute path
    input_binary=$(realpath "$input_binary")
    
    # Generate output name if not provided
    if [ -z "$output_name" ]; then
        output_name=$(basename "$input_binary" .bin)
    fi
    
    # Create output directory
    mkdir -p "$output_dir"
    output_dir=$(realpath "$output_dir")
    
    print_status "Input binary: $input_binary"
    print_status "Output directory: $output_dir"
    print_status "Output name: $output_name"
    
    # Load configuration
    load_config
    
    # Validate input binary
    if ! validate_binary "$input_binary"; then
        exit 1
    fi
    
    # Sign the binary
    local signed_bin="$output_dir/${output_name}_signed.bin"
    if sign_binary "$input_binary" "$output_dir" "$output_name" > /dev/null; then
        print_status "Signing completed successfully"
    else
        print_error "Signing failed"
        exit 1
    fi
    
    # Convert to Intel HEX
    local signed_hex="$output_dir/${output_name}_signed.hex"
    if convert_to_hex "$signed_bin" "$output_dir" "$output_name" > /dev/null; then
        print_status "HEX conversion completed successfully"
    else
        print_error "HEX conversion failed"
        exit 1
    fi
    
    # Display summary
    print_status "Binary signing and conversion completed!"
    print_status "Output files:"
    print_status "  Signed binary: $(basename "$signed_bin")"
    print_status "  Intel HEX:     $(basename "$signed_hex")"
    print_status ""
    print_status "Next steps:"
    print_status "1. Use scripts/flash_firmware.sh to flash the firmware"
    print_status "2. Or manually program $signed_hex to address $APPLICATION_ADDRESS"
}

# Execute main function
main "$@"