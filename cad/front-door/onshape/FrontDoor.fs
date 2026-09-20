FeatureScript 2293;
/**
 * OPTIONAL native OnShape feature (experimental).
 * Prefer importing exports/front_door_unit.step — that is the verified model.
 *
 * Door panel: 36" x 80". Sidelights 12" each. 6-lite over 1-panel.
 *
 * If you still want FeatureScript:
 *  1. Part Studio → Add custom feature → New Feature Studio
 *  2. Paste this file, commit, insert "Front door unit"
 */
import(path : "onshape/std/common.fs", version : "2293.0");

annotation { "Feature Type Name" : "Front door unit" }
export const frontDoorUnit = defineFeature(function(context is Context, id is Id, definition is map)
    precondition
    {
        annotation { "Name" : "Door width" }
        isLength(definition.doorWidth, { (inch) : [24, 36, 48] } as LengthBoundSpec);

        annotation { "Name" : "Door height" }
        isLength(definition.doorHeight, { (inch) : [70, 80, 96] } as LengthBoundSpec);

        annotation { "Name" : "Sidelight width" }
        isLength(definition.sidelightWidth, { (inch) : [8, 12, 18] } as LengthBoundSpec);

        annotation { "Name" : "Thickness" }
        isLength(definition.thickness, { (inch) : [1.375, 1.75, 2.25] } as LengthBoundSpec);

        annotation { "Name" : "Include casing" }
        definition.includeCasing is boolean;
    }
    {
        const doorW = definition.doorWidth;
        const doorH = definition.doorHeight;
        const slW = definition.sidelightWidth;
        const thick = definition.thickness;

        const stile = 4.5 * inch;
        const slStile = 3.0 * inch;
        const topRail = 5.0 * inch;
        const bottomRail = 9.0 * inch;
        const lockRail = 4.5 * inch;
        const muntin = 1.125 * inch;
        const mullionW = 3.5 * inch;
        const lockCenterFromTop = 0.72;

        const lockCenter = doorH * (1 - lockCenterFromTop);
        const lockBottom = lockCenter - lockRail / 2;
        const lockTop = lockCenter + lockRail / 2;

        const unitW = slW + mullionW + doorW + mullionW + slW;

        // Base plane sketch for door blank, then extrude — build as separate bodies
        // then boolean-union via fCutouts on each slab.

        // --- helpers via sketches on Front plane ---
        const front = qCreatedBy(makeId("Front"), EntityType.FACE);

        // Build unit as one sketch outline + extrude, then cut lites.
        // Simpler approach: extrude full rectangle unit, cut openings.

        var sketchId = id + "unitSketch";
        var sketch = newSketchOnPlane(context, sketchId, {
            "sketchPlane" : plane(vector(0, 0, 0) * inch, vector(0, 0, 1))
        });
        // Coordinate: X right, Y up. Unit centered on X.
        const x0 = -unitW / 2;
        // Outer unit rectangle
        skRectangle(sketch, "outer", {
            "firstCorner" : vector(x0, 0 * inch),
            "secondCorner" : vector(x0 + unitW, doorH)
        });
        skSolve(sketch);

        extrude(context, id + "unitExtrude", {
            "entities" : qSketchRegion(sketchId),
            "endBound" : BoundingType.BLIND,
            "depth" : thick
        });

        // Cut glass lites for door and sidelights
        cutLites(context, id + "doorLites", {
            "x0" : x0 + slW + mullionW + stile,
            "z0" : lockTop,
            "fieldW" : doorW - 2 * stile,
            "fieldH" : doorH - topRail - lockTop,
            "cols" : 2,
            "rows" : 3,
            "muntin" : muntin,
            "thick" : thick
        });

        cutLites(context, id + "slLeftLites", {
            "x0" : x0 + slStile,
            "z0" : lockTop,
            "fieldW" : slW - 2 * slStile,
            "fieldH" : doorH - topRail - lockTop,
            "cols" : 1,
            "rows" : 3,
            "muntin" : muntin,
            "thick" : thick
        });

        cutLites(context, id + "slRightLites", {
            "x0" : x0 + slW + mullionW + doorW + mullionW + slStile,
            "z0" : lockTop,
            "fieldW" : slW - 2 * slStile,
            "fieldH" : doorH - topRail - lockTop,
            "cols" : 1,
            "rows" : 3,
            "muntin" : muntin,
            "thick" : thick
        });

        // Recess lower panels (door + sidelights) — shallow extrude-remove both sides
        recessPanel(context, id + "doorPanel", {
            "x0" : x0 + slW + mullionW + stile + 1 * inch,
            "z0" : bottomRail + 1 * inch,
            "w" : doorW - 2 * stile - 2 * inch,
            "h" : lockBottom - bottomRail - 2 * inch,
            "thick" : thick,
            "recess" : 0.45 * inch
        });

        // Mullion grooves between SL and door (visual separation): cut shallow channels on front
        // Skipped — mullions are solid regions of the same extrusion (no glass there).

        if (definition.includeCasing)
        {
            const casingW = 4.5 * inch;
            const casingT = 0.75 * inch;
            var cSketch = newSketchOnPlane(context, id + "casingSketch", {
                "sketchPlane" : plane(vector(0, 0, thick + 0.2 * inch), vector(0, 0, 1))
            });
            skRectangle(cSketch, "cOuter", {
                "firstCorner" : vector(x0 - casingW, 0 * inch - casingW),
                "secondCorner" : vector(x0 + unitW + casingW, doorH + casingW)
            });
            skRectangle(cSketch, "cInner", {
                "firstCorner" : vector(x0, 0 * inch),
                "secondCorner" : vector(x0 + unitW, doorH)
            });
            skSolve(cSketch);
            extrude(context, id + "casingExtrude", {
                "entities" : qSketchRegion(id + "casingSketch", true),
                "endBound" : BoundingType.BLIND,
                "depth" : casingT
            });
        }
    }, { includeCasing: false });

