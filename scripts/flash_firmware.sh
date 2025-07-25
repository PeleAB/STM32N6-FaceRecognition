#!/bin/bash

# STM32N6 Firmware Flashing Script
# Flashes FSBL, application, and AI models to STM32N6570-DK

# set -e removed to prevent early exit on warnings/non-critical errors
set -u  # Exit on undefined variables

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CONFIG_FILE="$PROJECT_ROOT/stm32_tools_config.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

print_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

# Function to load configuration
load_config() {
    if [ ! -f "$CONFIG_FILE" ]; then
        print_error "Configuration file not found: $CONFIG_FILE"
        print_error "Please create and configure stm32_tools_config.json"
        exit 1
    fi
    
    # Extract tool paths from config
    PROGRAMMER_PATH=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['tools']['stm32programmer']['path'])" 2>/dev/null || echo "")
    EXTERNAL_LOADER_PATH=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['tools']['external_loader']['path'])" 2>/dev/null || echo "")
    
    # Validate programmer path
    if [ -z "$PROGRAMMER_PATH" ] || [ "$PROGRAMMER_PATH" = "/path/to/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI" ]; then
        print_error "STM32 Programmer path not configured in $CONFIG_FILE"
        print_error "Please update the 'stm32programmer.path' field with your actual installation path"
        exit 1
    fi
    
    if [ ! -f "$PROGRAMMER_PATH" ]; then
        print_error "STM32 Programmer not found at: $PROGRAMMER_PATH"
        print_error "Please verify the path in $CONFIG_FILE"
        exit 1
    fi
    
    # Validate external loader path
    if [ -z "$EXTERNAL_LOADER_PATH" ] || [ "$EXTERNAL_LOADER_PATH" = "/path/to/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/ExternalLoader/MX66UW1G45G_STM32N6570-DK.stldr" ]; then
        print_error "External loader path not configured in $CONFIG_FILE"
        print_error "Please update the 'external_loader.path' field with your actual installation path"
        exit 1
    fi
    
    if [ ! -f "$EXTERNAL_LOADER_PATH" ]; then
        print_error "External loader not found at: $EXTERNAL_LOADER_PATH"
        print_error "Please verify the path in $CONFIG_FILE"
        exit 1
    fi
    
    print_status "Using STM32 Programmer: $PROGRAMMER_PATH"
    print_status "Using External Loader: $EXTERNAL_LOADER_PATH"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS] [COMPONENTS...]"
    echo ""
    echo "Components:"
    echo "  fsbl               Flash FSBL (First Stage Boot Loader)"
    echo "  application        Flash signed application firmware"
    local available_models=$(get_available_models)
    for model_type in $available_models; do
        echo "  $model_type        Flash $model_type model"
    done
    echo "  models             Flash all AI models"
    echo "  all                Flash all components (default)"
    echo ""
    echo "Options:"
    echo "  -b, --binary-dir   Binary directory (default: ./Binary)"
    echo "  -v, --verify       Verify programming after flashing"
    echo "  -r, --reset        Reset device after programming"
    echo "  -h, --help         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                             # Flash all components"
    echo "  $0 fsbl application            # Flash only FSBL and application"
    echo "  $0 models                      # Flash only AI models"
    echo "  $0 -v -r application           # Flash application with verify and reset"
    echo ""
    echo "Prerequisites:"
    echo "  1. STM32N6570-DK connected via USB"
    echo "  2. BOOT1 switch in right position (dev mode)"  
    echo "  3. Required files in Binary directory:"
    echo "     - ai_fsbl.hex"
    echo "     - *_signed.hex (application)"
    local available_models=$(get_available_models)
    for model_type in $available_models; do
        echo "     - ${model_type}_data.hex"
    done
}

# Function to check prerequisites
check_prerequisites() {
    print_step "Checking prerequisites"
    
    # Check if device is connected
    print_status "Checking STM32N6570-DK connection..."
    
    local connection_test="$("$PROGRAMMER_PATH" -c port=SWD mode=HOTPLUG -el "$EXTERNAL_LOADER_PATH" 2>&1 || true)"
    
    if echo "$connection_test" | grep -q "Error" && ! echo "$connection_test" | grep -q "Connection established"; then
        print_error "Failed to connect to STM32N6570-DK"
        print_error "Please check:"
        print_error "1. USB cable is connected"
        print_error "2. BOOT1 switch is in right position (dev mode)"
        print_error "3. Board is powered on"
        return 1
    fi
    
    print_status "STM32N6570-DK connection verified"
    return 0
}

