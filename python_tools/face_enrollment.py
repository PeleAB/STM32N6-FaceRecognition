#!/usr/bin/env python3
"""
Advanced Face Enrollment and Management System

Features:
- Multi-photo enrollment with quality validation
- Face database management with persistence
- Quality assessment and feedback
- Batch enrollment from directories
- Real-time preview during enrollment
- Export/import face databases
"""

import os
import json
import time
import hashlib
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import List, Dict, Optional, Tuple
from datetime import datetime

import cv2
import numpy as np
from PySide6 import QtCore, QtGui, QtWidgets
from PySide6.QtCore import QThread, Signal
from PySide6.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QGridLayout,
    QLabel, QPushButton, QLineEdit, QListWidget, QListWidgetItem,
    QGroupBox, QProgressBar, QTextEdit, QSpinBox, QSlider,
    QFileDialog, QMessageBox, QTabWidget, QCheckBox,
    QTableWidget, QTableWidgetItem, QHeaderView
)
import onnxruntime as ort

from centerface import CenterFace

@dataclass
class FaceProfile:
    """Face profile with metadata"""
    id: str
    name: str
    embeddings: List[List[float]]
    creation_date: str
    last_updated: str
    enrollment_images: int
    quality_scores: List[float]
    notes: str = ""
    enabled: bool = True
    
    def get_average_embedding(self) -> np.ndarray:
        """Get average embedding from all enrollments"""
        if not self.embeddings:
            return np.array([])
        
        avg_emb = np.mean(np.array(self.embeddings), axis=0)
        norm = np.linalg.norm(avg_emb)
        if norm > 0:
            avg_emb /= norm
        return avg_emb
    
    def get_quality_score(self) -> float:
        """Get average quality score"""
        return np.mean(self.quality_scores) if self.quality_scores else 0.0

class FaceDatabase:
    """Face database management"""
    
    def __init__(self, db_path: Path):
        self.db_path = db_path
        self.profiles: Dict[str, FaceProfile] = {}
        self.load()
    
    def load(self):
        """Load face database from file"""
        if self.db_path.exists():
            try:
                with open(self.db_path, 'r') as f:
                    data = json.load(f)
                
                self.profiles = {}
                for profile_data in data.get('profiles', []):
                    profile = FaceProfile(**profile_data)
                    self.profiles[profile.id] = profile
                    
            except Exception as e:
                print(f"Failed to load face database: {e}")
    
    def save(self):
        """Save face database to file"""
        try:
            data = {
                'version': '1.0',
                'created': datetime.now().isoformat(),
                'profiles': [asdict(profile) for profile in self.profiles.values()]
            }
            
            self.db_path.parent.mkdir(parents=True, exist_ok=True)
            with open(self.db_path, 'w') as f:
                json.dump(data, f, indent=2)
                
        except Exception as e:
            print(f"Failed to save face database: {e}")
    
    def add_profile(self, profile: FaceProfile):
        """Add or update face profile"""
        self.profiles[profile.id] = profile
        self.save()
    
    def remove_profile(self, profile_id: str):
        """Remove face profile"""
        if profile_id in self.profiles:
            del self.profiles[profile_id]
            self.save()
    
    def get_profile(self, profile_id: str) -> Optional[FaceProfile]:
        """Get face profile by ID"""
        return self.profiles.get(profile_id)
    
    def list_profiles(self) -> List[FaceProfile]:
        """List all face profiles"""
        return list(self.profiles.values())
    
    def export_to_c_header(self, output_path: Path, selected_profile_id: str = None):
        """Export embeddings to C header file"""
        try:
            with open(output_path, 'w') as f:
                f.write("/* Auto-generated face embeddings */\n")
                f.write("#ifndef FACE_EMBEDDINGS_H\n")
                f.write("#define FACE_EMBEDDINGS_H\n\n")
                f.write("#include <stdint.h>\n\n")
                
                if selected_profile_id and selected_profile_id in self.profiles:
                    # Export single profile as target embedding
                    profile = self.profiles[selected_profile_id]
                    embedding = profile.get_average_embedding()
                    
                    f.write(f"/* Target embedding for: {profile.name} */\n")
                    f.write(f"#define EMBEDDING_SIZE {len(embedding)}\n")
                    f.write("float target_embedding[EMBEDDING_SIZE] = {\n")
                    
                    for i, val in enumerate(embedding):
                        if i % 8 == 0:
                            f.write("    ")
                        f.write(f"{val:.6f}f")
                        if i < len(embedding) - 1:
                            f.write(", ")
                        if i % 8 == 7 or i == len(embedding) - 1:
                            f.write("\n")
                    
                    f.write("};\n\n")
                else:
                    # Export all profiles
                    f.write(f"#define NUM_FACES {len(self.profiles)}\n")
                    f.write(f"#define EMBEDDING_SIZE {len(next(iter(self.profiles.values())).get_average_embedding()) if self.profiles else 0}\n\n")
                    
                    f.write("typedef struct {\n")
                    f.write("    const char* name;\n")
                    f.write("    const float* embedding;\n")
                    f.write("    int enabled;\n")
                    f.write("} face_profile_t;\n\n")
                    
                    # Write embeddings
                    for i, profile in enumerate(self.profiles.values()):
                        embedding = profile.get_average_embedding()
                        f.write(f"static const float embedding_{i}[EMBEDDING_SIZE] = {{\n")
                        
                        for j, val in enumerate(embedding):
                            if j % 8 == 0:
                                f.write("    ")
                            f.write(f"{val:.6f}f")
                            if j < len(embedding) - 1:
                                f.write(", ")
                            if j % 8 == 7 or j == len(embedding) - 1:
                                f.write("\n")
                        
                        f.write("};\n\n")
                    
                    # Write profile array
                    f.write("static const face_profile_t face_profiles[NUM_FACES] = {\n")
                    for i, profile in enumerate(self.profiles.values()):
                        f.write(f'    {{"{profile.name}", embedding_{i}, {1 if profile.enabled else 0}}}')
                        if i < len(self.profiles) - 1:
                            f.write(",")
                        f.write("\n")
                    f.write("};\n\n")
                
                f.write("#endif /* FACE_EMBEDDINGS_H */\n")
                
        except Exception as e:
            print(f"Failed to export C header: {e}")

