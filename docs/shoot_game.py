"""Drive the published web build with keystrokes and save what each screen looks like.

The game renders to a WebGL canvas, so nothing about how it looks can be read out
of the source. This loads the page in a headless browser, sends a scripted key
sequence, and writes one PNG per step for a human - or a model - to look at.

The hard part is not the screenshot, it is knowing the screen actually changed.
The game animates: torches flicker and sprite sheets alternate frames, so some
pixels differ between any two captures whether or not a key did anything. Every
step therefore measures that screen's own noise floor first, with no input, and
reports the fraction of pixels the keys then moved against it. A step below its
floor did nothing, and says so instead of writing a picture of the previous
screen.

Run it:

    python docs/shoot_game.py https://autoexecbatman.github.io/CppRogueLike/ docs/steps.json shots --width 1280 --height 900

      00_main_menu           moved 0.0000  floor 0.0000  baseline, no keys sent
      01_new_game            moved 0.0024  floor 0.0000  changed

And the refusal, from the run that found the frame-boundary problem below:

      01_confirm_1           moved 0.0000  floor 0.0000  UNCHANGED - the keys reached nothing

The PNG is written either way, so a reader can see what the screen was stuck on,
but no step ever claims a transition that did not happen.

Two things this gets wrong if you write them yourself, both found by measurement
rather than by reading the source. A key must be held across a frame boundary or
the game never polls it. And the canvas must be focused rather than clicked - a
click is real mouse input, and one at the canvas centre lands on a menu row.

The step file is a JSON list. Each entry needs a `label` and a `keys` list of
Playwright key names, pressed in order:

    [
      {"label": "main_menu", "keys": []},
      {"label": "new_game",  "keys": ["Enter"]}
    ]

Needs playwright and Pillow. Refuses a step file that is not a list of objects
carrying both fields, and a page whose loading overlay never clears.
"""

import argparse
import io
import json
import sys
import time
from pathlib import Path

from PIL import Image, ImageChops
from playwright.sync_api import sync_playwright

# How long to wait for the wasm bundle to download and the first frame to draw.
LOAD_TIMEOUT_SECONDS = 90
# How long to keep re-capturing after a keypress before calling the screen settled.
SETTLE_TIMEOUT_SECONDS = 6
# Gap between captures while waiting for a screen to change.
CAPTURE_INTERVAL_SECONDS = 0.35
# How long each key is held down. The game polls input once per rendered frame, so a
# keydown and keyup delivered in the same frame are never seen - measured here, a 0ms
# hold changes nothing and 50ms onward always registers. A human holds a key far
# longer than this; only a synthetic press is ever fast enough to disappear.
KEY_HOLD_SECONDS = 0.12
# Extra captures taken when measuring a screen's noise floor. One pair is not
# enough: the animation toggles on a timer, so a single pair can straddle nothing.
NOISE_FLOOR_SAMPLES = 4


def canvas_shot(page):
    """Capture the game canvas as PNG bytes.

    Shoots the element rather than the viewport, so the black page margin around
    the canvas cannot dilute the change measurement below.

    Example:
        data = canvas_shot(page)
        len(data)                      # -> 6521
        data[:4]                       # -> PNG magic

    Args:
        page: a Playwright page with the game already loaded.
    """
    # The canvas carries id="canvas"; the loading overlay is a sibling and is
    # excluded by shooting the element rather than the page.
    return page.locator("#canvas").screenshot(type="png")


def changed_fraction(before, after):
    """Fraction of pixels that differ between two PNG captures, in [0, 1].

    Any non-zero channel difference counts as a changed pixel, so a one-shade
    flicker weighs the same as a full repaint. That is deliberate: the question
    is how much of the screen moved, not by how much.

    Example:
        changed_fraction(menu_png, menu_png)        # -> 0.0
        changed_fraction(menu_png, character_png)   # -> 0.9083

    Args:
        before: PNG bytes captured first.
        after: PNG bytes captured second.

    Raises:
        ValueError: the two captures are different sizes, which means the canvas
            was resized between them and no comparison is meaningful.
    """
    # Both are decoded to RGB so an alpha channel cannot register as a change.
    first = Image.open(io.BytesIO(before)).convert("RGB")
    second = Image.open(io.BytesIO(after)).convert("RGB")
    if first.size != second.size:
        raise ValueError(f"canvas resized between captures: {first.size} then {second.size}")

    # A pixel counts as changed when any channel moved at all.
    difference = ImageChops.difference(first, second).convert("L")
    changed = sum(count for value, count in difference.getcolors(maxcolors=256) if value != 0)
    return changed / (first.size[0] * first.size[1])


def wait_for_game(page, timeout_seconds):
    """Block until the loading overlay clears and the canvas has drawn something.

    A capture taken before this returns is a picture of an empty canvas, which
    looks exactly like a game that renders nothing.

    Example:
        wait_for_game(page, 90)   # returns None once the menu is on screen

    Args:
        page: a Playwright page that has already navigated to the game.
        timeout_seconds: how long to wait before giving up.

    Raises:
        TimeoutError: the overlay never cleared, or the canvas stayed blank.
    """
    # Emscripten's shell hides this overlay once the bundle has run.
    page.wait_for_selector("#loading-overlay", state="hidden", timeout=timeout_seconds * 1000)

    # A hidden overlay is not a drawn frame: wait until the canvas stops being one
    # flat colour, which is what an uninitialised WebGL surface looks like.
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        image = Image.open(io.BytesIO(canvas_shot(page))).convert("RGB")
        colours = image.getcolors(maxcolors=256)
        if colours is None or len(colours) > 1:
            return
        time.sleep(CAPTURE_INTERVAL_SECONDS)

    raise TimeoutError(f"canvas was still one flat colour after {timeout_seconds}s")


