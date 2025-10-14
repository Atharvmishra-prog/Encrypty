import sys
import subprocess
import os
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QFileDialog, QTextEdit, QComboBox, QLineEdit,
    QListWidget, QListWidgetItem, QStackedWidget, QSplitter
)
from PyQt5.QtCore import Qt


class HomeWidget(QWidget):
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout()

        title = QLabel("Parallel File Encryptor/Decryptor")
        title.setStyleSheet("font-size: 28px; font-weight: bold;")
        layout.addWidget(title)

        team = QLabel("Created by B.Tech 5th Semester Students:")
        members = QLabel("\n".join([
            "- Atharv Mishra",
            "- Ayush Rana",
            "- Siddarth Rawat"
        ]))
        about = QLabel("""\nAbout:\nThis tool enables parallel encryption/decryption of more than 1000+ files using multiprocessing/multithreading.\nFor large files (20GB, 30GB), the Single File mode provides efficient processing with memory-mapped files.""")

        for widget in [team, members, about]:
            widget.setStyleSheet("font-size: 18px;")
            layout.addWidget(widget)

        layout.addStretch()
        self.setLayout(layout)

class SingleFileWidget(QWidget):
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout()

        file_layout = QHBoxLayout()
        self.file_input = QLineEdit()
        self.file_input.setPlaceholderText("Select single file")
        file_browse = QPushButton("Browse")
        file_browse.clicked.connect(self.browse_file)
        file_layout.addWidget(QLabel("File:"))
        file_layout.addWidget(self.file_input)
        file_layout.addWidget(file_browse)

        password_layout = QHBoxLayout()
        self.password_input = QLineEdit()
        self.password_input.setEchoMode(QLineEdit.Password)
        self.password_input.setPlaceholderText("Enter password")
        password_layout.addWidget(QLabel("Password:"))
        password_layout.addWidget(self.password_input)

        action_layout = QHBoxLayout()
        self.action_combo = QComboBox()
        self.action_combo.addItems(["encrypt", "decrypt"])
        action_layout.addWidget(QLabel("Action:"))
        action_layout.addWidget(self.action_combo)

        run_button = QPushButton("Run")
        run_button.clicked.connect(self.run_process)

        self.output_box = QTextEdit()
        self.output_box.setReadOnly(True)
        self.output_box.setFixedHeight(250)

        layout.addLayout(file_layout)
        layout.addLayout(password_layout)
        layout.addLayout(action_layout)
        layout.addWidget(run_button)
        layout.addWidget(QLabel("Output:"))
        layout.addWidget(self.output_box)
        layout.addStretch()
        self.setLayout(layout)

    def browse_file(self):
        file, _ = QFileDialog.getOpenFileName(self, "Select File")
        if file:
            self.file_input.setText(file)

    def run_process(self):
        file = self.file_input.text().strip()
        password = self.password_input.text().strip()
        action = self.action_combo.currentText().strip()

        if not file or not password:
            self.output_box.append("❌ File and Password are required.")
            return

        try:
            # Get the directory where this script is located
            script_dir = os.path.dirname(os.path.abspath(__file__))
            mmap_path = os.path.join(script_dir, 'mmap')
            
            process = subprocess.Popen([
                mmap_path
            ], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

            stdout, stderr = process.communicate(input=f"{file}\n{action}\n{password}\n")

            if stdout:
                self.output_box.append("✅ Output:\n" + stdout)
            if stderr:
                self.output_box.append("⚠️ Errors:\n" + stderr)
        except FileNotFoundError:
            self.output_box.append("❌ mmap binary not found.")

class MultiFileWidget(QWidget):
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout()

        folder_layout = QHBoxLayout()
        self.folder_input = QLineEdit()
        self.folder_input.setPlaceholderText("Select folder")
        folder_browse = QPushButton("Browse")
        folder_browse.clicked.connect(self.browse_folder)
        folder_layout.addWidget(QLabel("Folder:"))
        folder_layout.addWidget(self.folder_input)
        folder_layout.addWidget(folder_browse)

        password_layout = QHBoxLayout()
        self.password_input = QLineEdit()
        self.password_input.setEchoMode(QLineEdit.Password)
        self.password_input.setPlaceholderText("Enter password")
        password_layout.addWidget(QLabel("Password:"))
        password_layout.addWidget(self.password_input)

        method_layout = QHBoxLayout()
        self.method_combo = QComboBox()
        self.method_combo.addItems(["Multithreading", "Multiprocessing"])
        method_layout.addWidget(QLabel("Method:"))
        method_layout.addWidget(self.method_combo)

        action_layout = QHBoxLayout()
        self.action_combo = QComboBox()
        self.action_combo.addItems(["encrypt", "decrypt"])
        action_layout.addWidget(QLabel("Action:"))
        action_layout.addWidget(self.action_combo)

        run_button = QPushButton("Run")
        run_button.clicked.connect(self.run_process)

        self.output_box = QTextEdit()
        self.output_box.setReadOnly(True)
        self.output_box.setFixedHeight(250)

        layout.addLayout(folder_layout)
        layout.addLayout(password_layout)
        layout.addLayout(method_layout)
        layout.addLayout(action_layout)
        layout.addWidget(run_button)
        layout.addWidget(QLabel("Output:"))
        layout.addWidget(self.output_box)
        layout.addStretch()
        self.setLayout(layout)

    def browse_folder(self):
        folder = QFileDialog.getExistingDirectory(self, "Select Folder")
        if folder:
            self.folder_input.setText(folder)

    def run_process(self):
        folder = self.folder_input.text().strip()
        password = self.password_input.text().strip()
        method = self.method_combo.currentText().strip()
        action = self.action_combo.currentText().strip()

        if not folder or not password:
            self.output_box.append("❌ Folder and Password are required.")
            return

        try:
            # Get the directory where this script is located
            script_dir = os.path.dirname(os.path.abspath(__file__))
            simple_multifile_path = os.path.join(script_dir, 'simple_multifile.py')
            
            method_flag = 'thread' if method.lower().startswith('multi') and 'thread' in method.lower() else 'process'
            if method.lower().startswith('multi'):
                # Map exact labels to flags
                if method.lower() == 'multithreading':
                    method_flag = 'thread'
                elif method.lower() == 'multiprocessing':
                    method_flag = 'process'
            
            process = subprocess.Popen([
                'python3', simple_multifile_path, folder, action, password, method_flag
            ], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

            stdout, stderr = process.communicate()

            if stdout:
                self.output_box.append("✅ Output:\n" + stdout)
            if stderr:
                self.output_box.append("⚠️ Errors:\n" + stderr)
        except FileNotFoundError:
            self.output_box.append("❌ simple_multifile.py not found.")

class EncryptorApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Parallel File Encryptor-Decrpytor")
        self.showMaximized()

        splitter = QSplitter()
        sidebar = QListWidget()
        sidebar.setFixedWidth(200)
        sidebar.addItems(["Home", "Single File", "Multi File"])

        self.stack = QStackedWidget()
        self.home_widget = HomeWidget()
        self.single_widget = SingleFileWidget()
        self.multi_widget = MultiFileWidget()

        self.stack.addWidget(self.home_widget)
        self.stack.addWidget(self.single_widget)
        self.stack.addWidget(self.multi_widget)

        sidebar.currentRowChanged.connect(self.stack.setCurrentIndex)

        splitter.addWidget(sidebar)
        splitter.addWidget(self.stack)

        main_layout = QVBoxLayout()
        main_layout.addWidget(splitter)
        self.setLayout(main_layout)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyleSheet("""
        QWidget {
            background-color: #f4f4f4;
            font-family: 'Segoe UI', sans-serif;
            font-size: 14px;
        }

        QLabel {
            color: #333;
        }

        QLineEdit, QTextEdit, QComboBox {
            background-color: #ffffff;
            color: #111;  /* Ensure text is visible on light background */
            border: 1px solid #ccc;
            border-radius: 6px;
            padding: 6px;
        }

        QLineEdit::placeholder {
            color: #777;  /* Make placeholder text readable */
        }

        QPushButton {
            background-color: #4a90e2;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
        }

        QPushButton:hover {
            background-color: #357ab8;
        }

        QListWidget {
            background-color: #2c3e50;
            color: white;
            border: none;
            font-size: 20px;
            padding: 12px;
        }

        QListWidget::item {
            padding: 20px 10px;
            margin-bottom: 10px;
            border-radius: 8px;
        }

        QListWidget::item:selected {
            background-color: #1abc9c;
            color: black;
        }

        QComboBox QAbstractItemView {
            background-color: white;
            color: #111;  /* Ensure options text is visible */
            selection-background-color: #4a90e2;
            selection-color: white;
        }

        QComboBox QAbstractItemView::item {
            padding: 10px;
            border-radius: 4px;
            background-color: #f7f7f7;
        }

        QComboBox QAbstractItemView::item:hover {
            background-color: #e0e0e0;  /* Light gray for hover effect */
        }

        QComboBox QAbstractItemView::item:selected {
            background-color: #4a90e2;
            color: white;
        }

        QTextEdit {
            background-color: #fafafa;
            font-family: Consolas, monospace;
            color: #444;
        }

        QSplitter::handle {
            background-color: #ccc;
        }
    """)



    window = EncryptorApp()
    window.show()
    sys.exit(app.exec_())