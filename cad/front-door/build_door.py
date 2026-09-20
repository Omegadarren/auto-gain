"""
Parametric 3D front-door assembly matching the reference photo.

Known: main door panel = 36 in x 80 in.
Photo-derived layout: 6-lite (2x3) over one recessed panel, matching
sidelights (1x3), continuous lock rail, exterior handle on viewer-left.

Coordinate system: X = width, Y = thickness, Z = height.

Exports STEP for OnShape (Insert → STEP / File → Import).
"""

from __future__ import annotations

from pathlib import Path

import cadquery as cq

# --- Parameters (inches) -----------------------------------------------------

DOOR_W = 36.0
DOOR_H = 80.0
THICK = 1.75

STILE = 4.5
TOP_RAIL = 5.0
BOTTOM_RAIL = 9.0
LOCK_RAIL = 4.5
MUNTIN = 1.125

SL_W = 12.0
SL_STILE = 3.0
MULLION_W = 3.5

LOCK_CENTER_FROM_TOP = 0.72

PANEL_RECESS = 0.45
PANEL_INSET = 1.0
GLASS_THICK = 0.25

HANDLE_FROM_LATCH_EDGE = 3.0
HANDLE_Z0 = 36.0
HANDLE_Z1 = 52.0
HANDLE_W = 1.1
HANDLE_D = 1.55
HANDLE_BACK_W = 1.55
HANDLE_BACK_D = 0.12
HANDLE_BACK_EXTRA = 2.0

CASING_W = 4.5
CASING_THICK = 0.75
CASING_GAP = 0.2

OUT_DIR = Path(__file__).resolve().parent / "exports"


def lock_band(height: float) -> tuple[float, float]:
    center = height * (1.0 - LOCK_CENTER_FROM_TOP)
    return center - LOCK_RAIL / 2.0, center + LOCK_RAIL / 2.0


def lite_rects(
    origin_x: float,
    origin_z: float,
    field_w: float,
    field_h: float,
    cols: int,
    rows: int,
    muntin: float,
) -> list[tuple[float, float, float, float]]:
    """Lite rectangles as (x, z, w, h) in lower-left slab coordinates (0..W, 0..H)."""
    lite_w = (field_w - (cols - 1) * muntin) / cols
    lite_h = (field_h - (rows - 1) * muntin) / rows
    rects = []
    for r in range(rows):
        for c in range(cols):
            x = origin_x + c * (lite_w + muntin)
            z = origin_z + (rows - 1 - r) * (lite_h + muntin)
            rects.append((x, z, lite_w, lite_h))
    return rects


def box_xz(cx: float, cz: float, w: float, h: float, y_len: float, y_center: float = 0.0) -> cq.Workplane:
    """Axis-aligned box centered at (cx, y_center, cz)."""
    return (
        cq.Workplane("XZ")
        .workplane(offset=y_center)
        .center(cx, cz)
        .box(w, h, y_len)
    )


def make_slab(
    width: float,
    height: float,
    stile_l: float,
    stile_r: float,
    glass_cols: int,
    glass_rows: int,
) -> tuple[cq.Workplane, list[cq.Workplane]]:
    lock_bottom, lock_top = lock_band(height)
    glass_z0 = lock_top
    glass_z1 = height - TOP_RAIL
    glass_h = glass_z1 - glass_z0
    glass_x0 = stile_l
    glass_w = width - stile_l - stile_r

    panel_z0 = BOTTOM_RAIL
    panel_h = lock_bottom - BOTTOM_RAIL

    # Blank centered at origin
    slab = cq.Workplane("XZ").box(width, height, THICK)

    def to_center(x_ll: float, z_ll: float, w: float, h: float):
        return x_ll + w / 2.0 - width / 2.0, z_ll + h / 2.0 - height / 2.0

    rects = lite_rects(glass_x0, glass_z0, glass_w, glass_h, glass_cols, glass_rows, MUNTIN)
    for x, z, w, h in rects:
        cx, cz = to_center(x, z, w, h)
        slab = slab.cut(box_xz(cx, cz, w, h, THICK + 0.4))

    # Recessed panel pockets on both faces
    if glass_w > 2 * PANEL_INSET + 1 and panel_h > 2 * PANEL_INSET + 1:
        pw = glass_w - 2 * PANEL_INSET
        ph = panel_h - 2 * PANEL_INSET
        px = glass_x0 + PANEL_INSET
        pz = panel_z0 + PANEL_INSET
        cx, cz = to_center(px, pz, pw, ph)
        for y_c in (
            THICK / 2 - PANEL_RECESS / 2,
            -(THICK / 2 - PANEL_RECESS / 2),
        ):
            slab = slab.cut(box_xz(cx, cz, pw, ph, PANEL_RECESS + 0.05, y_center=y_c))

    glasses: list[cq.Workplane] = []
    for x, z, w, h in rects:
        cx, cz = to_center(x, z, w, h)
        glasses.append(box_xz(cx, cz, w - 0.06, h - 0.06, GLASS_THICK))

    return slab, glasses


