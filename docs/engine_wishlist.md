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
  text "$X / \sim$" at (0.2, 0.15) size 72
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

Defaults should be centralized rather than re-declared ad hoc in many call sites. In practice this probably means a small set of renderer/spec defaults collected in one place, likely as `#define`s or an equivalent central configuration layer for:

- Default background type and colors.
- Default stroke thickness.
- Default layer opacity.
- Default blur radius.
- Default jitter behavior and strength.
- Default render dimensions / frame rate where appropriate.

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
- Scene-to-scene transitions.

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

Default behavior is still an open design choice. A promising direction is:

- Jitter off by default for static objects.
- Temporarily raise jitter during object or camera motion so moving elements feel hand-redrawn rather than mechanically translated.
- Let jitter decay back down once motion settles.

This would preserve readability in still compositions while keeping motion from feeling sterile.

## Effects

Needed:

- Gaussian blur on layers.
- Glow effect for a light source.
- Opacity.
- Potentially color grading or vignette later.

The glow use case does not need real shadow casting. A luminous blurred shape or layer-level bloom is enough.

For scene transitions, the first useful tier is probably simple compositing-based transitions rather than ambitious spatial ones:

- Fade through.
- Crossfade.
- Brief dip-to-background / dip-to-white.
- Possibly wipe or slide later if they fit the visual language.

The main requirement is that multi-scene videos should not always hard-cut unless the author wants that.

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

## Coordinate System

Needed:

- A coordinate system that is easier to reason about than literal pixels.
- Specs that are less tightly coupled to one render resolution.
- A clear answer for how aspect-ratio changes should affect diagrams.

The main options are:

- Pixel coordinates everywhere. This is simple for the renderer but unpleasant to author and too tied to one output size.
- Normalize `x` by width and `y` by height. This makes full-frame layout easy and naturally allows stretching/squishing when aspect ratio changes, but a unit square is not automatically a square unless the author compensates.
- Normalize both axes by `min(width, height)`. This makes geometric reasoning nicer and preserves squares/circles more naturally, but objects near the frame edges become less intuitive to place and aspect-ratio changes do not fully behave like ordinary screen scaling.
- Support both screen-space layout coordinates and geometry-space coordinates. This is probably the most useful end state, but it is more design and parser work.

Current recommendation:

- Default to frame-relative coordinates for layout: think in proportions of the frame rather than pixels.
- Normalize `x` and `y` separately by width and height for ordinary placement, accepting that aspect-ratio changes may stretch the composition.
- Add a second geometry-friendly mode later for objects that should preserve shape more naturally.

This keeps title cards and scene layout easy to author while leaving room for a better mathematical geometry space later.

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

## Render UX

Needed:

- Clear progress reporting during renders.
- Per-scene progress and overall video progress.
- Useful timing information such as frame counts, elapsed time, and possibly ETA.
- Output that is readable during long renders rather than silent or spammy.

Pretty progress bars are not a core rendering feature, but they matter for usability. If a long-form render takes a while, the tool should make it obvious that it is healthy and how far along it is.

High urgency:

