#!/usr/bin/env python3
"""
Simple UI Test - Minimal version for testing basic functionality
"""

import sys
from pathlib import Path

try:
    from PySide6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget, QLabel, QPushButton
    from PySide6.QtGui import QAction
    from PySide6.QtCore import Qt
except ImportError:
    print("PySide6 not available, please install with: pip install PySide6")
    sys.exit(1)

class SimpleTestWindow(QMainWindow):
    """Simple test window to verify PySide6 functionality"""
    
    def __init__(self):
        super().__init__()
        self.setWindowTitle("STM32N6 Object Detection - UI Test")
        self.setMinimumSize(400, 300)
        
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Layout
        layout = QVBoxLayout(central_widget)
        
        # Test label
        label = QLabel("UI Test - Basic Functionality Working!")
        label.setAlignment(Qt.AlignCenter)
        layout.addWidget(label)
        
        # Test button
        button = QPushButton("Test Button")
        button.clicked.connect(self.button_clicked)
        layout.addWidget(button)
        
        # Create menu
        self.create_menu()
        
        # Set dark theme
        self.setStyleSheet("""
            QMainWindow {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QLabel {
                font-size: 16px;
                padding: 20px;
            }
            QPushButton {
                background-color: #404040;
                border: 1px solid #555;
                border-radius: 4px;
                padding: 8px;
                font-size: 14px;
            }
            QPushButton:hover {
                background-color: #505050;
            }
        """)
    
    def create_menu(self):
        """Create test menu"""
        menubar = self.menuBar()
        
        # File menu
        file_menu = menubar.addMenu("File")
        
        test_action = QAction(self)
        test_action.setText("Test Action")
        test_action.triggered.connect(self.test_action_triggered)
        file_menu.addAction(test_action)
        
        exit_action = QAction(self)
        exit_action.setText("Exit")
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
    
    def button_clicked(self):
        """Handle button click"""
        print("Button clicked - UI is working!")
    
    def test_action_triggered(self):
        """Handle test action"""
        print("Menu action triggered - Menus are working!")

def main():
    """Main test function"""
    print("Starting Simple UI Test...")
    
    app = QApplication(sys.argv)
    app.setApplicationName("STM32N6 Object Detection UI Test")
    
    window = SimpleTestWindow()
    window.show()
    
    print("UI Test window opened successfully!")
    print("If you see this message and a window appeared, basic UI functionality is working.")
    
    sys.exit(app.exec())

if __name__ == "__main__":
    main()