# Scale Patch Smoke

```whiteboard
video:
  output "scale_patch_smoke.mp4"

scene "scale patch smoke":
  2.1s

  patch layout:
    at (0.50w,0.56h)
    scale 150

    patch seed:
      at (0.0,0.0)

      circ ring:
        center (0,0)
        radius 0.42
        thickness 3.0
        colour purple

      text note "scales as a patch":
        at (-0.42,-0.58)
        size 30
        colour blue

      pt core:
        at (0.0,0.0)
        radius 7
        colour red

  scale_patch seed:
    0.5 -> 1.55
    0.2s..1.5s
```
