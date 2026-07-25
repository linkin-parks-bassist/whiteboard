# Affine Patch Smoke

```whiteboard
video:
  output "affine_patch_smoke.mp4"

scene "affine patch smoke":
  2.5s

  patch frame:
    at (0.48w,0.54h)
    scale 155

    patch motif:
      at (-0.15,0.10)
      rotate 0.20

      quad card:
        points (-0.50,-0.34) (0.46,-0.28) (0.40,0.34) (-0.56,0.28)
        thickness 2.8
        colour grey

      text top "affine":
        at (-0.10,-0.04)
        size 32
        colour blue

      arrow cue:
        from (-0.08,0.10) to (0.28,0.32)
        thickness 2.4
        head 0.12
        colour purple

  move_patch motif:
    (-0.15,0.10) -> (0.34,-0.08)
    0.2s..1.6s

  turn_patch motif:
    0.0 -> 1.2
    0.4s..2.0s

  scale_patch motif:
    0.9 -> 1.35
    0.6s..1.9s
```
