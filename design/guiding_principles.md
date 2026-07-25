# Guiding Principles

## Complexity Must Be Reached For

Whiteboard may have powerful internal machinery: nested coordinate patches,
local transforms, 2D and 3D projection, composited effects, inherited style,
animation, and render isolation. That machinery is not, by itself, the
authoring experience.

An author building a simple 2D scene should be able to write a simple 2D scene
without encountering or having to understand patch trees, coordinate-frame
composition, cameras, 3D, render targets, effects, or ordering controls. Those
capabilities should appear only when the author explicitly needs them.

The implicit root manifold, ordinary declaration order, sensible defaults, and
flat scene syntax are the normal path. Patches, `space`/3D, transforms, `z`,
effect isolation, and other advanced features are opt-in tools. They must not
force boilerplate into source that does not use them.

When designing syntax or implementation, prefer the model where:

- a simple scene reads as a short list of objects and optional defaults;
- advanced structure appears only where it buys the author something concrete;
- implementation complexity stays hidden behind stable, direct authoring
  concepts; and
- advanced features never make the simple case harder to read, write, or
  render.

## The Root Is A Patch, Not A Constant

Every scene has an implicit top-level coordinate patch. It supplies a useful
default centred mathematical viewport, but it is a configurable scene object
rather than a hard-coded global coordinate formula. Future language primitives
may configure its centre and visible world extent without changing the meaning
of child-patch coordinates or requiring a different renderer. Simple scenes
must continue to use the default without mentioning the root at all.
