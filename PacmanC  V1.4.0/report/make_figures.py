#!/usr/bin/env python3
"""
make_figures.py - draw the three diagrams used by the report.

MFE113TC Group Project - text-based Pac-Man (standard C only)
Part 4 (Game State & Testing) - documentation tooling.

    python report/make_figures.py

Writes report/figures/fig1_architecture.png, fig2_tick_flow.png and
fig3_state_machine.png.  Pillow is used so the figures can be regenerated from
the source tree at any time.
"""

import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "figures")
SCALE = 2

FONT_DIR = r"C:\Windows\Fonts"


def font(name, size):
    try:
        return ImageFont.truetype(os.path.join(FONT_DIR, name), size * SCALE)
    except OSError:
        return ImageFont.load_default()


F_TITLE = font("arialbd.ttf", 14)
F_BOX = font("arialbd.ttf", 10)
F_SUB = font("arial.ttf", 9)
F_SMALL = font("arial.ttf", 8)

INK = (33, 37, 41)
BLUE, BLUE_L = (30, 90, 160), (223, 236, 248)
GREEN, GREEN_L = (28, 130, 90), (223, 244, 235)
ORANGE, ORANGE_L = (198, 108, 20), (253, 238, 220)
RED, RED_L = (176, 42, 42), (250, 232, 232)
GREY, GREY_L = (110, 118, 126), (238, 241, 244)


class Canvas:
    def __init__(self, w, h, title):
        self.w, self.h = w, h
        self.img = Image.new("RGB", (w * SCALE, h * SCALE), "white")
        self.d = ImageDraw.Draw(self.img)
        self.d.text((18 * SCALE, 10 * SCALE), title, font=F_TITLE, fill=INK)
        self.d.line([(18 * SCALE, 33 * SCALE), ((w - 18) * SCALE, 33 * SCALE)],
                    fill=(205, 210, 215), width=SCALE)

    def box(self, x, y, w, h, lines, fill=GREY_L, edge=GREY):
        d = self.d
        d.rounded_rectangle([x * SCALE, y * SCALE, (x + w) * SCALE, (y + h) * SCALE],
                            radius=7 * SCALE, fill=fill, outline=edge, width=SCALE + 1)
        line_h = 14
        start = y + h / 2 - (len(lines) * line_h) / 2 + 1
        for i, text in enumerate(lines):
            use = F_BOX if i == 0 else F_SUB
            tw = d.textlength(text, font=use)
            d.text(((x + w / 2) * SCALE - tw / 2, (start + i * line_h) * SCALE),
                   text, font=use, fill=INK)

    def diamond(self, cx, cy, w, h, lines):
        d = self.d
        d.polygon([(cx * SCALE, (cy - h / 2) * SCALE),
                   ((cx + w / 2) * SCALE, cy * SCALE),
                   (cx * SCALE, (cy + h / 2) * SCALE),
                   ((cx - w / 2) * SCALE, cy * SCALE)],
                  fill=GREY_L, outline=GREY, width=SCALE + 1)
        line_h = 13
        start = cy - (len(lines) * line_h) / 2 + 1
        for i, text in enumerate(lines):
            use = F_BOX if i == 0 else F_SMALL
            tw = d.textlength(text, font=use)
            d.text((cx * SCALE - tw / 2, (start + i * line_h) * SCALE), text,
                   font=use, fill=INK)

    def arrow(self, points, label="", color=GREY, label_at=None):
        d = self.d
        pts = [(p[0] * SCALE, p[1] * SCALE) for p in points]
        d.line(pts, fill=color, width=SCALE + 1, joint="curve")
        (x1, y1), (x2, y2) = points[-2], points[-1]
        if abs(x2 - x1) >= abs(y2 - y1):
            s = 1 if x2 > x1 else -1
            head = [(x2 * SCALE, y2 * SCALE),
                    ((x2 - 8 * s) * SCALE, (y2 - 4) * SCALE),
                    ((x2 - 8 * s) * SCALE, (y2 + 4) * SCALE)]
        else:
            s = 1 if y2 > y1 else -1
            head = [(x2 * SCALE, y2 * SCALE),
                    ((x2 - 4) * SCALE, (y2 - 8 * s) * SCALE),
                    ((x2 + 4) * SCALE, (y2 - 8 * s) * SCALE)]
        d.polygon(head, fill=color)
        if label:
            lx, ly = label_at if label_at else ((x1 + x2) / 2, (y1 + y2) / 2)
            tw = d.textlength(label, font=F_SMALL)
            d.rectangle([lx * SCALE - tw / 2 - 3 * SCALE, (ly - 7) * SCALE,
                         lx * SCALE + tw / 2 + 3 * SCALE, (ly + 7) * SCALE],
                        fill="white")
            d.text((lx * SCALE - tw / 2, (ly - 6) * SCALE), label,
                   font=F_SMALL, fill=color)

    def note(self, x, y, text, color=INK):
        self.d.text((x * SCALE, y * SCALE), text, font=F_SMALL, fill=color)

    def save(self, name):
        path = os.path.join(OUT, name)
        self.img.save(path)
        return path


