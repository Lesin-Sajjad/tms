import os
import sys
import ctypes
import platform

def check_file(name):
    path = os.path.abspath(name)
    exists = os.path.exists(path)
    print(f"File: {name}")
    print(f"  Path: {path}")
    print(f"  Exists: {exists}")
    if exists:
        print(f"  Size: {os.path.getsize(path)} bytes")
    return exists

print(f"Python: {sys.version}")
print(f"Architecture: {platform.architecture()}")
print(f"Working Dir: {os.getcwd()}")
print("-" * 30)

check_file("py1401.pyd")
check_file("use1432.dll")
print("-" * 30)

# Try loading the DLL directly
if os.path.exists("use1432.dll"):
    print("Attempting to load use1432.dll via ctypes...")
    try:
        # Add current dir to DLL search path for the test
        if hasattr(os, 'add_dll_directory'):
            os.add_dll_directory(os.getcwd())
        
        lib_use = ctypes.WinDLL(os.path.abspath("use1432.dll"))
        print("Success: use1432.dll loaded successfully.")
    except Exception as e:
        print(f"Failure: Could not load use1432.dll. Error: {e}")
else:
    print("Skipping DLL load test (file missing).")

# Try loading the .pyd as a DLL
if os.path.exists("py1401.pyd"):
    print("\nAttempting to load py1401.pyd as a DLL via ctypes...")
    try:
        lib_pyd = ctypes.WinDLL(os.path.abspath("py1401.pyd"))
        print("Success: py1401.pyd loaded successfully (as a generic DLL).")
    except Exception as e:
        print(f"Failure: Could not load py1401.pyd. Error: {e}")
        print("Note: If it says 'specified procedure could not be found', it's likely a Python version mismatch.")
else:
    print("Skipping py1401.pyd load test.")


print("-" * 30)
print("Checking for common missing dependencies...")
# Check for VCRUNTIME or similar if possible (simplified)
system32 = os.environ.get('SystemRoot', 'C:\\Windows') + '\\System32'
for dll in ['vcruntime140.dll', 'msvcp140.dll']:
    path = os.path.join(system32, dll)
    print(f"{dll}: {'Found' if os.path.exists(path) else 'NOT FOUND'}")
