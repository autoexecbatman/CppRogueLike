from PIL import Image, ImageDraw, ImageFont
import platform

def mark_tiles_minimal(image_path, output_path, tile_size=16):
    try:
        img = Image.open(image_path).convert("RGBA")
        draw = ImageDraw.Draw(img)
        width, height = img.size

        # --- Font Selection ---
        # We try to load a standard font at a small size (9px)
        # If this fails, it falls back to the default PIL font.
        font_size = 9
        font = None
        
        system = platform.system()
        try:
            if system == "Windows":
                font = ImageFont.truetype("arial.ttf", font_size)
            elif system == "Darwin": # macOS
                font = ImageFont.truetype("Arial.ttf", font_size)
            else: # Linux
                font = ImageFont.truetype("DejaVuSans.ttf", font_size)
        except IOError:
            print("Warning: Arial/DejaVu font not found. Using default (might be larger).")
            font = ImageFont.load_default()

        print(f"Labeling non-empty tiles on {width}x{height} image...")

        count = 0
        for y in range(0, height, tile_size):
            for x in range(0, width, tile_size):
                
                # 1. Check if tile is empty so we don't clutter background
                tile = img.crop((x, y, x + tile_size, y + tile_size))
                if not tile.getbbox():
                    continue # Skip empty/transparent tiles

                # 2. Prepare Coordinate Text: "y,x" (Grid indices)
                grid_y = y // tile_size
                grid_x = x // tile_size
                text = f"{grid_y},{grid_x}"

                # 3. Draw Text (Top Left)
                # Position: (x + 1, y) to tuck it in the corner
                text_pos_x = x + 1
                text_pos_y = y - 1 # slightly nudge up if font has padding
                
                # Draw simple 1px Drop Shadow (Black) for contrast
                draw.text((text_pos_x + 1, text_pos_y + 1), text, font=font, fill="black")
                
                # Draw Main Text (Yellow or White)
                draw.text((text_pos_x, text_pos_y), text, font=font, fill="#FFFF00")
                
                count += 1

        img.save(output_path)
        print(f"Done! Labeled {count} tiles. Saved to: {output_path}")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    # Settings
    INPUT_FILE = "Pit0.png"
    OUTPUT_FILE = "Pit0_enumerated.png"
    TILE_SIZE = 16 

    mark_tiles_minimal(INPUT_FILE, OUTPUT_FILE, TILE_SIZE)