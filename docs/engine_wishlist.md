# Engine Wishlist

This is the direction for `Whiteboard`: a whiteboard-style animation engine driven by a human-readable video spec.

## Core Goal

Write a text spec for a full video and render it directly. The spec should cover scenes, timing, text, math, planar figures, 3D figures, layers, styling, backgrounds, movement, jitter, and effects.

The target use case is long-form mathematical video, including topics like algebraic topology, where the author needs fast notation, diagrams, surfaces, spaces, maps, arrows, deformations, and explanatory motion.

## Spec Language

The spec should probably live in a markdown-adjacent file. The exact language is undecided.

Options to consider:

- A purpose-built DSL tailored exactly to animation/math needs.
- Lua embedded as a scripting layer.
- Markdown with fenced blocks for scene declarations.
- YAML/TOML for declarative structure plus embedded expressions.

The important property is that the author can write mathematical objects naturally. A bespoke DSL may be justified if it keeps common operations terse and readable.

Example direction:

```text
scene "quotient space intuition" duration 45s

background radial near-white -> light-grey

layer board 2d:
  text "$X / \\sim$" at (0.2, 0.15) size 72
  circle center (0, 0) radius 2 stroke blue
  point p at (1, 0)
  open_point q at (-1, 0)
  arrow p -> q dotted

layer model 3d:
  camera orbit radius 6 at (0, 0, 0)
  tetrahedron vertices [...]
  surface param (u, v) -> (...)
```

## Scenes

Needed:

- Multiple scenes per video.
- Scene duration and local timeline.
- Scene contents declared in the spec.
- Per-scene background.
- Per-scene camera setup for 3D layers.
- Scene transitions eventually, but not required first.

## Layers

Needed:

- Ordered layers, rendered and stacked.
- 2D layers and 3D layers.
- Layer transforms, especially translating whole layers.
- Layer opacity.
- Layer effects, especially Gaussian blur.
- Ability to apply effects to a full rendered layer, not just individual objects.

## Backgrounds

Default background:

- Gentle radial gradient.
- Near-white in the center.
- Slightly darker very light grey toward the edges.

Also useful:

- Solid colors.
- Custom radial gradients.
- Paper/whiteboard texture later if it helps, but keep it subtle.

## 2D Figures

Convenient constructors:

- Point.
- Open point: small unfilled circle.
- Line segment.
- Ray.
- Arrow.
- Dotted line.
- Circle.
- Ellipse.
- Triangle.
- Polygon.
- Blob/freeform region.
- Parametric curve.
- Shaded planar region with color and opacity.

All should render whiteboard-style with constant visible stroke thickness.

## 3D Figures

Convenient constructors:

- Point.
- Line segment.
- Triangle.
- Tetrahedron.
- Polygon mesh or wireframe.
- Parametric curve.
- Parametric surface.
- Blob/implicit-looking surface later.
- Shaded volume or surface with color and opacity.

The intended 3D path:

1. Represent 3D curves/surfaces using NURBS or NURBS-like primitives.
2. Apply object transforms in 3D.
3. Project through a camera using standard projection formulae.
4. Convert/project 3D NURBS to 2D NURBS where possible.
5. Render the resulting planar curves with the same whiteboard stroke pipeline.

The visual style should remain constant-thickness linework after projection, matching the rest of the engine.

## Camera

Needed:

- Camera position.
- Look-at target.
- Perspective projection.
- Orthographic projection eventually.
- Camera animation.
- Orbit-style camera helpers.

Implemented first:

- Per-3D-layer perspective distance, projection scale, and screen center.

## Text And LaTeX

Needed:

- Strong LaTeX-ish math support.
- Ordinary text.
- Math text.
- Scripts, fractions, radicals, Greek, blackboard bold, common operators.
- Extensible symbol capture pipeline.
- Better typographic layout over time.

The engine should not use any fallback symbol set that changes the visual language. Captured/processed symbols are the source of truth.

## Motion And Timing

Needed:

- Object movement.
- Layer movement.
- Camera movement.
- Draw-on animations.
- Fade in/out.
- Timing functions/easing.
- Per-object and per-layer timing controls.
- Long videos with many timed events.

Motion should be smooth independently of any low-rate hand jitter.

## Jitter

Needed controls in the spec:

- Enable/disable jitter per object.
- Enable/disable jitter per layer.
- Jitter strength.
- Jitter update rate.
- Time-varying jitter strength.
- Possibly separate whole-object jitter from stroke deformation jitter.

Current direction:

- Avoid uncorrelated per-control-point noise.
- Prefer coherent position-based noise fields, individualized by object/figure seed.
- Preserve endpoints only when semantically needed; otherwise the whole object should feel redrawn.

## Effects

Needed:

- Gaussian blur on layers.
- Glow effect for a light source.
- Opacity.
- Potentially color grading or vignette later.

The glow use case does not need real shadow casting. A luminous blurred shape or layer-level bloom is enough.

## Styling

Needed:

- Stroke color.
- Stroke thickness.
- Fill color.
- Fill opacity.
- Dotted/dashed strokes.
- Antialiasing across stroke, fill, text, and layer-composited edges.
- Constant-thickness rendering.
- Whiteboard style defaults.

## Shading

Needed:

- Shade planar regions with color and opacity.
- Shade 3D surfaces/volumes in a whiteboard-compatible way.
- Keep shaded objects readable under linework.

## Renderer Architecture

Likely direction:

