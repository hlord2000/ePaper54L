import PIL
from PIL import Image
import numpy as np
import os

LVGL_TEMPLATE = '''#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_NORDIC
#define LV_ATTRIBUTE_IMG_NORDIC
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_NORDIC uint8_t {name}_map[] = {{
    0xff, 0xff, 0xff, 0xff,  /*Color of index 0*/
    0x00, 0x00, 0x00, 0xff,  /*Color of index 1*/

    /* Color bytes */
{byte_data}}};

const lv_img_dsc_t {name} = {{
    .header.cf = LV_IMG_CF_INDEXED_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = {width},
    .header.h = {height},
    .data_size = {data_size},
    .data = {name}_map,
}};'''

def srgb_to_linear(x):
    x = np.array(x, dtype=float) / 255.0
    return np.where(x <= 0.04045, x / 12.92, ((x + 0.055) / 1.055) ** 2.4)

def linear_to_srgb(x):
    return np.where(x <= 0.0031308, 12.92 * x, 1.055 * np.power(x, 1/2.4) - 0.055)

def atkinson_dither(img_array):
    height, width = img_array.shape
    output = img_array.copy()
    
    for y in range(height):
        for x in range(width):
            old_pixel = output[y, x]
            new_pixel = 1.0 if old_pixel > 0.5 else 0.0
            output[y, x] = new_pixel
            
            error = (old_pixel - new_pixel) / 8.0
            
            # Distribute error to neighboring pixels
            if x + 1 < width:
                output[y, x + 1] += error
            if x + 2 < width:
                output[y, x + 2] += error
            if y + 1 < height:
                output[y + 1, x] += error
                if x - 1 >= 0:
                    output[y + 1, x - 1] += error
                if x + 1 < width:
                    output[y + 1, x + 1] += error
            if y + 2 < height:
                output[y + 2, x] += error
                
    return (output > 0.5).astype(np.uint8) * 255

def process_image(input_path, output_path, target_size=(250, 102), dither_method='floyd', brightness=0, contrast=0):
    # Open and convert image to RGB
    img = Image.open(input_path).convert('RGB')
    
    # Resize image if needed
    if img.size != target_size:
        img = img.resize(target_size, PIL.Image.Resampling.LANCZOS)
    
    # Convert to grayscale
    img_gray = img.convert('L')
    
    # Apply brightness and contrast adjustments
    if brightness != 0 or contrast != 0:
        img_array = np.array(img_gray, dtype=np.float32)
        
        # Brightness adjustment
        if brightness != 0:
            img_array = img_array + (brightness * 255)
            
        # Contrast adjustment
        if contrast != 0:
            factor = (259 * (contrast * 255 + 255)) / (255 * (259 - contrast * 255))
            img_array = factor * (img_array - 128) + 128
            
        img_array = np.clip(img_array, 0, 255)
        img_gray = Image.fromarray(img_array.astype(np.uint8), mode='L')
    
    if dither_method == 'atkinson':
        # Convert to linear space, apply Atkinson dithering, then back to sRGB
        img_array = np.array(img_gray)
        linear_array = srgb_to_linear(img_array)
        dithered_array = atkinson_dither(linear_array)
        img = Image.fromarray(dithered_array.astype(np.uint8), mode='L')
    else:
        # Use Floyd-Steinberg dithering
        img = img_gray.convert('1', dither=PIL.Image.Dither.FLOYDSTEINBERG)
    
    # Save preview
    preview_path = os.path.join(os.path.dirname(output_path), f"preview_{os.path.basename(input_path)}")
    img.save(preview_path)
    
    # Convert to numpy array
    img_array = np.array(img)
    
    # Calculate bytes needed
    bytes_per_row = (target_size[0] + 7) // 8
    total_bytes = bytes_per_row * target_size[1]
    
    # Convert pixels to bytes
    byte_array = []
    for row in range(target_size[1]):
        for byte_index in range(bytes_per_row):
            byte = 0
            for bit in range(8):
                pixel_x = byte_index * 8 + bit
                if pixel_x < target_size[0]:
                    pixel = not img_array[row, pixel_x]
                    byte |= (pixel << (7 - bit))
            byte_array.append(byte)
    
    # Format byte data
    byte_strings = []
    for i in range(0, len(byte_array), 32):
        chunk = byte_array[i:i + 32]
        hex_str = ', '.join(f'0x{byte:02x}' for byte in chunk)
        byte_strings.append(f'    {hex_str},')
    
    # Generate output
    filename = os.path.splitext(os.path.basename(input_path))[0]
    output = LVGL_TEMPLATE.format(
        name=filename,
        byte_data='\n'.join(byte_strings),
        width=target_size[0],
        height=target_size[1],
        data_size=total_bytes + 8
    )
    
    with open(output_path, 'w') as f:
        f.write(output)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='Convert image to LVGL format')
    parser.add_argument('--input', required=True, help='Input image path')
    parser.add_argument('--width', type=int, default=250, help='Target image width')
    parser.add_argument('--height', type=int, default=102, help='Target image height')
    parser.add_argument('--dither', choices=['floyd', 'atkinson'], default='atkinson', help='Dithering method')
    parser.add_argument('--brightness', type=float, default=0, help='Brightness adjustment (-1.0 to 1.0)')
    parser.add_argument('--contrast', type=float, default=0, help='Contrast adjustment (-1.0 to 1.0)')
    args = parser.parse_args()
    
    output_path = os.path.splitext(args.input)[0] + '.c'
    process_image(args.input, output_path, target_size=(args.width, args.height), 
                dither_method=args.dither, brightness=args.brightness, contrast=args.contrast)
