# Nested Move Patch Smoke

```whiteboard
video:
  output "nested_move_patch_smoke.mp4"

scene "nested move patch":
  2.4s

  patch frame:
    at (0.50w,0.54h)
    scale 155

    patch left:
      at (-0.75,0.0)
      scale 0.9

      circ ring:
        center (0,0)
        radius 0.42
        thickness 3.0
        colour blue

      pt bead:
        at (0.42,0.0)
        radius 7
        colour red

    patch right:
      at (0.78,0.0)
      scale 0.9
      rotate 0.35

      tri sail:
        points (-0.38,-0.25) (0.46,0.0) (-0.18,0.54)
        thickness 3.0
        colour purple

      shade_triangle wash:
        points (-0.38,-0.25) (0.46,0.0) (-0.18,0.54)
        colour green
        opacity 0.10

  move_patch left:
    (-0.75,0.0) -> (-0.25,0.34)
    0.2s..1.3s

  move_patch right:
    (0.78,0.0) -> (0.26,-0.30)
    0.8s..2.0s
```
