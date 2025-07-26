#!/bin/bash
# Convenience script to activate venv and run the UI

cd "$(dirname "$0")"

# Check if venv exists
if [ ! -d "venv" ]; then
    echo "Virtual environment not found. Creating..."
    python3 -m venv venv
    source venv/bin/activate
    pip install --upgrade pip
    pip install -r requirements.txt
else
    source venv/bin/activate
fi

echo "Virtual environment activated!"
echo "Python: $(which python)"
echo "Starting Face Recognition UI..."
echo ""

python face_recognition_ui.py