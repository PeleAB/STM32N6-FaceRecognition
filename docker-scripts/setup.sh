#!/bin/bash
# STM32N6 Face Recognition System - Docker Setup Script

set -e

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

# Check if Docker is installed
check_docker() {
    if ! command -v docker &> /dev/null; then
        print_error "Docker is not installed. Please install Docker first."
        echo "Installation instructions: https://docs.docker.com/get-docker/"
        exit 1
    fi
    
    if ! command -v docker-compose &> /dev/null; then
        print_error "Docker Compose is not installed. Please install Docker Compose first."
        echo "Installation instructions: https://docs.docker.com/compose/install/"
        exit 1
    fi
    
    print_status "Docker and Docker Compose are available"
}

# Check Docker daemon
check_docker_daemon() {
    if ! docker info &> /dev/null; then
        print_error "Docker daemon is not running. Please start Docker first."
        exit 1
    fi
    print_status "Docker daemon is running"
}

# Set up environment variables
setup_environment() {
    print_status "Setting up environment variables..."
    
    # Create .env file if it doesn't exist
    if [ ! -f .env ]; then
        cat > .env << EOF
# Docker Environment Configuration
USER_ID=$(id -u)
GROUP_ID=$(id -g)
DISPLAY=${DISPLAY:-:0}

# Python tools configuration
PYTHON_HOST=0.0.0.0
PYTHON_PORT=8080

# Jupyter configuration
JUPYTER_PORT=8888
JUPYTER_TOKEN=stm32n6-dev

# Documentation server
DOCS_PORT=8090
EOF
        print_status "Created .env file with default configuration"
    else
        print_status ".env file already exists"
    fi
}

# Download STM32CubeProgrammer (optional)
download_stm32cubeprog() {
    print_warning "STM32CubeProgrammer download requires manual action due to license agreement"
    echo "Please download STM32CubeProgrammer from:"
    echo "https://www.st.com/en/development-tools/stm32cubeprog.html"
    echo ""
    echo "After downloading, place the zip file in this directory and run:"
    echo "  mv en.stm32cubeprog-*.zip stm32cubeprog.zip"
    echo ""
    echo "This will enable firmware flashing capabilities in the Docker environment."
}

# Build Docker images
build_images() {
    print_status "Building Docker images..."
    
    # Check available disk space
    available_space=$(df . | tail -1 | awk '{print $4}')
    required_space=5000000  # 5GB in KB
    
    if [ "$available_space" -lt "$required_space" ]; then
        print_warning "Low disk space detected. Docker build requires at least 5GB free space."
        read -p "Continue anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
    
    # Build development environment
    print_status "Building STM32 development environment..."
    docker-compose build development
    
    # Build Python tools
    print_status "Building Python tools environment..."
    docker-compose build python-tools
    
    print_status "Docker images built successfully"
}

# Create convenience scripts
create_scripts() {
    print_status "Creating convenience scripts..."
    
    mkdir -p docker-scripts
    
    # Development environment script
    cat > docker-scripts/dev.sh << 'EOF'
#!/bin/bash
# Start STM32N6 development environment
echo "Starting STM32N6 development environment..."
docker-compose run --rm development
EOF
    chmod +x docker-scripts/dev.sh
    
    # Build firmware script
    cat > docker-scripts/build.sh << 'EOF'
#!/bin/bash
# Build STM32N6 firmware
echo "Building STM32N6 firmware..."
docker-compose run --rm stm32-build
EOF
    chmod +x docker-scripts/build.sh
    
    # Python tools script
    cat > docker-scripts/python-ui.sh << 'EOF'
#!/bin/bash
# Start Python tools UI
echo "Starting Python tools UI..."
echo "Access the UI at: http://localhost:8080"
docker-compose up python-tools
EOF
    chmod +x docker-scripts/python-ui.sh
    
    # Jupyter notebook script
    cat > docker-scripts/jupyter.sh << 'EOF'
#!/bin/bash
# Start Jupyter notebook server
echo "Starting Jupyter Lab server..."
echo "Access Jupyter at: http://localhost:8888"
docker-compose up jupyter
EOF
    chmod +x docker-scripts/jupyter.sh
    
    # Documentation server script
    cat > docker-scripts/docs.sh << 'EOF'
#!/bin/bash
# Start documentation server
echo "Starting documentation server..."
echo "Access documentation at: http://localhost:8090"
docker-compose up docs
EOF
    chmod +x docker-scripts/docs.sh
    
    # CI/CD script
    cat > docker-scripts/ci.sh << 'EOF'
#!/bin/bash
# Run CI/CD pipeline
echo "Running CI/CD pipeline..."
docker-compose run --rm ci-cd
EOF
    chmod +x docker-scripts/ci.sh
    
    # Clean up script
    cat > docker-scripts/clean.sh << 'EOF'
#!/bin/bash
# Clean up Docker resources
echo "Cleaning up Docker resources..."
docker-compose down -v
docker system prune -f
echo "Cleanup completed"
EOF
    chmod +x docker-scripts/clean.sh
    
    print_status "Created convenience scripts in docker-scripts/"
}

# Setup USB permissions for STM32 programming
setup_usb_permissions() {
    print_status "Checking USB permissions for STM32 programming..."
    
    if [ -f /etc/udev/rules.d/99-stlink-v2.rules ]; then
        print_status "STM32 USB rules already configured"
    else
        print_warning "STM32 USB rules not found"
        echo "To enable STM32 programming, add these udev rules:"
        echo ""
        cat << 'EOF'
# STM32 Discovery boards
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374a", MODE="0666", GROUP="plugdev"
# STM32N6570-DK
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374e", MODE="0666", GROUP="plugdev"
EOF
        echo ""
        echo "Save these rules to /etc/udev/rules.d/99-stlink-v2.rules and run:"
        echo "  sudo udevadm control --reload-rules"
        echo "  sudo udevadm trigger"
    fi
}

# Main setup function
main() {
    echo "======================================"
    echo "STM32N6 Face Recognition System"
    echo "Docker Development Environment Setup"
    echo "======================================"
    echo ""
    
    check_docker
    check_docker_daemon
    setup_environment
    download_stm32cubeprog
    build_images
    create_scripts
    setup_usb_permissions
    
    echo ""
    print_status "Setup completed successfully!"
    echo ""
    echo "Quick start commands:"
    echo "  ./docker-scripts/dev.sh          - Start development environment"
    echo "  ./docker-scripts/build.sh        - Build firmware"
    echo "  ./docker-scripts/python-ui.sh    - Start Python tools UI"
    echo "  ./docker-scripts/jupyter.sh      - Start Jupyter Lab"
    echo "  ./docker-scripts/docs.sh         - Start documentation server"
    echo ""
    echo "For more information, see README.md"
}

# Run main function
main "$@"