# Nested Coordinate Patch Smoke

```whiteboard
video:
  output "nested_patch_smoke.mp4"

scene "nested patch smoke":
  2.0s

  defaults:
    thickness 3.0

  patch frame:
    at (0.48w,0.54h)
    scale 170
    rotate 0.20

    quad frame_box:
      points (-1.1,-0.8) (1.1,-0.8) (1.1,0.8) (-1.1,0.8)
      thickness 2.2
      colour grey

    patch motif:
      at (0.35,0.10)
      scale (0.85,0.85)
      rotate -0.70

      tri motif_outline:
        points (-0.65,-0.35) (0.72,-0.20) (-0.10,0.74)
        thickness 3.0
        colour blue

      shade_triangle motif_fill:
        points (-0.65,-0.35) (0.72,-0.20) (-0.10,0.74)
        colour green
        opacity 0.10

      patch orbit:
        at (-0.55,0.42)
        scale 0.75

        circ orbit_ring:
          center (0.0,0.0)
          radius 0.42
          thickness 3.0
          colour purple

        pt orbit_dot:
          at (0.42,0.0)
          colour red
          radius 7
```
