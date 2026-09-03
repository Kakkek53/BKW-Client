#!/usr/bin/env python3
"""Render real menus and catch missing blur or blur leaking outside panels.

Run under Xvfb with Mesa and Pillow. Each backend is tested with MSAA off/on,
including a return to blur 0 so cached backdrop images cannot mask a failure.
"""

import os
from pathlib import Path
import re
import subprocess
import sys
import time

from PIL import Image, ImageChops, ImageDraw, ImageStat


def wait_for(predicate, process, description, timeout=30):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f"Client exited ({process.returncode}) waiting for {description}")
        value = predicate()
        if value:
            return value
        time.sleep(0.1)
    raise TimeoutError(description)


def mean_difference(first, second):
    return sum(ImageStat.Stat(ImageChops.difference(first, second)).mean) / 3


def check_pair(sharp, blurred):
    assert sharp.size == blurred.size, "Unexpected resolution change"
    width, height = sharp.size
    # No panel reaches the four-pixel outer border. Even a full-screen blur
    # which looks plausible inside the menu must fail this check.
    borders = [(0, 0, width, 4), (0, height - 4, width, height),
               (0, 0, 4, height), (width - 4, 0, width, height)]
    outside = max(mean_difference(sharp.crop(box), blurred.crop(box)) for box in borders)
    assert outside < 0.2, f"Blur leaked outside panels: mean pixel change {outside:.3f}"
    center = (60, 120, width - 60, height - 80)
    inside = mean_difference(sharp.crop(center), blurred.crop(center))
    assert inside > 8, f"No visible backdrop blur inside menus: mean change {inside:.3f}"
    # Verify the sharp, detailed test background actually loaded.
    assert max(ImageStat.Stat(sharp.crop(borders[0])).stddev) > 40, "Test backdrop missing"
    print(f"Inside change={inside:.2f}, outside change={outside:.3f}", flush=True)


def run_case(binary, output, backend, samples, glass=True):
    case = output / f"{backend.lower()}-msaa{samples}{'' if glass else '-control'}"
    case.mkdir(parents=True, exist_ok=True)
    backdrop = Image.new("RGBA", (1280, 720))
    draw = ImageDraw.Draw(backdrop)
    for y in range(0, 720, 8):
        for x in range(0, 1280, 8):
            color = (225, 240, 255) if (x // 8 + y // 8) % 2 else (25, 45, 65)
            draw.rectangle((x, y, x + 7, y + 7), fill=color)
    backdrop.save(case / "backdrop.png")
    (case / "storage.cfg").write_text(f"add_path {case}\nadd_path {binary.parent / 'data'}\nadd_path {binary.parent}\n")
    fifo = case / "console.fifo"
    commands = [
        "cl_show_welcome 0", "cl_skip_start_menu 1", "ui_page 6",
        "player_name GlassTest", "snd_enable 0", "gfx_fullscreen 0",
        "gfx_screen_width 1280", "gfx_screen_height 720", "gfx_vsync 0",
        "gfx_refresh_rate 30", f"gfx_backend {backend}", f"gfx_fsaa_samples {samples}",
        "gfx_gl_major 3", "gfx_gl_minor 3", "gfx_render_thread_count 2",
        "bc_motion_blur 0", "bkw_tf_menu 0", "bc_menu_media_background 1",
        f'bc_menu_media_background_path "{case / "backdrop.png"}"',
        f"bkw_ui_glass {int(glass)}", "bkw_ui_glass_transparency 100", "bkw_ui_glass_blur 0",
        f'cl_input_fifo "{fifo}"',
    ]
    with (case / "client.log").open("w") as log:
        process = subprocess.Popen(["gdb", "-q", "-batch", "-ex", "run", "-ex", "thread apply all bt",
                                    "--args", str(binary), *commands], cwd=case, stdout=log, stderr=log)
        try:
            wait_for(fifo.exists, process, "console FIFO")

            def send(command):
                descriptor = os.open(fifo, os.O_WRONLY | os.O_NONBLOCK)
                try:
                    os.write(descriptor, (command + "\n").encode())
                finally:
                    os.close(descriptor)

            def snapshot(level, name):
                send(f"bkw_ui_glass_blur {level}")
                time.sleep(1.5)
                previous = set(case.glob("screenshots/*.png"))
                send("screenshot")
                path = wait_for(lambda: next(iter(set(case.glob("screenshots/*.png")) - previous), None),
                                process, f"{name} screenshot")
                # File can appear just before its final PNG block is written.
                def read_image():
                    try:
                        with Image.open(path) as image:
                            return image.convert("RGB")
                    except (OSError, ValueError):
                        return None
                image = wait_for(read_image, process, "complete PNG")
                image.save(case / f"{name}.png")
                return image

            if glass:
                sharp = snapshot(0, "sharp")
                blurred = snapshot(4, "blurred")
                restored = snapshot(0, "restored")
                check_pair(sharp, blurred)
                check_pair(restored, blurred)
            else:
                snapshot(0, "control")
            # Confirm the requested backend wasn't silently replaced at startup.
            text = (case / "client.log").read_text(errors="replace").lower()
            assert f"created {backend.lower()} " in text, f"Requested backend {backend} absent from log"
            send("quit")
            process.wait(timeout=20)
            assert process.returncode == 0, f"Unclean exit: {process.returncode}"
            text = (case / "client.log").read_text(errors="replace")
            assert not re.search(r"received signal SIG(?:SEGV|ABRT|BUS|ILL|FPE)", text), "Client crashed; see debugger backtrace"
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            if fifo.exists():
                fifo.unlink()
    print(f"PASS {backend}, MSAA {samples}", flush=True)


def main():
    binary = Path(sys.argv[1]).resolve()
    output = Path(sys.argv[2]).resolve()
    output.mkdir(parents=True, exist_ok=True)
    errors = []
    cases = [(backend, samples, True) for backend in ("OpenGL", "Vulkan") for samples in (0, 4)]
    cases.append(("OpenGL", 0, False))
    for backend, samples, glass in cases:
        try:
            run_case(binary, output, backend, samples, glass)
        except Exception as error:
            case = output / f"{backend.lower()}-msaa{samples}{'' if glass else '-control'}"
            errors.append(f"{case.name}: {error}")
            print(errors[-1], flush=True)
            log = case / "client.log"
            if log.exists():
                print("\n".join(log.read_text(errors="replace").splitlines()[-160:]), flush=True)
    if errors:
        raise RuntimeError("\n".join(errors))


if __name__ == "__main__":
    main()
