"""Preview individual tiles from DawnLike sheets to pick the best ones."""
from PIL import Image
import os

TILE = 16
DAWN = "DawnLike"
OUT = "tile_previews"
os.makedirs(OUT, exist_ok=True)

def load(path):
    return Image.open(path).convert("RGBA")

def get_tile(sheet, col, row):
    x, y = col * TILE, row * TILE
    return sheet.crop((x, y, x + TILE, y + TILE))

def save_row(sheet, name, row, max_cols):
    """Save all tiles from a row as a single strip for easy preview."""
    strip = Image.new("RGBA", (max_cols * TILE, TILE), (255, 0, 255, 255))
    for c in range(max_cols):
        tile = get_tile(sheet, c, row)
        strip.paste(tile, (c * TILE, 0), tile)
    # Scale up 4x for visibility
    strip = strip.resize((max_cols * TILE * 4, TILE * 4), Image.NEAREST)
    strip.save(f"{OUT}/{name}_row{row}.png")

def save_grid(sheet, name, max_cols, max_rows):
    """Save entire sheet as labeled grid."""
    w = min(max_cols, sheet.width // TILE)
    h = min(max_rows, sheet.height // TILE)
    grid = Image.new("RGBA", (w * TILE, h * TILE), (255, 0, 255, 255))
    for r in range(h):
        for c in range(w):
            tile = get_tile(sheet, c, r)
            grid.paste(tile, (c * TILE, r * TILE), tile)
    grid = grid.resize((w * TILE * 4, h * TILE * 4), Image.NEAREST)
    grid.save(f"{OUT}/{name}_grid.png")

# Preview key sheets at 4x zoom
sheets = {
    "Player0": (f"{DAWN}/Characters/Player0.png", 8, 15),
    "Humanoid0": (f"{DAWN}/Characters/Humanoid0.png", 8, 27),
    "Reptile0": (f"{DAWN}/Characters/Reptile0.png", 8, 16),
    "Pest0": (f"{DAWN}/Characters/Pest0.png", 8, 11),
    "Dog0": (f"{DAWN}/Characters/Dog0.png", 8, 7),
    "Avian0": (f"{DAWN}/Characters/Avian0.png", 8, 13),
    "Demon0": (f"{DAWN}/Characters/Demon0.png", 8, 9),
    "Potion": (f"{DAWN}/Items/Potion.png", 8, 5),
    "Scroll": (f"{DAWN}/Items/Scroll.png", 8, 6),
    "ShortWep": (f"{DAWN}/Items/ShortWep.png", 8, 5),
    "LongWep": (f"{DAWN}/Items/LongWep.png", 8, 5),
    "Armor": (f"{DAWN}/Items/Armor.png", 8, 9),
    "Shield": (f"{DAWN}/Items/Shield.png", 8, 2),
    "Ring": (f"{DAWN}/Items/Ring.png", 8, 5),
    "Money": (f"{DAWN}/Items/Money.png", 8, 8),
    "Food": (f"{DAWN}/Items/Food.png", 8, 6),
    "Amulet": (f"{DAWN}/Items/Amulet.png", 8, 3),
    "Hat": (f"{DAWN}/Items/Hat.png", 8, 2),
    "Boot": (f"{DAWN}/Items/Boot.png", 8, 2),
    "Wand": (f"{DAWN}/Items/Wand.png", 8, 5),
    "Decor0": (f"{DAWN}/Objects/Decor0.png", 8, 22),
}

for name, (path, cols, rows) in sheets.items():
    sheet = load(path)
    save_grid(sheet, name, cols, rows)
    print(f"Saved {name}_grid.png ({cols}x{rows})")

# Wall and Floor have special widths
wall = load(f"{DAWN}/Objects/Wall.png")
w_cols = wall.width // TILE
w_rows = wall.height // TILE
save_grid(wall, "Wall", w_cols, min(w_rows, 20))
print(f"Saved Wall_grid.png ({w_cols}x{min(w_rows, 20)})")

floor = load(f"{DAWN}/Objects/Floor.png")
f_cols = floor.width // TILE
f_rows = floor.height // TILE
save_grid(floor, "Floor", f_cols, min(f_rows, 20))
print(f"Saved Floor_grid.png ({f_cols}x{min(f_rows, 20)})")

door = load(f"{DAWN}/Objects/Door0.png")
d_cols = door.width // TILE
d_rows = door.height // TILE
save_grid(door, "Door0", d_cols, d_rows)
print(f"Saved Door0_grid.png ({d_cols}x{d_rows})")

trap = load(f"{DAWN}/Objects/Trap0.png")
t_cols = trap.width // TILE
t_rows = trap.height // TILE
save_grid(trap, "Trap0", t_cols, t_rows)
print(f"Saved Trap0_grid.png ({t_cols}x{t_rows})")

print(f"\nAll previews saved to {OUT}/")
