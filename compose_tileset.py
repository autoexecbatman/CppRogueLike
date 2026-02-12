"""
Compose DawnLike tiles into two 256-tile CP437 BMPs for animated rendering.

DawnLike sheets come in animation pairs: Sheet0.png (frame 0), Sheet1.png (frame 1).
Output: tileset.bmp (frame 0) and tileset1.bmp (frame 1).
Each is 256x256 (16 cols x 16 rows of 16x16 tiles), magenta background.

Tile coordinates verified via 4x zoomed debug previews.
"""

from PIL import Image, ImageDraw

TILE = 16
COLS = 16
ROWS = 16
MAGENTA = (255, 0, 255)
DAWN = "DawnLike"


def load_sheet(path):
    return Image.open(path).convert("RGBA")


def get_tile(sheet, col, row):
    """Extract a 16x16 tile from a DawnLike sheet at grid position (col, row)."""
    x = col * TILE
    y = row * TILE
    if x + TILE > sheet.width or y + TILE > sheet.height:
        print(f"  WARNING: ({col},{row}) out of bounds for {sheet.size}")
        return Image.new("RGBA", (TILE, TILE), MAGENTA + (255,))
    return sheet.crop((x, y, x + TILE, y + TILE))


def place_tile(output, tile_img, cp437_code, name=""):
    """Place a tile into the output image at the CP437 grid position."""
    out_col = cp437_code % COLS
    out_row = cp437_code // COLS
    bg = Image.new("RGBA", (TILE, TILE), MAGENTA + (255,))
    bg.paste(tile_img, (0, 0), tile_img)
    output.paste(bg.convert("RGB"), (out_col * TILE, out_row * TILE))
    if name:
        print(f"  '{chr(cp437_code)}' ({cp437_code:3d}) = {name}")


def draw_stairs_down():
    """Draw a custom stairs-down icon (descending steps)."""
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, 15, 15], fill=(30, 25, 25, 255))
    steps = [
        (1, 1, 5, 4, (170, 165, 155)),
        (3, 4, 8, 7, (140, 135, 125)),
        (5, 7, 11, 10, (110, 105, 95)),
        (8, 10, 14, 13, (80, 75, 65)),
    ]
    for x1, y1, x2, y2, color in steps:
        d.rectangle([x1, y1, x2, y2], fill=color + (255,))
        d.line([(x1, y1), (x2, y1)], fill=(200, 195, 185, 255))
    d.rectangle([0, 0, 15, 15], outline=(60, 55, 50, 255))
    return img


def draw_stairs_up():
    """Draw a custom stairs-up icon (ascending steps)."""
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, 15, 15], fill=(30, 25, 25, 255))
    steps = [
        (8, 1, 14, 4, (80, 75, 65)),
        (5, 4, 11, 7, (110, 105, 95)),
        (3, 7, 8, 10, (140, 135, 125)),
        (1, 10, 5, 13, (170, 165, 155)),
    ]
    for x1, y1, x2, y2, color in steps:
        d.rectangle([x1, y1, x2, y2], fill=color + (255,))
        d.line([(x1, y1), (x2, y1)], fill=(200, 195, 185, 255))
    d.rectangle([10, 1, 13, 3], fill=(240, 230, 180, 255))
    d.rectangle([0, 0, 15, 15], outline=(60, 55, 50, 255))
    return img


