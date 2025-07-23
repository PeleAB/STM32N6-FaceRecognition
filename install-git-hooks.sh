#!/bin/bash
# STM32N6 Face Recognition System - Git Hooks Installation Script

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in a git repository
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    print_error "Not in a git repository. Initialize git first with: git init"
    exit 1
fi

print_status "Installing git hooks for STM32N6 Face Recognition System..."

# Get the git hooks directory
GIT_HOOKS_DIR=$(git rev-parse --git-dir)/hooks
PROJECT_HOOKS_DIR=".git-hooks"

if [ ! -d "$PROJECT_HOOKS_DIR" ]; then
    print_error "Project hooks directory not found: $PROJECT_HOOKS_DIR"
    exit 1
fi

# Install pre-commit hook
if [ -f "$PROJECT_HOOKS_DIR/pre-commit" ]; then
    print_status "Installing pre-commit hook..."
    cp "$PROJECT_HOOKS_DIR/pre-commit" "$GIT_HOOKS_DIR/pre-commit"
    chmod +x "$GIT_HOOKS_DIR/pre-commit"
    print_status "Pre-commit hook installed successfully"
else
    print_warning "Pre-commit hook not found in $PROJECT_HOOKS_DIR"
fi

# Check for required tools
print_status "Checking for required tools..."

TOOLS_MISSING=false

if ! command -v clang-format >/dev/null 2>&1; then
    print_warning "clang-format not found"
    echo "  Install with: apt-get install clang-format"
    echo "  Or on macOS: brew install clang-format"
    TOOLS_MISSING=true
else
    print_status "clang-format found: $(clang-format --version | head -1)"
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
    print_warning "clang-tidy not found"
    echo "  Install with: apt-get install clang-tidy"
    echo "  Or on macOS: brew install llvm"
    TOOLS_MISSING=true
else
    print_status "clang-tidy found: $(clang-tidy --version | head -1)"
fi

if [ "$TOOLS_MISSING" = true ]; then
    echo ""
    print_warning "Some tools are missing. Git hooks will skip checks for missing tools."
    echo "For full functionality, install the missing tools."
fi

echo ""
print_status "Git hooks installation completed!"
echo ""
echo "Available hooks:"
echo "  pre-commit  - Checks code formatting and runs static analysis"
echo ""
echo "You can also run checks manually:"
echo "  make format       - Format code"
echo "  make format-check - Check formatting"
echo "  make analyze      - Run static analysis"
echo "  make check        - Run all checks"
echo ""
echo "To bypass hooks temporarily: git commit --no-verify"

exit 0