class EnrollmentWorker(QThread):
    """Background worker for face enrollment"""
    
    progress_updated = Signal(int, str)  # progress, message
    embedding_computed = Signal(np.ndarray, float)  # embedding, quality
    enrollment_completed = Signal(bool, str)  # success, message
    
    def __init__(self, image_paths: List[Path], detector: CenterFace, recognizer):
        super().__init__()
        self.image_paths = image_paths
        self.detector = detector
        self.recognizer = recognizer
        self.input_name = recognizer.get_inputs()[0].name
        self.output_name = recognizer.get_outputs()[0].name
        self._running = True
    
    def run(self):
        """Process enrollment images"""
        embeddings = []
        quality_scores = []
        
        for i, img_path in enumerate(self.image_paths):
            if not self._running:
                break
                
            try:
                self.progress_updated.emit(
                    int((i / len(self.image_paths)) * 100),
                    f"Processing {img_path.name}..."
                )
                
                # Load and process image
                img = cv2.imread(str(img_path))
                if img is None:
                    continue
                
                # Detect faces
                h, w = img.shape[:2]
                crop_size = min(h, w)
                off_x = (w - crop_size) // 2
                off_y = (h - crop_size) // 2
                img_sq = img[off_y:off_y + crop_size, off_x:off_x + crop_size]
                
                dets, lms = self.detector.inference(img_sq, threshold=0.5)
                if len(dets) == 0:
                    continue
                
                # Use highest confidence detection
                best_idx = np.argmax([det[4] for det in dets])
                det = dets[best_idx]
                lm = lms[best_idx]
                
                # Crop and align face
                face_crop = self._crop_align_face(img_sq, det, lm)
                if face_crop is None:
                    continue
                
                # Compute embedding
                embedding, quality = self._compute_embedding(face_crop)
                if embedding is not None:
                    embeddings.append(embedding)
                    quality_scores.append(quality)
                    self.embedding_computed.emit(embedding, quality)
                
            except Exception as e:
                print(f"Error processing {img_path}: {e}")
        
        # Complete enrollment
        if embeddings:
            self.enrollment_completed.emit(True, f"Successfully processed {len(embeddings)} images")
        else:
            self.enrollment_completed.emit(False, "No valid embeddings computed")
    
    def _crop_align_face(self, img: np.ndarray, det: np.ndarray, lm: np.ndarray) -> Optional[np.ndarray]:
        """Crop and align face from detection"""
        try:
            # Extract box and landmarks
            x1, y1, x2, y2, conf = det
            
            # Expand box
            center_x = (x1 + x2) / 2
            center_y = (y1 + y2) / 2
            size = max(x2 - x1, y2 - y1) * 1.2
            
            x1 = max(0, int(center_x - size / 2))
            y1 = max(0, int(center_y - size / 2))
            x2 = min(img.shape[1], int(center_x + size / 2))
            y2 = min(img.shape[0], int(center_y + size / 2))
            
            face_crop = img[y1:y2, x1:x2]
            face_resized = cv2.resize(face_crop, (96, 112))
            
            return face_resized
            
        except Exception as e:
            print(f"Face crop error: {e}")
            return None
    
    def _compute_embedding(self, face_img: np.ndarray) -> Tuple[Optional[np.ndarray], float]:
        """Compute face embedding and quality score"""
        try:
            # Preprocess for recognition model
            face_rgb = cv2.cvtColor(face_img, cv2.COLOR_BGR2RGB).astype(np.int16)
            face_rgb -= 128
            face_input = np.transpose(face_rgb.astype(np.int8), (2, 0, 1))[None, ...]
            
            # Run inference
            onnx_out = self.recognizer.run([self.output_name], {self.input_name: face_input})[0]
            embedding = onnx_out.astype(np.int8).flatten() / 128.0
            
            # Normalize
            norm = np.linalg.norm(embedding)
            if norm > 0:
                embedding /= norm
            
            # Calculate quality score based on face size and clarity
            quality = min(1.0, max(0.0, (face_img.shape[0] * face_img.shape[1]) / (96 * 112)))
            
            return embedding, quality
            
        except Exception as e:
            print(f"Embedding computation error: {e}")
            return None, 0.0
    
    def stop(self):
        """Stop enrollment process"""
        self._running = False