function cutLites(context is Context, id is Id, p is map)
{
    const liteW = (p.fieldW - (p.cols - 1) * p.muntin) / p.cols;
    const liteH = (p.fieldH - (p.rows - 1) * p.muntin) / p.rows;
    var i = 0;
    for (var r = 0; r < p.rows; r += 1)
    {
        for (var c = 0; c < p.cols; c += 1)
        {
            const x = p.x0 + c * (liteW + p.muntin);
            const z = p.z0 + (p.rows - 1 - r) * (liteH + p.muntin);
            var sk = newSketchOnPlane(context, id + unstableIdComponent(i) + "sk", {
                "sketchPlane" : plane(vector(0, 0, 0) * meter, vector(0, 0, 1))
            });
            // plane Z-normal means sketch in XY — but our height is Y in sketch.
            skRectangle(sk, "lite", {
                "firstCorner" : vector(x, z),
                "secondCorner" : vector(x + liteW, z + liteH)
            });
            skSolve(sk);
            opExtrude(context, id + unstableIdComponent(i) + "cut", {
                "entities" : qSketchRegion(id + unstableIdComponent(i) + "sk"),
                "faceEnd" : qNothing(),
                "endBound" : BoundingType.BLIND,
                "depth" : p.thick + 0.1 * inch,
                "operationType" : NewBodyOperationType.REMOVE
            });
            i += 1;
        }
    }
}

function recessPanel(context is Context, id is Id, p is map)
{
    if (p.w < 1 * inch || p.h < 1 * inch)
    {
        return;
    }
    var sk = newSketchOnPlane(context, id + "sk", {
        "sketchPlane" : plane(vector(0, 0, p.thick), vector(0, 0, 1))
    });
    skRectangle(sk, "panel", {
        "firstCorner" : vector(p.x0, p.z0),
        "secondCorner" : vector(p.x0 + p.w, p.z0 + p.h)
    });
    skSolve(sk);
    opExtrude(context, id + "cut", {
        "entities" : qSketchRegion(id + "sk"),
        "endBound" : BoundingType.BLIND,
        "depth" : p.recess,
        "operationType" : NewBodyOperationType.REMOVE
    });
}
