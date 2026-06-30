# Whiteboard

This vibe-coded project was created in order to render it a practical prospect to continue my work at [Boarbarktree](https://www.youtube.com/@boarbarktree).

I pushed three videos, over three months, out of Adobe Animate, by sheer force of will. It damn near killed me. We're talking 15 hours a day for three months straight. Then I moved to Sydney and started a PhD. I tried to keep going, but my brain said no. But I never stopped wanting to! Finally had the bright idea to automate it. Naturally, 3blue1brown's [manim](https://github.com/3b1b/manim) wouldn't suit my stylistic preferences, and I couldn't be bothered to learn it, either. So I started writing my own. In my mother tongue, C, of course. Well, I didn't really have enough time/energy for that to get very far, either.

Then came the era of agentic coding. I thought, hmm, might as well give it a try. Oh - why don't I task it to my animation software?

And thus, this slopject was born.

Staying on theme, the remainder of this readme, from this point on, is AI-generated. 

## What this is

Whiteboard is intended to be a text-driven animation tool for long-form mathematical explanation videos.

The desired workflow is:

```text
write video spec
render video
adjust spec
render again
publish
```

The spec should eventually describe scenes, timing, text, math notation, planar diagrams, 3D diagrams, layers, backgrounds, movement, jitter, blur, glow, fades, and other whiteboard-ish effects.

The target use case is maths-heavy video, including things like algebraic topology, where the author needs notation, diagrams, spaces, maps, arrows, quotient-y things, deformations, surfaces, and explanatory motion quickly.

## What this is not

This is not a coding project in the noble artisanal sense.

This is not here to demonstrate software engineering excellence.

This is not an attempt to learn compiler construction, graphics programming, rendering architecture, C, parsing, animation systems, or anything else for its own sake.

This is AI slop with a mission.

The point is to make the tool exist with minimal fuss, because the real project is making videos. The code is allowed to be ugly, provisional, embarrassing, strange, and obviously vibe-coded, as long as it keeps moving toward the tool being useful.

## Visual target

The engine should preserve the handmade whiteboard feel of the original videos as much as possible.

Important ingredients:

- near-white radial-gradient backgrounds
- hand-drawn-looking linework
- constant visible stroke thickness
- notation that looks like the author's actual handwriting
- subtle jitter/redraw behaviour
- smooth object and camera motion
- diagram-first mathematical clarity
- no sudden visual-language changes from fallback fonts or mismatched symbols

The intended result is not “clean corporate explainer animation”. It should look like mathematical scratchwork that has been gently possessed by a rendering engine.

## Handwritten symbol pipeline

The project uses a small HTML-based capture workflow to record tablet strokes and turn them into reusable symbols.

This is important: captured/processed handwritten symbols are the source of truth for the visual language. The engine should avoid falling back to generic fonts or symbol sets in ways that break the handmade look.

Over time, the math/text system should support LaTeX-ish notation while still rendering symbols in the captured handwritten style.

## Spec language

The exact spec language is still evolving.

Possible directions include:

- a bespoke DSL tailored to animation and mathematics
- Markdown with fenced scene blocks
- YAML/TOML plus embedded expressions
- Lua as a scripting layer

The important thing is that the author can write mathematical animation naturally and tersely.

A rough target might look like this:

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

Current parser direction includes scenes, ordered 2D/3D layer declarations, math, moves, lines, points, and open points.

## Intended features

Whiteboard should eventually support:

- multiple scenes per video
- scene durations and local timelines
- per-scene backgrounds
- ordered 2D and 3D layers
- layer opacity and movement
- layer-level effects such as Gaussian blur
- ordinary text and LaTeX-ish math text
- handwritten captured symbols
- draw-on animations
- fades
- easing/timing controls
- object movement
- layer movement
- camera movement
- coherent hand-drawn jitter
- planar figures
- projected 3D figures
- shaded regions and surfaces
- long-form video specs

## 2D drawing goals

Convenient 2D primitives should include:

- points
- open points
- line segments
- rays
- arrows
- dotted and dashed lines
- circles
- ellipses
- triangles
- polygons
- blobs/freeform regions
- parametric curves
- shaded planar regions

Simple analytic primitives should eventually be represented as jitterable drawn figures rather than sterile perfect geometry.

## 3D drawing goals

The 3D path is roughly:

1. represent curves and surfaces using NURBS or NURBS-like primitives
2. apply 3D object transforms
3. project through a camera
4. convert projected curves into planar drawn curves where possible
5. render them through the same whiteboard stroke pipeline as 2D figures

Useful 3D primitives should eventually include:

- points
- line segments
- triangles
- tetrahedra
- polygon meshes
- wireframes
- parametric curves
- parametric surfaces
- blob/implicit-looking surfaces
- shaded surfaces or volumes

The key visual constraint is that projected 3D linework should still look like whiteboard linework, not like a different renderer suddenly wandered in.

## Renderer architecture

The likely architecture is:

```text
spec
  -> parser
  -> scene graph
  -> layers
  -> objects
  -> layer buffers
  -> effects
  -> compositing
  -> final frames
```

Layers are rendered to intermediate buffers, effects are applied at the layer level, and layers are composited in order.

This makes blur, glow, opacity, translation, and future effects much easier to reason about.

## Current/near-term status

Some early infrastructure exists, including:

- first minimal spec format
- radial-gradient background support
- offscreen layer buffers and compositing
- layer translation
- CPU-side separable Gaussian blur
- parser work for scenes, layers, math, movement, lines, points, and open points

Near-term priorities:

- replace transparent-black compositing with true alpha-channel layer buffers
- convert primitive line segments and open points to jitterable NURBS-style drawn figures
- add explicit jitter controls for objects and layers
- implement antialiasing consistently
- fix scaling, baselines, descenders, tall operators, and awkward captured symbols
- add first 3D primitives and camera projection
- render projected 3D curves through the existing planar drawing path

## Development philosophy

This project should stay brutally pragmatic.

A feature is good if it helps make the videos easier to produce.

A clever architecture is good only if it reduces future pain.

A hacked-together implementation is acceptable if it gets the tool closer to being usable.

A perfect system that delays making videos is a failure.

Whiteboard is allowed to be AI-generated rubble as long as the rubble stacks into a bridge.
