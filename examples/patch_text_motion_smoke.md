# Patch Text Motion Smoke

```whiteboard
video:
  output "patch_text_motion_smoke.mp4"

scene "patch text motion":
  2.3s

  patch layout:
    at (0.50w,0.52h)
    scale (150,-150)

    patch label_cluster:
      at (-0.65,0.15)
      rotate -0.20

      text top "local label":
        at (0.00,0.00)
        size 34
        colour blue

      text sub "moves with its patch":
        at (0.10,-0.24)
        size 28
        colour grey

      arrow cue:
        from (-0.10,0.12) to (0.34,0.42)
        thickness 2.5
        head 0.12
        colour purple

  move_patch label_cluster:
    (-0.65,0.15) -> (0.35,-0.20)
    0.4s..1.7s
```