- Replace the transparent-black layer mask with a true alpha channel. Black currently leaks into final renders and is not acceptable as a transparency key.
- Represent simple primitives as jitterable NURBS-style drawn figures, not perfect analytic shapes. Line segments should have hand-drawn variation and jitter; open points should be drawn/jitterable circle curves rather than static filled-disc cutouts.
- Optimize the renderer aggressively. The current engine is far too slow and too casual about CPU work to be comfortable for long-form video iteration.

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
25. Done: Add a quadrilateral outline constructor. The spec supports `quad name points (x,y) (x,y) (x,y) (x,y) thickness T colour C`, rendered as four jitterable hand-drawn edges and compatible with draw-on timing.
26. Done: Add a ray constructor for mathematical diagrams. The spec supports `ray name from (x,y) through (x,y) thickness T colour C`, rendered through the existing hand-drawn stroke path and compatible with draw-on timing.
27. Done: Add an ellipse constructor for planar diagrams. The spec supports `ellipse name center (x,y) radii (rx,ry) thickness T colour C`, rendered as a jitterable NURBS-style stroke and compatible with draw-on timing.
28. Done: Add a first pass of prettier shorthand syntax for common 2D commands. The parser now accepts concise aliases such as `seg`, `pt`, `opt`, `circ`, `ell`, and `tri`; a deeper cleanup toward a genuinely mathematical DSL is still desirable later.
29. Done: Add a polygon outline constructor. The spec supports `polygon/poly name ...` with 3 to 7 planar vertices, rendered as jitterable hand-drawn edges and compatible with draw-on timing.
30. Done: Add a shaded polygon region constructor. The spec supports `shade_polygon/shade_poly name ...` with 3 to 7 planar vertices, composited through the true alpha path and compatible with draw-on opacity timing.
31. Done: Add a dashed line constructor. The spec supports `dashed_line/dash name ...`, rendered as jitterable hand-drawn dash segments and compatible with draw-on timing.
32. Done: Add first camera animation support for 3D layers. The spec supports `move_camera layer from distance D scale S center (x,y) to distance D scale S center (x,y) during Ts..Ts`, interpolated through the existing easing path.
33. Done: Add first layer fade support. The spec supports `fade_layer layer from A to A during Ts..Ts`, animated through the compositor by interpolating layer opacity over time.
34. Done: Add first object fade support for shaded primitives. The spec supports `fade name from A to A during Ts..Ts`; this pass applies to opacity-bearing shaded objects such as `shade_disc`, `shade_triangle`, and `shade_polygon`.
35. Done: Add a projected 3D point primitive. The spec supports `point3d name at (x,y,z) radius R colour C`, projected through the current camera into the existing planar point renderer.
36. Done: Add a projected 3D open-point primitive. The spec supports `open_point3d name at (x,y,z) radius R thickness T colour C`, projected through the current camera into the existing planar open-point renderer.
37. Done: Add a projected 3D triangle outline primitive. The spec supports `triangle3d name points (x,y,z) (x,y,z) (x,y,z) thickness T colour C`, projected through the current camera into the existing planar hand-drawn triangle path.
38. Done: Add a projected 3D shaded triangle primitive. The spec supports `shade_triangle3d name points (x,y,z) (x,y,z) (x,y,z) colour C opacity A`, projected through the current camera into the existing planar alpha-fill triangle path and compatible with draw-on opacity timing.
39. Done: Add a first real concise-authoring pass for spec syntax. Math, camera, movement, draw, and fade commands now have shorter forms such as `@`, `s`, `c`, arrow-style motion, and terse timing syntax; a deeper DSL pass is still desirable later, but specs no longer have to read quite so much like boilerplate.
40. Done: Add a first frame-relative coordinate mode for 2D layout. Specs can now use `[x,y]` outside quoted text/LaTeX to mean fractions of frame width/height, which makes broad placement much less resolution-bound even though a more geometry-aware coordinate system could still come later.
41. Done: Centralize engine/spec defaults. `wb_defaults.h` is now the obvious shared home for background, opacity, jitter, camera, sizing, and related default/range values, and parser/scene code has been pushed onto those shared constants instead of repeating the same magic numbers inline.
42. Done: Add a first orbit-style camera helper for 3D layers. The spec supports `camera ... yaw A ...` for static orientation and `orbit_camera layer from A to A during Ts..Ts` for simple animated yaw around the scene, which is enough for rotating-polyhedron demos without pretending the camera system is finished.
43. Done: Revisit default jitter behavior. Jitter is now off by default unless a spec explicitly enables it, but object/layer/camera motion automatically injects temporary jitter when no explicit override was set, then lets it settle back down afterward.
44. Done: Land a first renderer-throughput pass. The hottest hand-drawn stroke and projected-curve paths now build their small NURBS on the stack instead of heap-allocating and freeing them every frame; broader profiling and deeper rasterization/blur work still remain.
45. Done: Add first-pass render progress reporting. Long renders now emit a single-line updating stderr progress bar with overall percentage, current scene, per-scene frame count, total frame count, and elapsed time; richer terminal UI and ETA estimation can come later.
46. Done: Add first scene transition support. The spec now supports top-level `transition fade Ns` and `transition crossfade Ns` declarations between scenes; multi-scene renders are emitted as one continuous video timeline with overlapped scene compositing instead of forced hard cuts.