# Function to verify file exists
verify_file() {
    local file_path="$1"
    local description="$2"
    
    if [ ! -f "$file_path" ]; then
        print_error "$description file not found: $file_path"
        return 1
    fi
    
    local file_size=$(stat -c%s "$file_path")
    print_status "$description file found: $(basename "$file_path") (${file_size} bytes)"
    return 0
}

# Function to flash a single component
flash_component() {
    local file_path="$1"
    local description="$2"
    local verify_flag="$3"
    
    print_step "Flashing $description"
    
    local cmd=(
        "$PROGRAMMER_PATH"
        "-c" "port=SWD" "mode=HOTPLUG"
        "-el" "$EXTERNAL_LOADER_PATH"
        "-hardRst"
        "-w" "$file_path"
    )
    
    if [ "$verify_flag" = "true" ]; then
        cmd+=("-v")
    fi
    
    print_status "Running: ${cmd[*]}"
    
    # Run STM32CubeProgrammer and capture exit code
    local flash_exit_code=0
    "${cmd[@]}" > /tmp/flash_output.log 2>&1 || flash_exit_code=$?
    
    # Check output for success indicators instead of relying solely on exit code
    if [ $flash_exit_code -eq 0 ] || (grep -q "Programming Complete" /tmp/flash_output.log && ! grep -q "Error" /tmp/flash_output.log); then
        print_status "$description flashed successfully"
        return 0
    else
        print_error "$description flashing failed (exit code: $flash_exit_code)"
        print_error "STM32CubeProgrammer output:"
        cat /tmp/flash_output.log
        return 1
    fi
}

# Function to flash FSBL
flash_fsbl() {
    local binary_dir="$1"
    local verify_flag="$2"
    
    local fsbl_file="$binary_dir/ai_fsbl.hex"
    
    if verify_file "$fsbl_file" "FSBL"; then
        flash_component "$fsbl_file" "FSBL (First Stage Boot Loader)" "$verify_flag"
        return $?
    else
        return 1
    fi
}

# Function to flash application
flash_application() {
    local binary_dir="$1"
    local verify_flag="$2"
    
    # Look for signed hex file
    local app_file=$(find "$binary_dir" -name "*_signed.hex" | head -1)
    
    if [ -z "$app_file" ]; then
        print_error "No signed application hex file found in $binary_dir"
        print_error "Please run scripts/sign_binary.sh first"
        return 1
    fi
    
    if verify_file "$app_file" "Application"; then
        flash_component "$app_file" "Application Firmware" "$verify_flag"
        return $?
    else
        return 1
    fi
}

# Function to get available models from config
get_available_models() {
    python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(' '.join(config['models'].keys()))" 2>/dev/null || echo ""
}

# Function to flash a model
flash_model() {
    local model_type="$1"
    local binary_dir="$2" 
    local verify_flag="$3"
    
    local model_file="$binary_dir/${model_type}_data.hex"
    
    if verify_file "$model_file" "$model_type Model"; then
        flash_component "$model_file" "$model_type Model" "$verify_flag"
        return $?
    else
        return 1
    fi
}

# Function to reset device
reset_device() {
    print_step "Resetting device"
    
    local cmd=(
        "$PROGRAMMER_PATH"
        "-c" "port=SWD" "mode=HOTPLUG"
        "-rst"
    )
    
    if "${cmd[@]}" > /tmp/reset_output.log 2>&1; then
        print_status "Device reset successfully"
        return 0
    else
        print_warning "Device reset failed (this may be normal)"
        return 0  # Don't fail the entire process for reset failure
    fi
}

# Function to display memory layout
show_memory_layout() {
    print_status "STM32N6 External Flash Memory Layout:"
    echo "┌─────────────────────────────────────────┐"
    
    # Show FSBL and Application
    local fsbl_address=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['memory_layout']['fsbl_address'])" 2>/dev/null || echo "0x70000000")
    local app_address=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['memory_layout']['application_address'])" 2>/dev/null || echo "0x70100000")
    
    echo "│ $fsbl_address - FSBL (1MB)                │"
    echo "├─────────────────────────────────────────┤"
    echo "│ $app_address - Application (8MB)         │"
    
    # Show models dynamically
    local available_models=$(get_available_models)
    for model_type in $available_models; do
        local address=$(python3 -c "import json; config=json.load(open('$CONFIG_FILE')); print(config['models']['$model_type']['address'])" 2>/dev/null || echo "N/A")
        local model_name=$(echo "$model_type" | sed 's/_/ /g' | sed 's/\b\w/\U&/g')
        echo "├─────────────────────────────────────────┤"
        printf "│ %-39s │\n" "$address - $model_name (16MB)"
    done
    
    echo "└─────────────────────────────────────────┘"
}

