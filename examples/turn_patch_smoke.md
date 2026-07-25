# Turn Patch Smoke

```whiteboard
video:
  output "turn_patch_smoke.mp4"

scene "turn patch smoke":
  2.2s

  patch board:
    at (0.50w,0.54h)
    scale 165

    patch rotor:
      at (0.00,0.00)

      tri blade:
        points (0.10,0.00) (0.72,0.12) (0.18,0.22)
        thickness 3.0
        colour blue

      shade_triangle fill:
        points (0.10,0.00) (0.72,0.12) (0.18,0.22)
        colour green
        opacity 0.10

      pt hub:
        at (0.0,0.0)
        radius 7
        colour red

  turn_patch rotor:
    0.0 -> 1.8
    0.3s..1.8s
```