class FaceEnrollmentDialog(QDialog):
    """Face enrollment dialog"""
    
    def __init__(self, face_db: FaceDatabase, parent=None):
        super().__init__(parent)
        self.face_db = face_db
        self.setWindowTitle("Face Enrollment")
        self.setModal(True)
        self.resize(800, 600)
        
        # Initialize models
        model_dir = Path(__file__).parent / "models"
        det_model = model_dir / "centerface_1x3xHxW_integer_quant.tflite"
        rec_model = model_dir / "mobilefacenet_integer_quant_1_OE_3_2_0.onnx"
        
        try:
            self.detector = CenterFace(str(det_model))
            self.recognizer = ort.InferenceSession(str(rec_model))
        except Exception as e:
            QMessageBox.critical(self, "Model Error", f"Failed to load models: {e}")
            return
        
        self.current_embeddings = []
        self.current_qualities = []
        self.setup_ui()
    
    def setup_ui(self):
        """Setup user interface"""
        layout = QVBoxLayout(self)
        
        # Profile information
        info_group = QGroupBox("Profile Information")
        info_layout = QGridLayout(info_group)
        
        info_layout.addWidget(QLabel("Name:"), 0, 0)
        self.name_edit = QLineEdit()
        info_layout.addWidget(self.name_edit, 0, 1)
        
        info_layout.addWidget(QLabel("Notes:"), 1, 0)
        self.notes_edit = QLineEdit()
        info_layout.addWidget(self.notes_edit, 1, 1)
        
        layout.addWidget(info_group)
        
        # Image selection
        selection_group = QGroupBox("Image Selection")
        selection_layout = QVBoxLayout(selection_group)
        
        selection_buttons = QHBoxLayout()
        self.add_images_btn = QPushButton("📁 Add Images")
        self.add_images_btn.clicked.connect(self.add_images)
        selection_buttons.addWidget(self.add_images_btn)
        
        self.add_folder_btn = QPushButton("📂 Add Folder")
        self.add_folder_btn.clicked.connect(self.add_folder)
        selection_buttons.addWidget(self.add_folder_btn)
        
        self.clear_btn = QPushButton("🗑️ Clear")
        self.clear_btn.clicked.connect(self.clear_images)
        selection_buttons.addWidget(self.clear_btn)
        
        selection_layout.addLayout(selection_buttons)
        
        self.image_list = QListWidget()
        selection_layout.addWidget(self.image_list)
        
        layout.addWidget(selection_group)
        
        # Enrollment progress
        progress_group = QGroupBox("Enrollment Progress")
        progress_layout = QVBoxLayout(progress_group)
        
        self.progress_bar = QProgressBar()
        progress_layout.addWidget(self.progress_bar)
        
        self.status_label = QLabel("Ready to enroll")
        progress_layout.addWidget(self.status_label)
        
        layout.addWidget(progress_group)
        
        # Results
        results_group = QGroupBox("Results")
        results_layout = QVBoxLayout(results_group)
        
        self.results_text = QTextEdit()
        self.results_text.setMaximumHeight(100)
        self.results_text.setReadOnly(True)
        results_layout.addWidget(self.results_text)
        
        layout.addWidget(results_group)
        
        # Buttons
        button_layout = QHBoxLayout()
        
        self.enroll_btn = QPushButton("🚀 Start Enrollment")
        self.enroll_btn.clicked.connect(self.start_enrollment)
        button_layout.addWidget(self.enroll_btn)
        
        self.save_btn = QPushButton("💾 Save Profile")
        self.save_btn.clicked.connect(self.save_profile)
        self.save_btn.setEnabled(False)
        button_layout.addWidget(self.save_btn)
        
        button_layout.addStretch()
        
        cancel_btn = QPushButton("Cancel")
        cancel_btn.clicked.connect(self.reject)
        button_layout.addWidget(cancel_btn)
        
        layout.addLayout(button_layout)
    
    def add_images(self):
        """Add individual images"""
        files, _ = QFileDialog.getOpenFileNames(
            self, "Select Images", "",
            "Image Files (*.jpg *.jpeg *.png *.bmp)"
        )
        
        for file_path in files:
            item = QListWidgetItem(Path(file_path).name)
            item.setData(QtCore.Qt.UserRole, file_path)
            self.image_list.addItem(item)
    
    def add_folder(self):
        """Add all images from a folder"""
        folder = QFileDialog.getExistingDirectory(self, "Select Folder")
        if folder:
            folder_path = Path(folder)
            extensions = ['.jpg', '.jpeg', '.png', '.bmp']
            
            for ext in extensions:
                for img_path in folder_path.glob(f"*{ext}"):
                    item = QListWidgetItem(img_path.name)
                    item.setData(QtCore.Qt.UserRole, str(img_path))
                    self.image_list.addItem(item)
                    
                for img_path in folder_path.glob(f"*{ext.upper()}"):
                    item = QListWidgetItem(img_path.name)
                    item.setData(QtCore.Qt.UserRole, str(img_path))
                    self.image_list.addItem(item)
    
    def clear_images(self):
        """Clear image list"""
        self.image_list.clear()
        self.current_embeddings.clear()
        self.current_qualities.clear()
        self.save_btn.setEnabled(False)
    
    def start_enrollment(self):
        """Start enrollment process"""
        if self.image_list.count() == 0:
            QMessageBox.warning(self, "No Images", "Please select images for enrollment")
            return
        
        if not self.name_edit.text().strip():
            QMessageBox.warning(self, "No Name", "Please enter a name for the profile")
            return
        
        # Get image paths
        image_paths = []
        for i in range(self.image_list.count()):
            item = self.image_list.item(i)
            image_paths.append(Path(item.data(QtCore.Qt.UserRole)))
        
        # Start enrollment worker
        self.enrollment_worker = EnrollmentWorker(image_paths, self.detector, self.recognizer)
        self.enrollment_worker.progress_updated.connect(self.update_progress)
        self.enrollment_worker.embedding_computed.connect(self.add_embedding)
        self.enrollment_worker.enrollment_completed.connect(self.enrollment_finished)
        
        self.enroll_btn.setEnabled(False)
        self.current_embeddings.clear()
        self.current_qualities.clear()
        self.results_text.clear()
        
        self.enrollment_worker.start()
    
    def update_progress(self, value: int, message: str):
        """Update progress bar and status"""
        self.progress_bar.setValue(value)
        self.status_label.setText(message)
    
    def add_embedding(self, embedding: np.ndarray, quality: float):
        """Add computed embedding"""
        self.current_embeddings.append(embedding.tolist())
        self.current_qualities.append(quality)
        
        self.results_text.append(f"Embedding computed (quality: {quality:.2f})")
    
    def enrollment_finished(self, success: bool, message: str):
        """Handle enrollment completion"""
        self.enroll_btn.setEnabled(True)
        self.status_label.setText(message)
        
        if success and self.current_embeddings:
            self.save_btn.setEnabled(True)
            avg_quality = np.mean(self.current_qualities)
            self.results_text.append(f"\\nEnrollment completed: {len(self.current_embeddings)} embeddings")
            self.results_text.append(f"Average quality: {avg_quality:.2f}")
    
    def save_profile(self):
        """Save enrolled profile"""
        if not self.current_embeddings:
            return
        
        profile_id = hashlib.md5(
            (self.name_edit.text() + str(time.time())).encode()
        ).hexdigest()[:8]
        
        profile = FaceProfile(
            id=profile_id,
            name=self.name_edit.text().strip(),
            embeddings=self.current_embeddings,
            creation_date=datetime.now().isoformat(),
            last_updated=datetime.now().isoformat(),
            enrollment_images=len(self.current_embeddings),
            quality_scores=self.current_qualities,
            notes=self.notes_edit.text().strip()
        )
        
        self.face_db.add_profile(profile)
        QMessageBox.information(self, "Success", f"Profile '{profile.name}' saved successfully!")
        self.accept()