def measure_noise_floor(page):
    """Fraction of pixels that move on their own, with no input sent.

    This is the threshold every step is read against. Animation puts a floor under
    the measurement, and an effect below it is invisible rather than absent - the
    two are not the same finding.

    Sampled across several captures and reported as the largest gap seen, because
    the animation toggles on a timer: two captures a third of a second apart can
    both land inside the same frame and report a floor of zero on a screen that
    plainly moves.

    Example:
        measure_noise_floor(page)    # -> 0.0 on the static main menu
        measure_noise_floor(page)    # -> 0.0063 in the dungeon

    Args:
        page: a Playwright page with the game loaded and settled.
    """
    # A window wide enough to contain at least one animation toggle.
    captures = [canvas_shot(page)]
    for _ in range(NOISE_FLOOR_SAMPLES):
        time.sleep(CAPTURE_INTERVAL_SECONDS)
        captures.append(canvas_shot(page))

    return max(changed_fraction(captures[0], later) for later in captures[1:])


def run_step(page, step, destination):
    """Send one step's keys, wait for the screen to settle, and write the PNG.

    Re-captures until the screen has moved by more than this screen's own noise
    floor, or the settle timeout expires. Writes the last capture either way and
    reports which happened - a step whose keys did nothing is a result, not a
    failure.

    The floor is measured per step rather than once for the run. Menus are static
    and the dungeon is not: torches flicker and sprites alternate frames, so a
    floor taken on the main menu is 0.0000 and makes every later in-game step read
    as changed whether or not its keys arrived. That is how a dead key went
    unnoticed here.

    Example:
        run_step(page, {"label": "new_game", "keys": ["Enter"]}, path)
        # -> {'label': 'new_game', 'fraction': 0.9083, 'floor': 0.0, 'moved': True}

    Args:
        page: a Playwright page with the game loaded.
        step: dict carrying `label` and a `keys` list of Playwright key names.
        destination: Path to write the PNG to.
    """
    # What this screen does on its own, before anything is pressed.
    noise_floor = measure_noise_floor(page)

    # The frame this step starts from, for the comparison below.
    before = canvas_shot(page)

    # Keys go to the focused canvas, in the order written, each held long enough to
    # survive to the next polled frame. page.keyboard.press() does not do this.
    for key in step["keys"]:
        page.keyboard.down(key)
        time.sleep(KEY_HOLD_SECONDS)
        page.keyboard.up(key)

    # Re-capture until the screen has moved further than animation alone would.
    after = before
    fraction = 0.0
    deadline = time.monotonic() + SETTLE_TIMEOUT_SECONDS
    while time.monotonic() < deadline:
        time.sleep(CAPTURE_INTERVAL_SECONDS)
        after = canvas_shot(page)
        fraction = changed_fraction(before, after)
        if fraction > noise_floor:
            break

    destination.write_bytes(after)
    return {
        "label": step["label"],
        "fraction": fraction,
        "floor": noise_floor,
        "moved": fraction > noise_floor
    }


def load_steps(path):
    """Read and validate the step file.

    Example:
        load_steps(Path("docs/steps.json"))
        # -> [{'label': 'main_menu', 'keys': []}, {'label': 'new_game', 'keys': ['Enter']}]

    Args:
        path: Path to a JSON file holding a list of step objects.

    Raises:
        ValueError: the file is not a list, or a step is missing `label` or `keys`.
    """
    steps = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(steps, list):
        raise ValueError(f"{path} must hold a JSON list of steps")

    # Checked up front: a malformed step found halfway through leaves a part-written
    # run that reads like a complete one.
    for index, step in enumerate(steps):
        if not isinstance(step, dict) or "label" not in step or "keys" not in step:
            raise ValueError(f"step {index} in {path} needs both 'label' and 'keys'")
    return steps


def main():
    """Walk the step file, writing one PNG per step, and report what moved."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("url", help="the published game, e.g. https://autoexecbatman.github.io/CppRogueLike/")
    parser.add_argument("steps", help="JSON file listing the steps to walk")
    parser.add_argument("out_dir", help="directory to write the PNGs into")
    parser.add_argument("--width", type=int, required=True, help="browser viewport width in pixels")
    parser.add_argument("--height", type=int, required=True, help="browser viewport height in pixels")
    arguments = parser.parse_args()

    steps = load_steps(Path(arguments.steps))
    out_dir = Path(arguments.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    with sync_playwright() as playwright:
        browser = playwright.chromium.launch()
        page = browser.new_page(viewport={"width": arguments.width, "height": arguments.height})
        page.goto(arguments.url, wait_until="domcontentloaded")
        wait_for_game(page, LOAD_TIMEOUT_SECONDS)

        # The canvas carries tabindex=0, so it takes focus without a click. Do not
        # click it: the game reads mouse input, and a click at the canvas centre
        # lands on a menu row and moves the cursor before the first step runs.
        page.locator("#canvas").focus()

        for index, step in enumerate(steps):
            destination = out_dir / f"{index:02d}_{step['label']}.png"
            result = run_step(page, step, destination)
            # A step that sends nothing is a baseline capture; only a step that
            # pressed something and moved nothing is reporting a problem.
            if not step["keys"]:
                verdict = "baseline, no keys sent"
            elif result["moved"]:
                verdict = "changed"
            else:
                verdict = "UNCHANGED - the keys reached nothing"
            print(f"  {index:02d}_{step['label']:22} moved {result['fraction']:.4f}  floor {result['floor']:.4f}  {verdict}")

        browser.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