# Main execution
main() {
    print_status "STM32N6 Firmware Flashing Script"
    print_status "================================="
    
    # Parse command line arguments
    local binary_dir="$PROJECT_ROOT/embedded/Binary"
    local verify_flag="false"
    local reset_flag="false"
    local components=()
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -b|--binary-dir)
                binary_dir="$2"
                shift 2
                ;;
            -v|--verify)
                verify_flag="true"
                shift
                ;;
            -r|--reset)
                reset_flag="true"
                shift
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
                components+=("$1")
                shift
                ;;
        esac
    done
    
    # Default to all components if none specified
    if [ ${#components[@]} -eq 0 ]; then
        components=("all")
    fi
    
    # Convert to absolute path
    binary_dir=$(realpath "$binary_dir")
    
    print_status "Binary directory: $binary_dir"
    print_status "Verify after programming: $verify_flag"
    print_status "Reset after programming: $reset_flag"
    print_status "Components to flash: ${components[*]}"
    
    # Load configuration
    load_config
    
    # Check prerequisites
    if ! check_prerequisites; then
        exit 1
    fi
    
    # Show memory layout
    show_memory_layout
    
    # Process components
    local flash_success=true
    local components_flashed=0
    local available_models=$(get_available_models)
    
    for component in "${components[@]}"; do
        case "$component" in
            fsbl)
                if flash_fsbl "$binary_dir" "$verify_flag"; then
                    ((components_flashed++))
                else
                    flash_success=false
                fi
                ;;
            application)
                if flash_application "$binary_dir" "$verify_flag"; then
                    ((components_flashed++))
                else
                    flash_success=false
                fi
                ;;
            models)
                # Flash all available models
                for model_type in $available_models; do
                    if flash_model "$model_type" "$binary_dir" "$verify_flag"; then
                        ((components_flashed++))
                    else
                        flash_success=false
                    fi
                done
                ;;
            all)
                # Flash FSBL first
                local fsbl_result=0
                flash_fsbl "$binary_dir" "$verify_flag" || fsbl_result=$?
                if [ $fsbl_result -eq 0 ]; then
                    ((components_flashed++))
                    print_status "✅ FSBL flashing completed"
                else
                    flash_success=false
                    print_error "❌ FSBL flashing failed"
                fi
                
                # Flash all models
                for model_type in $available_models; do
                    local model_result=0
                    flash_model "$model_type" "$binary_dir" "$verify_flag" || model_result=$?
                    if [ $model_result -eq 0 ]; then
                        ((components_flashed++))
                        print_status "✅ $model_type model flashing completed"
                    else
                        flash_success=false
                        print_error "❌ $model_type model flashing failed"
                    fi
                done
                
                # Flash application last
                local app_result=0
                flash_application "$binary_dir" "$verify_flag" || app_result=$?
                if [ $app_result -eq 0 ]; then
                    ((components_flashed++))
                    print_status "✅ Application flashing completed"
                else
                    flash_success=false
                    print_error "❌ Application flashing failed"
                fi
                ;;
            *)
                # Check if it's a model type
                if [[ " $available_models " =~ " $component " ]]; then
                    if flash_model "$component" "$binary_dir" "$verify_flag"; then
                        ((components_flashed++))
                    else
                        flash_success=false
                    fi
                else
                    print_error "Unknown component: $component"
                    print_error "Valid components: fsbl, application, $available_models, models, all"
                    flash_success=false
                fi
                ;;
        esac
    done
    
    # Reset device if requested
    if [ "$reset_flag" = "true" ] && [ "$flash_success" = "true" ]; then
        reset_device
    fi
    
    # Display results
    if [ "$flash_success" = "true" ]; then
        print_status "🎉 Firmware flashing completed successfully!"
        print_status "Components flashed: $components_flashed"
        print_status ""
        print_status "Next steps:"
        print_status "1. Switch BOOT1 to left position (boot from flash)"
        print_status "2. Power cycle the board"
        print_status "3. Check UART output at 921600 baud"
        print_status "4. Connect PC streaming client for face detection/recognition"
    else
        print_error "❌ Firmware flashing completed with errors"
        print_error "Please check the error messages above and try again"
        exit 1
    fi
    
    # Clean up temporary files
    rm -f /tmp/flash_output.log /tmp/reset_output.log
}

# Execute main function
main "$@"