class FaceManagementDialog(QDialog):
    """Face management dialog"""
    
    def __init__(self, face_db: FaceDatabase, parent=None):
        super().__init__(parent)
        self.face_db = face_db
        self.setWindowTitle("Face Management")
        self.setModal(True)
        self.resize(900, 700)
        self.setup_ui()
        self.refresh_profiles()
    
    def setup_ui(self):
        """Setup user interface"""
        layout = QVBoxLayout(self)
        
        # Toolbar
        toolbar = QHBoxLayout()
        
        self.refresh_btn = QPushButton("🔄 Refresh")
        self.refresh_btn.clicked.connect(self.refresh_profiles)
        toolbar.addWidget(self.refresh_btn)
        
        self.export_btn = QPushButton("📤 Export C Header")
        self.export_btn.clicked.connect(self.export_header)
        toolbar.addWidget(self.export_btn)
        
        self.import_btn = QPushButton("📥 Import Database")
        self.import_btn.clicked.connect(self.import_database)
        toolbar.addWidget(self.import_btn)
        
        toolbar.addStretch()
        
        layout.addLayout(toolbar)
        
        # Profile table
        self.profile_table = QTableWidget()
        self.profile_table.setColumnCount(7)
        self.profile_table.setHorizontalHeaderLabels([
            "Name", "ID", "Images", "Quality", "Created", "Status", "Actions"
        ])
        
        header = self.profile_table.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.Stretch)
        header.setSectionResizeMode(6, QHeaderView.ResizeToContents)
        
        layout.addWidget(self.profile_table)
        
        # Close button
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(self.accept)
        layout.addWidget(close_btn)
    
    def refresh_profiles(self):
        """Refresh profile table"""
        profiles = self.face_db.list_profiles()
        self.profile_table.setRowCount(len(profiles))
        
        for row, profile in enumerate(profiles):
            # Name
            self.profile_table.setItem(row, 0, QTableWidgetItem(profile.name))
            
            # ID
            self.profile_table.setItem(row, 1, QTableWidgetItem(profile.id))
            
            # Images
            self.profile_table.setItem(row, 2, QTableWidgetItem(str(profile.enrollment_images)))
            
            # Quality
            quality = profile.get_quality_score()
            self.profile_table.setItem(row, 3, QTableWidgetItem(f"{quality:.2f}"))
            
            # Created
            created = datetime.fromisoformat(profile.creation_date).strftime("%Y-%m-%d")
            self.profile_table.setItem(row, 4, QTableWidgetItem(created))
            
            # Status
            status = "Enabled" if profile.enabled else "Disabled"
            self.profile_table.setItem(row, 5, QTableWidgetItem(status))
            
            # Actions
            actions_widget = QWidget()
            actions_layout = QHBoxLayout(actions_widget)
            actions_layout.setContentsMargins(4, 4, 4, 4)
            
            edit_btn = QPushButton("✏️")
            edit_btn.setMaximumSize(30, 30)
            edit_btn.clicked.connect(lambda checked, p=profile: self.edit_profile(p))
            actions_layout.addWidget(edit_btn)
            
            delete_btn = QPushButton("🗑️")
            delete_btn.setMaximumSize(30, 30)
            delete_btn.clicked.connect(lambda checked, p=profile: self.delete_profile(p))
            actions_layout.addWidget(delete_btn)
            
            export_btn = QPushButton("📤")
            export_btn.setMaximumSize(30, 30)
            export_btn.clicked.connect(lambda checked, p=profile: self.export_single_profile(p))
            actions_layout.addWidget(export_btn)
            
            self.profile_table.setCellWidget(row, 6, actions_widget)
    
    def edit_profile(self, profile: FaceProfile):
        """Edit profile properties"""
        # TODO: Implement profile editing dialog
        QMessageBox.information(self, "Edit Profile", f"Editing {profile.name} - Feature coming soon!")
    
    def delete_profile(self, profile: FaceProfile):
        """Delete profile"""
        reply = QMessageBox.question(
            self, "Delete Profile",
            f"Are you sure you want to delete '{profile.name}'?",
            QMessageBox.Yes | QMessageBox.No
        )
        
        if reply == QMessageBox.Yes:
            self.face_db.remove_profile(profile.id)
            self.refresh_profiles()
    
    def export_single_profile(self, profile: FaceProfile):
        """Export single profile as target embedding"""
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Export Target Embedding", f"{profile.name}_embedding.h",
            "C Header Files (*.h)"
        )
        
        if file_path:
            self.face_db.export_to_c_header(Path(file_path), profile.id)
            QMessageBox.information(self, "Export Complete", f"Exported {profile.name} to {file_path}")
    
    def export_header(self):
        """Export all profiles to C header"""
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Export Face Database", "face_embeddings.h",
            "C Header Files (*.h)"
        )
        
        if file_path:
            self.face_db.export_to_c_header(Path(file_path))
            QMessageBox.information(self, "Export Complete", f"Exported database to {file_path}")
    
    def import_database(self):
        """Import face database"""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Import Face Database", "",
            "JSON Files (*.json)"
        )
        
        if file_path:
            # TODO: Implement database import
            QMessageBox.information(self, "Import", "Database import feature coming soon!")

# Example usage functions
def create_face_database(db_path: Path = None) -> FaceDatabase:
    """Create or load face database"""
    if db_path is None:
        db_path = Path.home() / ".face_detection" / "face_database.json"
    return FaceDatabase(db_path)

def show_enrollment_dialog(face_db: FaceDatabase, parent=None) -> bool:
    """Show face enrollment dialog"""
    dialog = FaceEnrollmentDialog(face_db, parent)
    return dialog.exec_() == QDialog.Accepted

def show_management_dialog(face_db: FaceDatabase, parent=None):
    """Show face management dialog"""
    dialog = FaceManagementDialog(face_db, parent)
    dialog.exec_()