video:
  output "polar_patch_motion_smoke.mp4"

scene "polar patch motion smoke":
  duration 1.2s

  patch petals:
    at (0.50w,0.54h)
    coords polar
    scale 150

    blob flower:
      points (1.00,0.00) (1.42,0.52) (1.00,1.04) (1.42,1.56) (1.00,2.08) (1.42,2.60) (1.00,3.12)
      thickness 3.0
      colour purple

    shade_blob wash:
      points (0.72,0.16) (1.10,0.70) (0.76,1.24) (1.10,1.78) (0.76,2.32)
      colour green
      opacity 0.08

    circ ring:
      center (1.16,0.0)
      radius 6
      colour blue

  move_patch petals:
    (0.0,0.0) -> (-0.14,0.10)
    0.0s..1.2s

  turn_patch petals:
    0.0 -> 0.52
    0.0s..1.2s

  scale_patch petals:
    (1.0,1.0) -> (1.18,0.86)
    0.0s..1.2s

  text note "animated polar patch":
    at (0.50w,0.82h)
    size 42
    colour grey
