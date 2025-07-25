#!/bin/bash

# STM32N6 All Models Compilation Script
# Quality of Life script to compile all AI models with predefined paths

# set -e removed to prevent early exit on individual model failures
set -u  # Exit on undefined variables

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
INPUT_MODELS_DIR="$PROJECT_ROOT/input_models"

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

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Description:"
    echo "  Compiles all AI models for STM32N6 Face Recognition system"
    echo "  Automatically finds model files in input_models/ directory"
    echo ""
    echo "Options:"
    echo "  -m, --model MODEL_TYPE       Compile specific model type only"
    echo "  -l, --list                   List available model files"
    echo "  -h, --help                   Show this help message"
    echo ""
    echo "Available Model Types:"
    local available_models=$(get_available_models)
    for model_type in $available_models; do
        local filename=$(get_model_info "$model_type" "filename")
        echo "  $model_type: $filename"
    done
    echo ""
    echo "Examples:"
    echo "  $0                            # Compile all models"
    echo "  $0 -m face_detection          # Compile only face detection"
    echo "  $0 -m face_recognition        # Compile only face recognition"
    echo "  $0 -l                         # List available models"
}

# Function to list available models
list_models() {
    print_status "Available model files in $INPUT_MODELS_DIR:"
    echo ""
    
    if [ ! -d "$INPUT_MODELS_DIR" ]; then
        print_error "Input models directory not found: $INPUT_MODELS_DIR"
        return 1
    fi
    
    echo "Face Detection Models (.tflite):"
    find "$INPUT_MODELS_DIR" -name "*.tflite" -exec basename {} \; | sort | sed 's/^/  /'
    
    echo ""
    echo "Face Recognition Models (.onnx):"
    find "$INPUT_MODELS_DIR" -name "*.onnx" -exec basename {} \; | sort | sed 's/^/  /'
    
    echo ""
    print_status "Available models from configuration:"
    local available_models=$(get_available_models)
    for model_type in $available_models; do
        local filename=$(get_model_info "$model_type" "filename")
        local address=$(get_model_info "$model_type" "address")
        echo "  $model_type: $filename (@ $address)"
    done
}

# Function to get model info from config
get_model_info() {
    local model_type="$1"
    local field="$2"
    python3 -c "import json; config=json.load(open('$SCRIPT_DIR/../stm32_tools_config.json')); print(config['models']['$model_type']['$field'])" 2>/dev/null || echo ""
}

# Function to get all available models from config
get_available_models() {
    python3 -c "import json; config=json.load(open('$SCRIPT_DIR/../stm32_tools_config.json')); print(' '.join(config['models'].keys()))" 2>/dev/null || echo ""
}

# Function to find model file
find_model_file() {
    local model_type="$1"
    local filename=$(get_model_info "$model_type" "filename")
    local model_file="$INPUT_MODELS_DIR/$filename"
    
    if [ -f "$model_file" ]; then
        echo "$model_file"
        return 0
    else
        return 1
    fi
}

# Function to compile a model
compile_model() {
    local model_type="$1"
    local filename=$(get_model_info "$model_type" "filename")
    
    print_step "Compiling $model_type model"
    
    local model_file
    if model_file=$(find_model_file "$model_type"); then
        print_status "Found $model_type model: $(basename "$model_file")"
        
        # Run compile_model.sh and capture exit code
        local compile_exit_code=0
        "$SCRIPT_DIR/compile_model.sh" "$model_type" "$model_file" || compile_exit_code=$?
        
        if [ $compile_exit_code -eq 0 ]; then
            print_status "✅ $model_type model compiled successfully"
            return 0
        else
            print_error "❌ $model_type model compilation failed (exit code: $compile_exit_code)"
            return 1
        fi
    else
        print_error "$model_type model not found: $filename"
        print_warning "Please ensure $filename is in $INPUT_MODELS_DIR"
        return 1
    fi
}

# Function to check prerequisites
check_prerequisites() {
    print_status "Checking prerequisites..."
    
    # Check if input models directory exists
    if [ ! -d "$INPUT_MODELS_DIR" ]; then
        print_error "Input models directory not found: $INPUT_MODELS_DIR"
        print_error "Please create the directory and add your model files"
        return 1
    fi
    
    # Check if compile_model.sh exists and is executable
    if [ ! -f "$SCRIPT_DIR/compile_model.sh" ]; then
        print_error "compile_model.sh not found in scripts directory"
        return 1
    fi
    
    if [ ! -x "$SCRIPT_DIR/compile_model.sh" ]; then
        print_error "compile_model.sh is not executable"
        print_status "Run: chmod +x $SCRIPT_DIR/compile_model.sh"
        return 1
    fi
    
    print_status "Prerequisites check passed"
    return 0
}

# Main execution
main() {
    print_status "STM32N6 All Models Compilation Script"
    print_status "====================================="
    
    # Parse command line arguments
    local target_model=""
    local available_models=$(get_available_models)
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -m|--model)
                target_model="$2"
                # Validate model type
                if [[ ! " $available_models " =~ " $target_model " ]]; then
                    print_error "Unknown model type: $target_model"
                    print_error "Available models: $available_models"
                    exit 1
                fi
                shift 2
                ;;
            -l|--list)
                list_models
                exit 0
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
                print_error "Unexpected argument: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    print_status "Input models directory: $INPUT_MODELS_DIR"
    if [ -n "$target_model" ]; then
        print_status "Target model: $target_model"
    else
        print_status "Compiling all available models: $available_models"
    fi
    
    # Check prerequisites
    if ! check_prerequisites; then
        exit 1
    fi
    
    # Compilation tracking
    local models_compiled=0
    local models_failed=0
    local compilation_success=true
    
    # Determine which models to compile
    local models_to_compile
    if [ -n "$target_model" ]; then
        models_to_compile="$target_model"
    else
        models_to_compile="$available_models"
    fi
    
    # Compile models
    print_status "Models to compile: $models_to_compile"
    local model_count=0
    for model_type in $models_to_compile; do
        ((model_count++))
        print_status "[$model_count] Starting compilation of: $model_type"
        
        # Compile model with error handling
        local model_result=0
        compile_model "$model_type" || model_result=$?
        
        if [ $model_result -eq 0 ]; then
            ((models_compiled++))
            print_status "✅ [$model_count] Completed: $model_type"
        else
            ((models_failed++))
            compilation_success=false
            print_error "❌ [$model_count] Failed: $model_type (exit code: $model_result)"
        fi
        echo ""  # Add spacing between models
    done
    
    # Display results
    print_status "Compilation Summary:"
    print_status "==================="
    print_status "Models compiled successfully: $models_compiled"
    if [ $models_failed -gt 0 ]; then
        print_error "Models failed to compile: $models_failed"
    fi
    
    if [ "$compilation_success" = "true" ]; then
        print_status "🎉 All model compilations completed successfully!"
        print_status ""
        print_status "Next steps:"
        print_status "1. Build your STM32CubeIDE project in embedded/STM32CubeIDE/"
        print_status "2. Sign the application: ./scripts/sign_binary.sh (uses configured path)"
        print_status "3. Flash firmware: ./scripts/flash_firmware.sh all"
        
        # Show file locations
        print_status ""
        print_status "Generated files:"
        print_status "- Model C files: embedded/Models/"
        print_status "- Model binaries: embedded/Binary/"
        print_status "- Conversion logs: converted_models/"
    else
        print_error "❌ Some model compilations failed"
        print_error "Please check the error messages above and fix the issues"
        exit 1
    fi
}

# Execute main function
main "$@"