def draw_bow():
    """Draw a custom bow sprite for ranged weapons."""
    img = Image.new("RGBA", (TILE, TILE), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # Bow arc (brown wood)
    wood = (139, 90, 43, 255)
    d.arc([3, 1, 11, 14], start=270, end=90, fill=wood, width=2)
    # Bowstring (light gray)
    string = (200, 200, 200, 255)
    d.line([(9, 2), (9, 13)], fill=string)
    # Arrow
    shaft = (160, 130, 80, 255)
    tip = (180, 190, 200, 255)
    d.line([(6, 8), (14, 8)], fill=shaft)
    d.polygon([(14, 6), (14, 10), (15, 8)], fill=tip)
    # Fletching
    d.line([(6, 6), (6, 10)], fill=(200, 50, 50, 255))
    return img


def compose_frame(frame_num):
    """Compose one animation frame. frame_num is 0 or 1."""
    suffix = str(frame_num)
    output = Image.new("RGB", (COLS * TILE, ROWS * TILE), MAGENTA)

    # ===== LOAD SHEETS =====
    # Characters: have animation pairs (0/1)
    player = load_sheet(f"{DAWN}/Characters/Player{suffix}.png")
    humanoid = load_sheet(f"{DAWN}/Characters/Humanoid{suffix}.png")
    reptile = load_sheet(f"{DAWN}/Characters/Reptile{suffix}.png")
    pest = load_sheet(f"{DAWN}/Characters/Pest{suffix}.png")
    dog = load_sheet(f"{DAWN}/Characters/Dog{suffix}.png")
    avian = load_sheet(f"{DAWN}/Characters/Avian{suffix}.png")
    demon = load_sheet(f"{DAWN}/Characters/Demon{suffix}.png")

    # Items: no animation pairs, same for both frames
    potion = load_sheet(f"{DAWN}/Items/Potion.png")
    scroll = load_sheet(f"{DAWN}/Items/Scroll.png")
    short_wep = load_sheet(f"{DAWN}/Items/ShortWep.png")
    armor_sheet = load_sheet(f"{DAWN}/Items/Armor.png")
    ring_sheet = load_sheet(f"{DAWN}/Items/Ring.png")
    money = load_sheet(f"{DAWN}/Items/Money.png")
    food = load_sheet(f"{DAWN}/Items/Food.png")
    amulet_sheet = load_sheet(f"{DAWN}/Items/Amulet.png")
    hat = load_sheet(f"{DAWN}/Items/Hat.png")
    chest = load_sheet(f"{DAWN}/Items/Chest{suffix}.png")

    # Objects: some have animation pairs
    wall = load_sheet(f"{DAWN}/Objects/Wall.png")
    floor_sheet = load_sheet(f"{DAWN}/Objects/Floor.png")
    door = load_sheet(f"{DAWN}/Objects/Door0.png")  # always frame 0 -- doors don't animate
    decor = load_sheet(f"{DAWN}/Objects/Decor{suffix}.png")
    pit = load_sheet(f"{DAWN}/Objects/Pit{suffix}.png")

    tag = f"[frame {frame_num}]"

    # ================================================================
    # MAP TILES
    # ================================================================

    # '#' Wall: Wall.png col 1, row 6 = dark solid blue stone
    place_tile(output, get_tile(wall, 1, 6), ord('#'), f"{tag} Wall")

    # '.' Floor: Floor.png col 2, row 3 = gray stone
    place_tile(output, get_tile(floor_sheet, 2, 3), ord('.'), f"{tag} Floor")

    # '~' Water: Pit0/1.png row 8 = blue water with ripples
    place_tile(output, get_tile(pit, 0, 8), ord('~'), f"{tag} Water")

    # '+' Closed Door: Door0.png col 0, row 1 (no animation -- always frame 0)
    place_tile(output, get_tile(door, 0, 1), ord('+'), f"{tag} Closed Door")

    # "'" Open Door: Door0.png col 0, row 3 (no animation -- always frame 0)
    # Open doors use apostrophe to avoid conflict with '/' weapons
    place_tile(output, get_tile(door, 0, 3), ord("'"), f"{tag} Open Door")

    # '/' Sword: ShortWep.png col 0, row 0 (weapon character, not a door)
    place_tile(output, get_tile(short_wep, 0, 0), ord('/'), f"{tag} Sword")

    # '>' Stairs Down (custom - same both frames)
    place_tile(output, draw_stairs_down(), ord('>'), f"{tag} Stairs Down")

    # '<' Stairs Up (custom - same both frames)
    place_tile(output, draw_stairs_up(), ord('<'), f"{tag} Stairs Up")

    # '*' Spider Web: Decor0/1.png row 20
    place_tile(output, get_tile(decor, 1, 20), ord('*'), f"{tag} Web")

    # ================================================================
    # PLAYER
    # ================================================================

    # '@' Player: Player0/1.png col 0, row 0
    place_tile(output, get_tile(player, 0, 0), ord('@'), f"{tag} Player")

    # ================================================================
    # MONSTERS (all animated via Character0/1 pairs)
    # ================================================================

    place_tile(output, get_tile(humanoid, 0, 14), ord('g'), f"{tag} Goblin")
    place_tile(output, get_tile(humanoid, 2, 14), ord('o'), f"{tag} Orc")
    place_tile(output, get_tile(demon, 0, 0), ord('T'), f"{tag} Troll")
    place_tile(output, get_tile(reptile, 0, 4), ord('D'), f"{tag} Dragon")
    place_tile(output, get_tile(humanoid, 0, 4), ord('a'), f"{tag} Archer")
    place_tile(output, get_tile(humanoid, 0, 6), ord('m'), f"{tag} Mage")
    place_tile(output, get_tile(dog, 0, 0), ord('w'), f"{tag} Wolf")
    place_tile(output, get_tile(avian, 0, 2), ord('b'), f"{tag} Bat")
    place_tile(output, get_tile(reptile, 0, 0), ord('k'), f"{tag} Kobold")
    place_tile(output, get_tile(chest, 0, 0), ord('M'), f"{tag} Mimic")
    place_tile(output, get_tile(pest, 1, 4), ord('S'), f"{tag} Giant Spider")
    place_tile(output, get_tile(pest, 0, 4), ord('s'), f"{tag} Small Spider")
    place_tile(output, get_tile(pest, 3, 4), ord('W'), f"{tag} Web Weaver")

    # ================================================================
    # ITEMS (same both frames - no animation)
    # ================================================================

    place_tile(output, get_tile(potion, 0, 0), ord('!'), f"{tag} Potion")
    place_tile(output, get_tile(scroll, 0, 0), ord('?'), f"{tag} Scroll")
    place_tile(output, draw_bow(), ord(')'), f"{tag} Bow")
    place_tile(output, get_tile(short_wep, 0, 2), ord('|'), f"{tag} Club")
    place_tile(output, get_tile(armor_sheet, 0, 0), ord('['), f"{tag} Armor")
    place_tile(output, get_tile(hat, 3, 0), ord('^'), f"{tag} Helmet")
    place_tile(output, get_tile(ring_sheet, 0, 0), ord('='), f"{tag} Ring")
    place_tile(output, get_tile(amulet_sheet, 0, 0), ord('"'), f"{tag} Amulet")
    place_tile(output, get_tile(food, 0, 2), ord('%'), f"{tag} Food")
    place_tile(output, get_tile(money, 0, 0), ord('$'), f"{tag} Gold")

    return output


def main():
    print("Composing animated tileset from DawnLike sprites...\n")

    for frame in (0, 1):
        output = compose_frame(frame)
        bmp_name = "tileset.bmp" if frame == 0 else "tileset1.bmp"
        output.save(bmp_name, "BMP")
        print(f"  {bmp_name} saved")

    # Preview: side-by-side comparison of both frames
    f0 = Image.open("tileset.bmp").resize((COLS * TILE * 4, ROWS * TILE * 4), Image.NEAREST)
    f1 = Image.open("tileset1.bmp").resize((COLS * TILE * 4, ROWS * TILE * 4), Image.NEAREST)
    combined = Image.new("RGB", (f0.width * 2 + 8, f0.height), MAGENTA)
    combined.paste(f0, (0, 0))
    combined.paste(f1, (f0.width + 8, 0))
    combined.save("tileset_preview.png", "PNG")
    print("  tileset_preview.png saved (side-by-side frames)")
    print("\nDone! Two animation frames generated.")


if __name__ == "__main__":
    main()
