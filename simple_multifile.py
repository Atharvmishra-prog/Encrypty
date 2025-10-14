#!/usr/bin/env python3
import os
import subprocess
import sys
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor, as_completed
from typing import List

def _encrypt_one_file(file_path: Path, directory: Path, password: str, mmap_binary: Path) -> bool:
    # Create a temporary directory for this file's encryption
    temp_dir = directory / f"temp_{file_path.stem}"
    temp_dir.mkdir(exist_ok=True)

    try:
        # Copy the file to temp directory
        temp_file = temp_dir / file_path.name
        import shutil
        shutil.copy2(file_path, temp_file)

        # Run encryption in temp directory
        result = subprocess.run(
            [str(mmap_binary)],
            input=f"{temp_file}\nencrypt\n{password}\n",
            text=True,
            capture_output=True,
            timeout=30
        )

        if result.returncode == 0:
            # Move the encrypted file (mid.txt) to the main directory with a unique name
            encrypted_file = temp_dir / "mid.txt"
            if encrypted_file.exists():
                final_encrypted = directory / f"{file_path.stem}_encrypted.txt"
                shutil.move(str(encrypted_file), str(final_encrypted))
                return True
            return False
        return False
    finally:
        # Clean up temp directory
        import shutil
        shutil.rmtree(temp_dir, ignore_errors=True)


def encrypt_multifile(directory, password, use_multithreading=True):
    """Simple multi-file encryption using the mmap binary"""
    directory = Path(directory)
    if not directory.exists():
        print(f"❌ Directory {directory} does not exist")
        return False
    
    # Get all .txt files in the directory (excluding .password.txt, mid.txt, out.txt, and already encrypted files)
    txt_files = [f for f in directory.glob("*.txt") if f.name not in [".password.txt", "mid.txt", "out.txt"] and not f.name.endswith("_encrypted.txt")]
    if not txt_files:
        print(f"❌ No .txt files found in {directory}")
        return False
    
    print(f"📁 Found {len(txt_files)} files to process")
    
    # Create password file
    password_file = directory / ".password.txt"
    with open(password_file, 'w') as f:
        f.write(password)
    
    # Get the script directory
    script_dir = Path(__file__).parent
    mmap_binary = script_dir / "mmap"
    
    if not mmap_binary.exists():
        print(f"❌ mmap binary not found at {mmap_binary}")
        return False
    
    executor_cls = ThreadPoolExecutor if use_multithreading else ProcessPoolExecutor
    max_workers = os.cpu_count() or 4

    success_count = 0
    print(f"🚀 Using {'multithreading' if use_multithreading else 'multiprocessing'} with {max_workers} workers")
    with executor_cls(max_workers=max_workers) as executor:
        future_to_file = {
            executor.submit(_encrypt_one_file, file_path, directory, password, mmap_binary): file_path
            for file_path in txt_files
        }

        for idx, future in enumerate(as_completed(future_to_file), 1):
            file_path = future_to_file[future]
            try:
                ok = future.result()
                if ok:
                    print(f"✅ Encrypted: {file_path.name}")
                    success_count += 1
                else:
                    print(f"❌ Failed to encrypt {file_path.name}")
            except subprocess.TimeoutExpired:
                print(f"⏰ Timeout encrypting {file_path.name}")
            except Exception as e:
                print(f"❌ Error encrypting {file_path.name}: {e}")

    print(f"\n🎉 Successfully encrypted {success_count}/{len(txt_files)} files")
    return success_count > 0