- Parse spec into a scene graph.
- Scene graph contains layers.
- Layers contain objects.
- Objects can be 2D or 3D.
- 3D layers have camera/projection state.
- Render each layer to an intermediate buffer.
- Apply layer effects.
- Composite layers in order.

This should make blur, glow, opacity, and layer translation natural.

High urgency:

- Replace the transparent-black layer mask with a true alpha channel. Black currently leaks into final renders and is not acceptable as a transparency key.
- Represent simple primitives as jitterable NURBS-style drawn figures, not perfect analytic shapes. Line segments should have hand-drawn variation and jitter; open points should be drawn/jitterable circle curves rather than static filled-disc cutouts.

## Near-Term Implementation Steps

1. Done: Move generated captured-symbol C out of `src/`.
2. Done: Rename the internal C prefix convention from `mb_` to `wb_`.
3. Done: Define a first minimal spec format.
4. Done: Implement parsing for scenes, layers, math text, and simple 2D figures. Current parser supports multiple scenes, real ordered 2D/3D layer declarations, math, move, line, point, and open_point.
5. Done: Add radial-gradient background support.
6. Done: Add layer buffers and compositing. Layers render into offscreen buffers and composite in declaration order with layer opacity.
7. Done: Add layer translation. The spec supports `move_layer name from (x,y) to (x,y) during Ts..Ts`.
8. Done: Add Gaussian blur for a layer. Layer declarations support `blur N`; the first implementation is CPU-side separable Gaussian blur and should be optimized later.
9. Done: Replace transparent-black layer compositing with true alpha-channel layer buffers. Layer RGB is now separate from layer visibility, so black ink can render without leaking through transparency.
10. Done: Convert primitive line segments and open points to jitterable NURBS-style drawn figures instead of perfect straight/disc geometry. Scene lines now render as jittered temporary NURBS curves, and open points render as jittered NURBS circle strokes; filled points still use the analytic disc path.
11. Done: Add explicit jitter controls to objects/layers. Layer and object declarations support `jitter off`, `jitter on`, or `jitter N`; time-varying jitter controls remain future work.
12. Done: Implement antialiasing consistently for rendered strokes, fills, math symbols, and alpha-composited layer edges. The rasterizer now writes coverage into the layer alpha buffer directly, preserving antialiased edges through compositing; this is coverage AA, not supersampling.
13. Done: Verify and fix scaling, height, baseline, and placement for awkward captured/math symbols such as `\mu`, descenders, tall operators, and other non-boxy glyphs. Runtime metric adjustments now handle `\mu`, descenders, digits, and tall operators; `wb_debug_print_symbol_metrics` is available for future audits.
14. Done: Add first 3D primitives and camera projection. The spec supports `line3d name from (x,y,z) to (x,y,z) ...`, rendered through a fixed simple perspective camera into the existing hand-drawn 2D stroke path; richer camera controls remain future work.
15. Done: Add projected 3D curve rendering through the existing planar drawing path. The spec supports `curve3d name through (x,y,z) (x,y,z) (x,y,z) ...`, projected through the fixed camera into a temporary 2D NURBS curve and rendered with the planar stroke renderer.
16. Done: Add basic per-3D-layer camera controls. The spec supports `camera distance D scale S center (x,y)` on the current layer; orientation, look-at, orbit helpers, orthographic projection, and camera animation remain future work.
17. Done: Add a stroked 2D circle constructor and demos. The spec supports `circle name center (x,y) radius R thickness T colour C`, rendered as a jitterable NURBS-style stroke; filled disks and open points remain separate constructors.
18. Done: Honor `video ... output "name.mp4"` when writing renders. Single-scene specs write exactly that path; multi-scene specs suffix the basename as `name_00.mp4`, `name_01.mp4`, etc. Output paths are intentionally restricted to simple shell-safe characters while rendering still uses `ffmpeg` via `popen`.
19. Done: Expose first draw-on timing in the spec. `draw name during Ts..Ts` now animates line-like strokes, stroked circles/open points, and projected 3D curves by revealing the sampled stroke over time; math and filled points still appear discretely.
20. Done: Add a dotted line constructor for mathematical diagrams. The spec supports `dotted_line name from (x,y) to (x,y) thickness T gap G colour C`, rendered as jitterable drawn dots and compatible with `draw name during Ts..Ts`.
21. Done: Add a hand-drawn arrow constructor. The spec supports `arrow name from (x,y) to (x,y) thickness T head H colour C`, rendered as a jitterable line shaft plus two drawn head strokes and compatible with draw-on timing.
22. Done: Add a first shaded planar region primitive. The spec supports `shade_disc name center (x,y) radius R colour C opacity A`, composited through the true layer alpha path so translucent fills sit correctly under linework.
23. Done: Add a triangle outline constructor. The spec supports `triangle name points (x,y) (x,y) (x,y) thickness T colour C`, rendered as three jitterable hand-drawn edges and compatible with draw-on timing.
24. Done: Add a shaded triangle region constructor. The spec supports `shade_triangle name points (x,y) (x,y) (x,y) colour C opacity A`, composited through the true layer alpha path and compatible with draw-on opacity timing.

## Open Questions

- Use a bespoke DSL, Lua, or markdown plus fenced blocks?
- How expressive should mathematical expressions in constructors be?
- Should 3D surfaces be true NURBS from the start, or should the first version support sampled parametric curves/surfaces and graduate later?
- How much of LaTeX parsing should be implemented locally versus delegated to a preprocessing step?
- Should long videos be one spec file or a directory of scene files?
