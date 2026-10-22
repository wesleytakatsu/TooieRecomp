#!/usr/bin/env python3
"""Generate ui_assets/TooieLogo.svg ("Banjo-Tooie" / "Recompiled" launcher wordmark).

Reuses the per-letter vector groups of the BanjoRecomp launcher Logo.svg
(read-only input) and adds a hand-drawn vector T. Pure vector output, no
raster or ROM data. Usage: make_tooie_launcher_logo.py [Logo.svg] [out.svg]
"""
import sys
import xml.etree.ElementTree as ET

NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", NS)
Q = "{%s}" % NS

RED, BLUE, YELLOW = "rgb(255,10,1)", "rgb(0,16,193)", "rgb(246,206,39)"
W, H = 7300, 2450  # viewBox; launcher_animation.cpp size literals must match

# Letter bounding boxes (x, y, w, h) in the source viewBox space (inkscape --query-all).
# Indices are child positions inside the source "Banjo" / "Recompiled" groups.
BANJO = {"B": 5, "A": 2, "N": 3, "j": 4, "O": 1, "-": 0}
BANJO_BOX = {0: (3897.82, 892.68, 528, 436.6), 1: (2750.45, 490.99, 1118.6, 971.1),
             2: (641.21, 322.88, 1142.3, 1128.6), 3: (1495.65, 157.71, 1110.2, 1154.4),
             4: (2383.74, 15.0, 554, 1582.6), 5: (3.45, 519.27, 950.4, 1040.3)}
RECOMP_BOX = {9: (770.97, 1619.05, 860.4, 810.2), 2: (1479.54, 1538.2, 463.9, 723.0),
              4: (1894.94, 1451.29, 544.6, 810.0), 1: (2295.86, 1640.5, 801.1, 727.3),
              8: (2922.06, 1728.45, 1023.9, 805.9), 7: (3722.37, 1611.24, 666.1, 790.9),
              6: (4265.79, 1355.0, 490.4, 998.3), 5: (4619.97, 1692.74, 561.7, 730.8),
              3: (5029.94, 1525.61, 552.0, 775.0), 0: (5497.51, 1426.47, 675.0, 821.3)}
RECOMP_ORDER = [9, 2, 4, 1, 8, 7, 6, 5, 3, 0]  # R E C O M P I L E D, left to right


def place(parent_chain, group, box, x, y, s, recolor=None):
    """Wrap a source letter group so its bbox top-left lands at (x, y) scaled by s."""
    bx, by = box[0], box[1]
    # translate then scale: p -> T + s*p, so the bbox origin maps to (x, y) when T = (x, y) - s*b.
    outer = ET.Element(Q + "g", transform="translate(%.2f,%.2f) scale(%.4f)" % (x - s * bx, y - s * by, s))
    cur = outer
    for t in parent_chain:
        g = ET.SubElement(cur, Q + "g", transform=t)
        cur = g
    letter = ET.fromstring(ET.tostring(group))
    if recolor:
        for p in letter.iter(Q + "path"):
            if RED in (p.get("style") or ""):
                p.set("style", p.get("style").replace(RED, recolor))
    cur.append(letter)
    return outer


def poly_glyph(points, x, y, s, fill, outline=30):
    """Hand-drawn polygon glyph: yellow outset outline under a flat fill."""
    d = "M" + "L".join("%.1f,%.1f" % (px, py) for px, py in points) + "Z"
    g = ET.Element(Q + "g", transform="translate(%.2f,%.2f) scale(%.4f)" % (x, y, s))
    ET.SubElement(g, Q + "path", d=d, style="fill:%s;stroke:%s;stroke-width:%d;stroke-linejoin:round;" % (YELLOW, YELLOW, outline * 2))
    ET.SubElement(g, Q + "path", d=d, style="fill:%s;" % fill)
    return g


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else "assets/Logo.svg"
    out = sys.argv[2] if len(sys.argv) > 2 else "ui_assets/TooieLogo.svg"
    root = ET.parse(src).getroot()
    flat = next(e for e in root.iter(Q + "g") if e.get("id") == "Logo_Straight_Flat")
    banjo = next(e for e in flat if e.get("id") == "Banjo")
    recomp = next(e for e in flat if e.get("id") == "Recompiled")
    chain_b = [flat.get("transform"), banjo.get("transform")]
    chain_r = [flat.get("transform"), recomp.get("transform")]

    svg = ET.Element(Q + "svg", width="100%", height="100%", viewBox="0 0 %d %d" % (W, H), version="1.1",
                     style="fill-rule:evenodd;clip-rule:evenodd;stroke-linejoin:round;stroke-miterlimit:2;")
    s1, s2 = 0.835, 0.9
    ox1 = 100  # left margin of line 1
    pad = 40  # top margin
    root_g = ET.SubElement(svg, Q + "g", id="Banjo-Tooie_Recompiled")

    # Line 1: "Banjo-" keeps the source layout; draw order = source DOM order (hyphen, O, A, N, j, B).
    line1 = ET.SubElement(root_g, Q + "g", id="Banjo-Tooie")
    for idx in (0, 1, 2, 3, 4, 5):
        bx, by, _, _ = BANJO_BOX[idx]
        line1.append(place(chain_b, banjo[idx], BANJO_BOX[idx], ox1 + s1 * bx, pad + s1 * by, s1))

    # "Tooie": T hand drawn, O O copied from the source O, i / e recolored source I / E.
    tx = 4420
    t_pts = [(0, 0), (830, -20), (850, 250), (560, 262), (540, 1000), (250, 1010), (270, 268), (-10, 262)]
    line1.append(poly_glyph(t_pts, ox1 + s1 * tx, pad + s1 * 300, s1, BLUE))
    ox = 5160
    for x, y in ((ox + 1020, 520), (ox, 480)):  # later O first so the earlier letter sits on top
        line1.append(place(chain_b, banjo[1], BANJO_BOX[1], ox1 + s1 * x, pad + s1 * y, s1))
    # E, then i, scaled to match the Banjo letter height; drawn right-to-left like the source.
    sc = 1.25
    line1.append(place(chain_r, recomp[3], RECOMP_BOX[3], ox1 + s1 * 7813, pad + s1 * 560, s1 * sc, BLUE))
    line1.append(place(chain_r, recomp[6], RECOMP_BOX[6], ox1 + s1 * 7240, pad + s1 * 250, s1 * sc, BLUE))

    # Line 2: "Recompiled" from the source letters (no leading hyphen), centered under line 1.
    line2 = ET.SubElement(root_g, Q + "g", id="Recompiled")
    x0 = RECOMP_BOX[9][0]
    span = (RECOMP_BOX[0][0] + RECOMP_BOX[0][2] - x0) * s2
    left = (W - span) / 2
    y_off = 1180
    for idx in RECOMP_ORDER:
        bx, by, _, _ = RECOMP_BOX[idx]
        line2.append(place(chain_r, recomp[idx], RECOMP_BOX[idx], left + (bx - x0) * s2, pad + y_off + (by - 1355.0) * s2, s2))

    ET.ElementTree(svg).write(out, encoding="utf-8", xml_declaration=True)


if __name__ == "__main__":
    main()
