import struct
import os
import sys

def get_architecture(filepath):
    if not os.path.exists(filepath):
        return "File not found"
    
    try:
        with open(filepath, 'rb') as f:
            # Check for MZ signature
            if f.read(2) != b'MZ':
                return "Not a valid PE file"
            
            # Offset to PE header is at 0x3C
            f.seek(0x3C)
            pe_offset = struct.unpack('<I', f.read(4))[0]
            
            # Seek to PE header
            f.seek(pe_offset)
            if f.read(4) != b'PE\0\0':
                return "Not a valid PE header"
            
            # Machine type is the next 2 bytes
            machine_type = struct.unpack('<H', f.read(2))[0]
            
            if machine_type == 0x014c:
                return "32-bit (x86)"
            elif machine_type == 0x8664:
                return "64-bit (x64)"
            else:
                return f"Unknown machine type: {hex(machine_type)}"
    except Exception as e:
        return f"Error reading file: {e}"

if len(sys.argv) > 1:
    files = sys.argv[1:]
else:
    files = ["py1401.pyd", "use1432.dll"]

for f in files:
    arch = get_architecture(f)
    print(f"{f}: {arch}")

