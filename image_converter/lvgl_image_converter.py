#!/usr/bin/env python3
from PIL import Image
import sys
import os

def image_to_lvgl_mono(image_path, variable_name="image_data"):
    """
    Convert a monochrome image to LVGL compatible C array format.
    Each byte contains 8 pixels, bit order: MSB first
    """
    # Open and convert image to black and white
    img = Image.open(image_path).convert('1')
    width, height = img.size
    
    # Calculate padded width (must be multiple of 8 bits)
    padded_width = (width + 7) & ~7
    bytes_per_line = padded_width // 8
    
    # Create output array
    output_bytes = []
    pixels = img.load()
    
    # Process each line
    for y in range(height):
        line_bytes = []
        current_byte = 0
        bit_count = 0
        
        # Process each pixel in the line
        for x in range(padded_width):
            # If we're within the actual image, get the pixel value
            if x < width:
                # Convert pixel to 1 or 0 (inverted because in PIL, 1 is black)
                pixel = 0 if pixels[x, y] else 1
            else:
                # Padding pixels are 0
                pixel = 0
                
            # Add pixel to current byte (MSB first)
            current_byte = (current_byte << 1) | pixel
            bit_count += 1
            
            # When we have 8 bits, store the byte
            if bit_count == 8:
                line_bytes.append(current_byte)
                current_byte = 0
                bit_count = 0
                
        output_bytes.extend(line_bytes)
    
    # Generate C code
    c_code = [
        f"#define {variable_name.upper()}_WIDTH {width}",
        f"#define {variable_name.upper()}_HEIGHT {height}",
        "",
        f"static const uint8_t {variable_name}[] = {{",
        "    /* Pixel format: Monochrome, MSB first */"
    ]
    
    # Convert bytes to hex format with comments showing binary
    for i in range(0, len(output_bytes), bytes_per_line):
        line = output_bytes[i:i + bytes_per_line]
        hex_values = [f"0x{byte:02X}" for byte in line]
        
        # Add line with hex values and binary comment
        c_code.append(f"    {', '.join(hex_values)}, ")
    
    c_code.append("};")
    
    # Add LVGL descriptor
    c_code.extend([
        "",
        "static lv_img_dsc_t img_dsc = {",
        "    .header.cf = LV_IMG_CF_ALPHA_1BIT,",
        "    .header.always_zero = 0,",
        "    .header.reserved = 0,",
        f"    .header.w = {width},",
        f"    .header.h = {height},",
        f"    .data_size = {len(output_bytes)},",
        f"    .data = {variable_name}",
        "};"
    ])
    
    return "\n".join(c_code)

def main():
    if len(sys.argv) != 2:
        print("Usage: python script.py <image_path>")
        sys.exit(1)
        
    image_path = sys.argv[1]
    if not os.path.exists(image_path):
        print(f"Error: File '{image_path}' not found")
        sys.exit(1)
        
    try:
        # Generate base name for variable from file name
        base_name = os.path.splitext(os.path.basename(image_path))[0]
        variable_name = f"{base_name}_map"
        
        # Convert image and print result
        result = image_to_lvgl_mono(image_path, variable_name)
        print(result)
        
    except Exception as e:
        print(f"Error processing image: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