## Next Wave

The next batch of work should probably focus less on adding one more primitive and more on making the engine pleasant to author with and practical to iterate on.

Recommended order:

1. Coordinate system and defaults.
2. Jitter behavior.
3. Render speed and render UX.
4. Scene transitions.
5. DSL cleanup.

Rationale:

- Better coordinates and sensible defaults improve every spec immediately.
- Better jitter policy improves the visual feel of almost every animation.
- Better performance and progress reporting are necessary before long-form use becomes tolerable.
- Scene transitions matter once multi-scene videos become common.
- Prettier syntax is important, but it is easier to judge once the behavioral defaults are less clumsy.

### First-Pass Proposals

For the current pending items, the most plausible first implementations are:

- `39`: Add a second, cleaner spelling for common commands without removing the current parser-friendly forms. Treat this as syntax layering, not a parser rewrite.
- `40`: Introduce frame-relative 2D coordinates, probably in `[0,1]` screen space or something similarly explicit, while keeping raw pixels as a fallback during migration.
- `41`: Move the current scattered magic defaults into one header/config section and make the parser rely on those values instead of open-coded literals.
- `42`: Keep static jitter low or off by default, and attach extra jitter to motion interpolation paths rather than to every resting object.
- `43`: Start with profiling and allocation audits before doing speculative micro-optimizations. The first target should be frame-to-frame repeated work and expensive temporary geometry generation.
- `44`: Add a single-line updating progress display first; fancier bars or richer terminal UI can come later if they do not complicate logging.
- `45`: Start with `fade` and `crossfade` transitions between whole scenes; do not begin with wipes, slides, or object-aware transitions.

### First-Pass Acceptance Criteria

The next pending items should not be considered "done" merely because some code exists. A reasonable first-pass bar for each item is:

- `39`: At least one cleaner author-facing syntax path exists for common scene/layer/object declarations, is documented by example, and can coexist with the old forms without breaking them.
- `40`: A spec author can place ordinary 2D content using frame-relative coordinates without manually converting to pixels, and the coordinate mode is explicit rather than magical.
- `41`: There is one obvious source of truth for renderer/spec defaults, and changing a default there actually changes parser/runtime behavior consistently.
- `42`: Static scenes no longer look overly jittery by default, while moving objects visibly gain redraw energy during motion without requiring manual per-object tuning in every spec.
- `43`: There is evidence from measurement, not guesswork, that the slowest render paths were identified and at least one meaningful bottleneck was improved.
- `44`: Long renders expose ongoing progress in a way that is legible in a normal terminal and useful enough that the user can tell the process is healthy and advancing.
- `45`: A multi-scene spec can request at least one non-hard-cut transition and get a visibly correct result in the final render.

### Suggested Implementation Sketches

These are not commitments, but they are plausible first cuts:

- `39`: Add optional aliases such as scene-local shorthand blocks or terser constructor spellings before attempting any grand syntax redesign.
- `40`: Accept coordinates like `(0.5w, 0.3h)` or an explicit normalized mode, rather than replacing pixels wholesale on day one.
- `41`: Introduce a single defaults header and route parser fallbacks through it before worrying about user-overridable configuration files.
- `42`: Modulate jitter from the motion/easing path or from object velocity, rather than inventing a large new timing language immediately.
- `43`: Instrument frame time, allocation count, or per-stage timing first; optimize second.
- `44`: Keep progress reporting on stderr or otherwise separate from ordinary output paths so snapshot/video filenames remain scriptable.
- `45`: Implement scene overlap in the compositor first, because that reuses existing opacity/layer machinery and is easier to reason about than bespoke transition rendering.