def _decrypt_one_file(file_path: Path, directory: Path, password: str, mmap_binary: Path) -> bool:
    # Create a temporary directory for this file's decryption
    temp_dir = directory / f"temp_{file_path.stem}"
    temp_dir.mkdir(exist_ok=True)

    try:
        # Copy the encrypted file to temp directory as mid.txt
        temp_file = temp_dir / "mid.txt"
        import shutil
        shutil.copy2(file_path, temp_file)

        # Copy the password file to temp directory
        password_file = directory / ".password.txt"
        temp_password_file = temp_dir / ".password.txt"
        shutil.copy2(password_file, temp_password_file)

        # Run decryption in temp directory
        result = subprocess.run(
            [str(mmap_binary)],
            input=f"{temp_file}\ndecrypt\n{password}\n",
            text=True,
            capture_output=True,
            timeout=30
        )

        if result.returncode == 0:
            # Move the decrypted file (out.txt) to the main directory
            decrypted_file = temp_dir / "out.txt"
            if decrypted_file.exists():
                # Get original filename (remove _encrypted suffix)
                original_name = file_path.stem.replace("_encrypted", "") + ".txt"
                final_decrypted = directory / original_name
                shutil.move(str(decrypted_file), str(final_decrypted))
                return True
            return False
        return False
    finally:
        import shutil
        shutil.rmtree(temp_dir, ignore_errors=True)


def decrypt_multifile(directory, password, use_multithreading=True):
    """Simple multi-file decryption using the mmap binary"""
    directory = Path(directory)
    if not directory.exists():
        print(f"❌ Directory {directory} does not exist")
        return False
    
    # Get all encrypted files (files ending with _encrypted.txt)
    encrypted_files = list(directory.glob("*_encrypted.txt"))
    if not encrypted_files:
        print(f"❌ No encrypted files found in {directory}")
        return False
    
    print(f"📁 Found {len(encrypted_files)} encrypted files to process")
    
    # Check password file
    password_file = directory / ".password.txt"
    if not password_file.exists():
        print(f"❌ Password file not found. Cannot decrypt without original password.")
        return False
    
    with open(password_file, 'r') as f:
        stored_password = f.read().strip()
    
    if stored_password != password:
        print(f"❌ Incorrect password. Access denied!")
        return False
    
    # Get the script directory
    script_dir = Path(__file__).parent
    mmap_binary = script_dir / "mmap"
    
    if not mmap_binary.exists():
        print(f"❌ mmap binary not found at {mmap_binary}")
        return False
    
    executor_cls = ThreadPoolExecutor if use_multithreading else ProcessPoolExecutor
    max_workers = os.cpu_count() or 4

    success_count = 0
    print(f"🚀 Using {'multithreading' if use_multithreading else 'multiprocessing'} with {max_workers} workers")
    with executor_cls(max_workers=max_workers) as executor:
        future_to_file = {
            executor.submit(_decrypt_one_file, file_path, directory, password, mmap_binary): file_path
            for file_path in encrypted_files
        }

        for future in as_completed(future_to_file):
            file_path = future_to_file[future]
            try:
                ok = future.result()
                if ok:
                    print(f"✅ Decrypted: {file_path.name}")
                    success_count += 1
                else:
                    print(f"❌ Failed to decrypt {file_path.name}")
            except subprocess.TimeoutExpired:
                print(f"⏰ Timeout decrypting {file_path.name}")
            except Exception as e:
                print(f"❌ Error decrypting {file_path.name}: {e}")

    print(f"\n🎉 Successfully decrypted {success_count}/{len(encrypted_files)} files")
    return success_count > 0

if __name__ == "__main__":
    if len(sys.argv) not in (4, 5):
        print("Usage: python3 simple_multifile.py <directory> <encrypt|decrypt> <password> [thread|process]")
        sys.exit(1)

    directory = sys.argv[1]
    action = sys.argv[2]
    password = sys.argv[3]
    method = sys.argv[4].lower() if len(sys.argv) == 5 else "thread"
    use_multithreading = method in ("thread", "threads", "multithreading")

    if action == "encrypt":
        encrypt_multifile(directory, password, use_multithreading=use_multithreading)
    elif action == "decrypt":
        decrypt_multifile(directory, password, use_multithreading=use_multithreading)
    else:
        print("❌ Action must be 'encrypt' or 'decrypt'")
        sys.exit(1)