def make_handle() -> cq.Workplane:
    zc = (HANDLE_Z0 + HANDLE_Z1) / 2.0 - DOOR_H / 2.0
    back = box_xz(
        0.0,
        zc,
        HANDLE_BACK_W,
        (HANDLE_Z1 - HANDLE_Z0) + HANDLE_BACK_EXTRA,
        HANDLE_BACK_D,
        y_center=THICK / 2 + HANDLE_BACK_D / 2,
    )
    bar = box_xz(
        0.0,
        zc,
        HANDLE_W,
        HANDLE_Z1 - HANDLE_Z0,
        HANDLE_D,
        y_center=THICK / 2 + HANDLE_BACK_D + HANDLE_D / 2,
    )
    return back.union(bar)


def make_mullion() -> cq.Workplane:
    return cq.Workplane("XZ").box(MULLION_W, DOOR_H, THICK + 0.4)


def make_casing(unit_w: float) -> cq.Workplane:
    outer_w = unit_w + 2 * CASING_W
    outer_h = DOOR_H + 2 * CASING_W
    y_c = THICK / 2 + CASING_GAP + CASING_THICK / 2
    outer = box_xz(0, 0, outer_w, outer_h, CASING_THICK, y_center=y_c)
    inner = box_xz(0, 0, unit_w, DOOR_H, CASING_THICK + 0.2, y_center=y_c)
    return outer.cut(inner)