# ----------------------------------------------------------------- figure 1
def figure_architecture():
    c = Canvas(900, 400, "Figure 1 - module architecture (owner part in brackets)")

    c.box(350, 50, 200, 46, ["main.c", "CLI, tick loop, summary  [Part 4]"], BLUE_L, BLUE)

    c.box(50, 160, 180, 48, ["input.c / input.h", "keys, EOF  [Part 2]"], ORANGE_L, ORANGE)
    c.box(350, 160, 200, 48, ["game.c / game.h", "state, rules, tick order  [Part 4+2]"], BLUE_L, BLUE)
    c.box(670, 160, 180, 48, ["render.c / render.h", "ASCII board, status  [Part 1]"], GREEN_L, GREEN)

    c.arrow([(410, 96), (250, 118), (165, 158)])
    c.arrow([(450, 96), (450, 158)])
    c.arrow([(490, 96), (700, 118), (760, 158)])

    modules = [
        (40, "map.c / map.h", "board, maps, checks  [Part 1]", GREEN_L, GREEN),
        (255, "entity.c / entity.h", "positions, movement  [Part 2]", ORANGE_L, ORANGE),
        (470, "ghost.c / ghost.h", "AI, scared, modes  [Part 3]", ORANGE_L, ORANGE),
        (685, "rng.c / rng.h", "xorshift32, seed  [Part 4]", BLUE_L, BLUE),
    ]
    for x, title, sub, fill, edge in modules:
        c.box(x, 285, 175, 48, [title, sub], fill, edge)
        c.arrow([(450, 208), (450, 250), (x + 87, 250), (x + 87, 283)])

    c.note(40, 350, "Arrows = direction of the calls.  main.c drives the loop and calls game.c, input.c and render.c;")
    c.note(40, 364, "game.c uses the four service modules below it.  The .h files were frozen before parallel work started.")
    c.note(40, 378, "Colours: green = Part 1 (map, render), orange = Part 2 / Part 3 (entities, input, ghosts), blue = Part 4.", GREY)

    c.save("fig1_architecture.png")