### Design Gating

Not all pending items are equally ready to build. Some mainly need engineering time; others still need one or two product/design decisions before implementation will be clean.

Items that can largely start immediately:

- `41` centralize defaults
- `43` optimize the renderer
- `44` add render progress reporting

These are mostly engineering tasks. They may still involve design judgment, but they do not seem blocked on a major language or UX decision.

Items that need a clearer design decision first:

- `39` prettier syntax
- `40` better coordinate system
- `42` default jitter behavior
- `45` scene transitions

These affect the author-facing model of the tool, so a messy first implementation could create long-lived compatibility or UX baggage.

Minimal decisions that would unblock them:

- `39`: Decide whether the prettier syntax should be layered on top of the current DSL or whether the DSL itself is about to change shape more substantially.
- `40`: Decide whether frame-relative coordinates should become the default, an opt-in mode, or a separate syntax for specific fields.
- `42`: Decide whether motion-activated jitter is automatic engine behavior, a default policy with overrides, or an explicit spec feature.
- `45`: Decide where transitions live in the language: per-scene, between-scene declarations, or video-level defaults.

Practical implication:

- If implementation work should start immediately, `41`, `43`, and `44` are the safest next targets.
- If the next step is more design discussion, `40` and `42` are probably the highest-leverage choices because they affect how nearly every future spec will feel to author.

### Dependency Map

Some of the pending items are not just individually valuable; they also make later work cleaner.

- `41` defaults supports `40` coordinates, `42` jitter policy, and `45` transitions by providing one place for fallback behavior.
- `40` coordinates influences `39` prettier syntax, because the nicest surface syntax depends partly on how positions are spelled.
- `42` jitter policy interacts with `43` optimization, because automatic motion-linked jitter may change where the expensive paths are.
- `43` optimization and `44` progress reporting fit well together, because profiling/instrumentation work can power both performance improvements and better user feedback.
- `45` transitions are easier once `41` defaults exist, because transition durations/types/background handling can inherit sane defaults instead of requiring verbose per-scene declarations.

In other words:

- `41` is a cleanup task that also removes friction for several later features.
- `40` and `42` are design choices that should be made before investing too heavily in `39`.
- `43` and `44` can proceed in parallel or in one combined pass.
- `45` should not block everything else, but it will be cleaner after defaults are centralized.

### Immediate Next Actions

If the project wants a concrete "what should I do next?" answer, the most sensible choices are:

1. Start `41`: create the central defaults source of truth.
2. In parallel or immediately after, start `43`/`44`: add measurement hooks and basic render progress output.
3. After that, make a deliberate call on `40` and `42`.
4. Only then spend real time on `39` syntax cleanup.
5. Add `45` transitions once the surrounding defaults/compositor behavior are less ad hoc.

## Open Questions

- Use a bespoke DSL, Lua, or markdown plus fenced blocks?
- How expressive should mathematical expressions in constructors be?
- Should 2D coordinates be normalized by width/height, by `min(width, height)`, or should the language expose both layout-space and geometry-space coordinates?
- Should jitter be a mostly explicit stylistic choice, or should the engine automatically inject it during motion while keeping static frames cleaner?
- Which defaults belong in compile-time constants/`#define`s versus user-visible spec-level defaults?
- How should scene transitions be expressed in the spec: transition objects between scenes, per-scene outgoing/incoming transition declarations, or video-level defaults?
- Should 3D surfaces be true NURBS from the start, or should the first version support sampled parametric curves/surfaces and graduate later?
- How much of LaTeX parsing should be implemented locally versus delegated to a preprocessing step?
- Should long videos be one spec file or a directory of scene files?
