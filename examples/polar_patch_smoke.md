# Polar Coordinate Patch Smoke

```whiteboard
video:
  output "polar_patch_smoke.mp4"

scene "polar patch smoke":
  2.0s

  defaults:
    thickness 3.0

  patch petals:
    at (0.50w,0.52h)
    coords polar
    scale 145

    blob flower:
      points (1.00,0.00) (1.45,0.55) (1.00,1.10) (1.45,1.65) (1.00,2.20) (1.45,2.75) (1.00,3.30)
      thickness 3.0
      colour purple

    shade_blob wash:
      points (0.72,0.20) (1.10,0.75) (0.78,1.30) (1.08,1.85) (0.76,2.40)
      colour green
      opacity 0.08

    circ ring:
      center (1.18,0.0)
      radius 6
      colour blue

  text note "polar local coordinates":
    at (0.50w,0.82h)
    size 42
    colour grey
```
