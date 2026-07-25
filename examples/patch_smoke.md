# Coordinate Patch Smoke

```whiteboard
video:
  output "patch_smoke.mp4"

scene "patch smoke":
  2.0s

  defaults:
    thickness 3.0

  patch graph:
    at (0.50w,0.56h)
    scale (220,-170)

    seg x_axis:
      from (-1.45,0.0) to (1.45,0.0)
      thickness 2.0
      colour grey

    seg y_axis:
      from (0.0,-1.05) to (0.0,1.05)
      thickness 2.0
      colour grey

    curve parabola:
      through (-1.00,0.55) (-0.25,-0.10) (0.95,0.72)
      thickness 3.5
      colour blue

    pt origin:
      at (0.0,0.0)
      radius 7
      colour red

    text caption "local cartesian patch":
      at (0.65,-0.88)
      size 36
      colour purple
```
