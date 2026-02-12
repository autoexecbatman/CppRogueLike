"""Debug: extract specific tile regions to preview and find the right ones."""
from PIL import Image
import os

TILE = 16
DAWN = "DawnLike"
OUT = "tile_debug"
os.makedirs(OUT, exist_ok=True)

def load(path):
    return Image.open(path).convert("RGBA")

def get_tile(sheet, col, row):
    x, y = col * TILE, row * TILE
    if x + TILE > sheet.width or y + TILE > sheet.height:
        return None
    return sheet.crop((x, y, x + TILE, y + TILE))

def save_tile_grid(sheet, name, cols_range, rows_range, label_coords=True):
    """Save a grid of tiles with coordinate labels for identification."""
    cols = list(cols_range)
    rows = list(rows_range)
    scale = 4
    cell = TILE * scale
    margin = 2 * scale

    img = Image.new("RGBA", (len(cols) * (cell + margin), len(rows) * (cell + margin)), (255, 0, 255, 255))

    for ri, r in enumerate(rows):
        for ci, c in enumerate(cols):
            tile = get_tile(sheet, c, r)
            if tile:
                scaled = tile.resize((cell, cell), Image.NEAREST)
                img.paste(scaled, (ci * (cell + margin), ri * (cell + margin)), scaled)

    img.save(f"{OUT}/{name}.png")
    # Print coordinate guide
    print(f"{name}: cols {cols[0]}-{cols[-1]}, rows {rows[0]}-{rows[-1]}")
    for ri, r in enumerate(rows):
        items = []
        for ci, c in enumerate(cols):
            items.append(f"({c},{r})")
        print(f"  row {r}: {' '.join(items)}")

# Wall: check first 4 cols, rows 2-10 of first style
wall = load(f"{DAWN}/Objects/Wall.png")
print(f"\nWall.png size: {wall.size} = {wall.width//TILE}x{wall.height//TILE} tiles")
save_tile_grid(wall, "wall_style1", range(0, 4), range(2, 12))

# Also check cols 8-11 (second style - brown)
save_tile_grid(wall, "wall_style2", range(8, 12), range(2, 12))

# Floor: check first few styles
floor = load(f"{DAWN}/Objects/Floor.png")
print(f"\nFloor.png size: {floor.size} = {floor.width//TILE}x{floor.height//TILE} tiles")
save_tile_grid(floor, "floor_gray", range(0, 7), range(2, 8))

# Door: all tiles
door = load(f"{DAWN}/Objects/Door0.png")
print(f"\nDoor0.png size: {door.size} = {door.width//TILE}x{door.height//TILE} tiles")
save_tile_grid(door, "door_all", range(0, door.width//TILE), range(0, door.height//TILE))

# Pit: water tiles
pit = load(f"{DAWN}/Objects/Pit0.png")
print(f"\nPit0.png size: {pit.size} = {pit.width//TILE}x{pit.height//TILE} tiles")
save_tile_grid(pit, "pit_water", range(0, 8), range(0, 10))

# Food
food = load(f"{DAWN}/Items/Food.png")
print(f"\nFood.png size: {food.size} = {food.width//TILE}x{food.height//TILE} tiles")
save_tile_grid(food, "food_all", range(0, 8), range(0, food.height//TILE))

# Armor: check body armor rows
armor = load(f"{DAWN}/Items/Armor.png")
print(f"\nArmor.png size: {armor.size} = {armor.width//TILE}x{armor.height//TILE} tiles")
save_tile_grid(armor, "armor_all", range(0, 8), range(0, armor.height//TILE))

# LongWep for bow
lwep = load(f"{DAWN}/Items/LongWep.png")
print(f"\nLongWep.png size: {lwep.size} = {lwep.width//TILE}x{lwep.height//TILE} tiles")
save_tile_grid(lwep, "longwep_all", range(0, 8), range(0, lwep.height//TILE))

# ShortWep for blunt
swep = load(f"{DAWN}/Items/ShortWep.png")
print(f"\nShortWep.png size: {swep.size} = {swep.width//TILE}x{swep.height//TILE} tiles")
save_tile_grid(swep, "shortwep_all", range(0, 8), range(0, swep.height//TILE))

# Decor for stairs/web
decor = load(f"{DAWN}/Objects/Decor0.png")
print(f"\nDecor0.png size: {decor.size} = {decor.width//TILE}x{decor.height//TILE} tiles")
save_tile_grid(decor, "decor_bottom", range(0, 8), range(14, min(22, decor.height//TILE)))

print(f"\nAll debug tiles saved to {OUT}/")
