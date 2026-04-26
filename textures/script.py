import os
from PIL import Image

input_root = r"C:\Users\Shivam\Documents\cgv\2D-Top-down-shooter\textures"
output_root = r"C:\Users\Shivam\Documents\cgv\2D-Top-down-shooter\textures_bmp"

for root, dirs, files in os.walk(input_root):
    for file in files:
        if file.lower().endswith(".png"):
            input_path = os.path.join(root, file)

            # preserve folder structure
            relative_path = os.path.relpath(root, input_root)
            output_dir = os.path.join(output_root, relative_path)
            os.makedirs(output_dir, exist_ok=True)

            output_file = os.path.splitext(file)[0] + ".bmp"
            output_path = os.path.join(output_dir, output_file)

            with Image.open(input_path) as img:
                rgb_img = img.convert("RGB")  # ensures 24-bit
                rgb_img.save(output_path, "BMP")

            print(f"Converted: {input_path} -> {output_path}")