def build_assembly():
    door, door_glass = make_slab(DOOR_W, DOOR_H, STILE, STILE, 2, 3)
    sl_proto, sl_glass_proto = make_slab(SL_W, DOOR_H, SL_STILE, SL_STILE, 1, 3)

    unit_w = SL_W + MULLION_W + DOOR_W + MULLION_W + SL_W
    x0 = -unit_w / 2.0
    x_sl_l = x0 + SL_W / 2.0
    x_mull_l = x0 + SL_W + MULLION_W / 2.0
    x_door = x0 + SL_W + MULLION_W + DOOR_W / 2.0
    x_mull_r = x0 + SL_W + MULLION_W + DOOR_W + MULLION_W / 2.0
    x_sl_r = unit_w / 2.0 - SL_W / 2.0

    def moved(solid: cq.Workplane, x: float) -> cq.Workplane:
        return solid.translate((x, 0, 0))

    door_m = moved(door, x_door)
    sl_l = moved(sl_proto, x_sl_l)
    sl_r = moved(sl_proto, x_sl_r)
    mull_l = moved(make_mullion(), x_mull_l)
    mull_r = moved(make_mullion(), x_mull_r)
    handle = make_handle().translate(
        (x_door - DOOR_W / 2.0 + HANDLE_FROM_LATCH_EDGE, 0, 0)
    )
    casing = make_casing(unit_w)

    door_g = [moved(g, x_door) for g in door_glass]
    sl_l_g = [moved(g, x_sl_l) for g in sl_glass_proto]
    sl_r_g = [moved(g, x_sl_r) for g in sl_glass_proto]

    parts: dict[str, cq.Workplane] = {
        "door": door_m,
        "sidelight_left": sl_l,
        "sidelight_right": sl_r,
        "mullion_left": mull_l,
        "mullion_right": mull_r,
        "handle": handle,
        "casing": casing,
    }
    for i, g in enumerate(door_g):
        parts[f"door_glass_{i}"] = g
    for i, g in enumerate(sl_l_g):
        parts[f"sl_left_glass_{i}"] = g
    for i, g in enumerate(sl_r_g):
        parts[f"sl_right_glass_{i}"] = g

    fused = door_m
    for key in (
        "sidelight_left",
        "sidelight_right",
        "mullion_left",
        "mullion_right",
        "handle",
    ):
        fused = fused.union(parts[key])
    for g in door_g + sl_l_g + sl_r_g:
        fused = fused.union(g)
    fused_with_casing = fused.union(casing)

    lock_bottom, lock_top = lock_band(DOOR_H)
    meta = {
        "unit_w": unit_w,
        "lock_bottom": lock_bottom,
        "lock_top": lock_top,
        "glass_h": (DOOR_H - TOP_RAIL) - lock_top,
        "panel_h": lock_bottom - BOTTOM_RAIL,
    }
    return parts, fused, fused_with_casing, meta


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    parts, fused, fused_with_casing, meta = build_assembly()

    step_unit = OUT_DIR / "front_door_unit.step"
    step_full = OUT_DIR / "front_door_with_casing.step"
    stl_path = OUT_DIR / "front_door_unit.stl"
    parts_path = OUT_DIR / "front_door_parts.step"

    cq.exporters.export(fused, str(step_unit))
    cq.exporters.export(fused_with_casing, str(step_full))
    cq.exporters.export(fused, str(stl_path))

    asm = cq.Assembly()
    for name, solid in parts.items():
        asm.add(solid, name=name)
    asm.save(str(parts_path))

    bb = fused.val().BoundingBox()
    (OUT_DIR / "dimensions.txt").write_text(
        "\n".join(
            [
                "Front door assembly (inches)",
                f"Door panel: {DOOR_W} x {DOOR_H} x {THICK}",
                f"Sidelight each: {SL_W} x {DOOR_H} x {THICK}",
                f"Mullion width: {MULLION_W}",
                f"Overall unit width (no casing): {meta['unit_w']}",
                f"Overall with {CASING_W}\" casing: {meta['unit_w'] + 2 * CASING_W} x {DOOR_H + 2 * CASING_W}",
                f"Lock rail: {meta['lock_bottom']:.2f} .. {meta['lock_top']:.2f} from bottom",
                f"Glass field height: {meta['glass_h']:.2f}",
                f"Lower panel field height: {meta['panel_h']:.2f}",
                "Door lites: 2 x 3",
                "Sidelight lites: 1 x 3 (horizontals aligned to door)",
                f"Bounding box: X[{bb.xmin:.2f},{bb.xmax:.2f}] Y[{bb.ymin:.2f},{bb.ymax:.2f}] Z[{bb.zmin:.2f},{bb.zmax:.2f}]",
                "",
                "OnShape import:",
                "  1. New document → Part Studio (or Assemblies)",
                "  2. Insert / Import → STEP → front_door_unit.step",
                "     (or front_door_with_casing.step / front_door_parts.step)",
                "  3. Optional native FeatureScript: onshape/FrontDoor.fs",
            ]
        )
        + "\n"
    )
    print(f"unit width = {meta['unit_w']} in")
    print(f"bbox X[{bb.xmin:.2f},{bb.xmax:.2f}] Y[{bb.ymin:.2f},{bb.ymax:.2f}] Z[{bb.zmin:.2f},{bb.zmax:.2f}]")
    print(f"wrote {step_unit}")
    print(f"wrote {step_full}")
    print(f"wrote {stl_path}")
    print(f"wrote {parts_path}")


if __name__ == "__main__":
    main()
