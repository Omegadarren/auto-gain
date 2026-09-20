# Front door 3D model (OnShape-ready)

Reference photo of a black 6-lite entry door with matching sidelights.
Known size: **door panel 36" × 80"**.

## What you get

| File | Use |
|------|-----|
| `exports/front_door_unit.step` | **Import this into OnShape** (door + sidelights + mullions + handle + glass) |
| `exports/front_door_with_casing.step` | Same unit plus white exterior casing |
| `exports/front_door_parts.step` | Separate bodies (door, sidelights, glass, etc.) |
| `exports/front_door_unit.stl` | Quick mesh preview |
| `onshape/FrontDoor.fs` | Optional native FeatureScript (parametric Part Studio) |
| `reference/door-photo.jpg` | Source photo |
| `build_door.py` | Regenerates exports (`python3 build_door.py`) |

## Assumed dimensions (inches)

- Door: **36 × 80 × 1.75**
- Each sidelight: **12 × 80 × 1.75** (photo proportion; change in `build_door.py` if needed)
- Mullions between door and sidelights: **3.5** wide
- Overall unit width (no casing): **67**
- Door lites: **2 × 3**; sidelights: **1 × 3** (rails aligned)
- Exterior pull handle on viewer-left (as in photo)

## Import into OnShape

1. Open [cad.onshape.com](https://cad.onshape.com) (any device — phone/iPad/desktop).
2. **Create** → Document → blank Part Studio (or Assembly).
3. **Insert** → **STEP** (or drag the file in) → choose `front_door_unit.step`.
4. Scale should already be inches; if OnShape asks for units, pick **inch**.

Optional: paste `onshape/FrontDoor.fs` into a custom Feature Studio for a parametric rebuild.

## Regenerate locally

```bash
pip install cadquery
python3 cad/front-door/build_door.py
```
