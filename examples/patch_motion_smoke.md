# Coordinate Patch Motion Smoke

```whiteboard
video:
  output "patch_motion_smoke.mp4"

scene "patch motion smoke":
  2.2s

  patch board:
    at (0.52w,0.55h)
    scale (180,-180)

    seg x_axis:
      from (-1.4,0.0) to (1.4,0.0)
      colour grey
      thickness 2.0

    seg y_axis:
      from (0.0,-1.0) to (0.0,1.0)
      colour grey
      thickness 2.0

    patch motif:
      at (-0.55,0.10)
      scale 0.80
      rotate 0.35

      circ orbit:
        center (0.0,0.0)
        radius 0.34
        colour purple
        thickness 3.0

      pt bead:
        at (0.34,0.0)
        colour red
        radius 7

      text tag "moving in patch space":
        at (0.0,-0.58)
        size 30
        colour blue

      move tag:
        (0.0,-0.58) -> (0.25,-0.68)
        0.8s..1.8s

  move_patch motif:
    (-0.55,0.10) -> (0.38,0.28)
    0.3s..1.5s
```