# ----------------------------------------------------------------- figure 2
def figure_tick_flow():
    c = Canvas(1100, 770, "Figure 2 - what happens inside one tick (game_tick)")
    cx = 560

    steps = [
        ("1  read one key", "INPUT_MOVE / QUIT / EOF / invalid key"),
        ("2  Pac-Man moves one cell", "a wall keeps it in place, the tick still advances"),
        ("3  eat pellet / power pellet / fruit", "score, pelletsRemaining, scared timer"),
    ]
    y = 55
    for i, (title, sub) in enumerate(steps):
        c.box(cx - 210, y, 420, 44, [title, sub], BLUE_L, BLUE)
        if i:
            c.arrow([(cx, y - 20), (cx, y - 2)])
        y += 64

    c.diamond(cx, y + 26, 340, 74, ["collision?", "Pac-Man and a normal ghost on one cell"])
    c.arrow([(cx, y - 20), (cx, y - 11)])
    c.arrow([(cx + 170, y + 26), (cx + 235, y + 26)], "yes", RED)
    c.box(cx + 240, y, 250, 52, ["lose one life", "respawn, pellets stay eaten"], RED_L, RED)
    y += 76

    c.diamond(cx, y + 30, 360, 78, ["all pellets eaten?", "pelletsRemaining == 0"])
    c.arrow([(cx, y - 16), (cx, y - 9)])
    c.arrow([(cx - 180, y + 30), (cx - 250, y + 30)], "yes", GREEN)
    c.box(cx - 500, y + 4, 245, 52,
          ["next level, or WIN", "level < levels ? load level+1 : GAME_WIN"], GREEN_L, GREEN)
    y += 82

    c.box(cx - 210, y, 420, 44,
          ["5  timers and step 4 result", "scared countdown, chase/scatter, fruit, lives == 0 -> LOSE"],
          BLUE_L, BLUE)
    c.arrow([(cx, y - 16), (cx, y - 2)])

    y += 64
    c.box(cx - 210, y, 420, 44, ["6  every ghost moves exactly once", "ghost.c picks one legal cell"],
          BLUE_L, BLUE)
    c.arrow([(cx, y - 20), (cx, y - 2)])

    y += 64
    c.diamond(cx, y + 26, 340, 74, ["collision?", "a ghost walked onto Pac-Man"])
    c.arrow([(cx, y - 20), (cx, y - 11)])
    c.arrow([(cx + 170, y + 26), (cx + 235, y + 26)], "yes", RED)
    c.box(cx + 240, y, 250, 52, ["lose one life (again)", "same rule as above"], RED_L, RED)

    y += 76
    c.box(cx - 210, y, 420, 44, ["7  tick++ and render the board", "status: score, lives, pellets, tick"],
          BLUE_L, BLUE)
    c.arrow([(cx, y - 18), (cx, y - 2)])

    c.note(30, y + 62, "The whole sequence repeats until the game returns WIN, LOSE or QUIT.")
    c.note(30, y + 76, "The win check happens BEFORE the ghosts move, so the last pellet can never be taken away by an unlucky ghost step.")
    c.note(30, y + 90, "Step 3/4 and step 7 are the two collision points of the brief: Pac-Man walks into a ghost, or a ghost walks into Pac-Man.", GREY)

    c.save("fig2_tick_flow.png")


# ----------------------------------------------------------------- figure 3
def figure_state_machine():
    c = Canvas(900, 480, "Figure 3 - game state machine")

    c.box(60, 180, 150, 46, ["INIT", "flags, seed, level 1"], GREY_L, GREY)
    c.box(350, 180, 200, 50, ["RUNNING", "one tick per key"], BLUE_L, BLUE)
    c.box(150, 60, 180, 46, ["PAC_DIED", "lives--, respawn"], ORANGE_L, ORANGE)
    c.box(60, 300, 150, 46, ["QUIT", "q or EOF"], GREY_L, GREY)
    c.box(620, 60, 200, 46, ["WIN", "no pellet left (last level)"], GREEN_L, GREEN)
    c.box(620, 180, 200, 46, ["LEVEL_CLEAR", "load the next map"], GREEN_L, GREEN)
    c.box(620, 300, 200, 46, ["LOSE", "lives == 0"], RED_L, RED)

    c.arrow([(210, 203), (348, 205)], "start", label_at=(278, 192))
    c.arrow([(390, 180), (300, 108)], "collision", label_at=(300, 146))
    c.arrow([(240, 106), (240, 150), (470, 150), (470, 178)],
            "lives > 0: respawn", label_at=(355, 138))
    c.arrow([(150, 83), (30, 83), (30, 440), (720, 440), (720, 348)],
            "lives == 0", label_at=(300, 428), color=RED)
    c.arrow([(550, 196), (618, 196)], "pellets == 0,", label_at=(584, 184))
    c.arrow([(618, 214), (550, 214)], "next map loaded", label_at=(584, 228), color=GREEN)
    c.arrow([(520, 180), (700, 108)], "pellets == 0, last level", label_at=(648, 140), color=GREEN)
    c.arrow([(390, 230), (390, 323), (212, 323)], "", color=GREY)
    c.note(283, 311, "quit command", GREY)

    c.save("fig3_state_machine.png")


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    figure_architecture()
    figure_tick_flow()
    figure_state_machine()
    print("figures written to", OUT)
