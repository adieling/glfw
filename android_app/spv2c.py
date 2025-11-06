#!/usr/bin/env python3
"""
Extract SPIR-V binary from compiled shader and generate C code
"""

import struct
import sys

def spv_to_c_array(spv_file, var_name):
    """Convert SPIR-V binary to C array"""
    with open(spv_file, 'rb') as f:
        data = f.read()
    
    # Ensure 4-byte aligned
    if len(data) % 4 != 0:
        data += b'\x00' * (4 - len(data) % 4)
    
    # Convert to uint32 array
    words = struct.unpack(f'{len(data)//4}I', data)
    
    # Generate C code
    lines = [f"const uint32_t {var_name}[] = {{"]
    for i in range(0, len(words), 8):
        chunk = words[i:i+8]
        hex_strs = [f"0x{w:08x}" for w in chunk]
        lines.append("    " + ",".join(hex_strs) + ("," if i + 8 < len(words) else ""))
    lines.append("};")
    
    return "\n".join(lines), len(data)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 spv2c.py <spv_file> [var_name]")
        sys.exit(1)
    
    spv_file = sys.argv[1]
    var_name = sys.argv[2] if len(sys.argv) > 2 else "shader_code"
    
    code, size = spv_to_c_array(spv_file, var_name)
    print(code)
    print(f"\n// Size: {size} bytes")
