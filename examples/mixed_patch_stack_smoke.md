video:
  output "mixed_patch_stack_smoke.mp4"

scene "mixed patch stack smoke":
  duration 1.2s

  patch parent:
    at (0.50w,0.56h)
    scale (230,-175)

    seg axis_x:
      from (-1.3,0.0) to (1.3,0.0)
      thickness 2.0
      colour grey

    seg axis_y:
      from (0.0,-1.0) to (0.0,1.0)
      thickness 2.0
      colour grey

    patch child:
      at (0.55,0.0)

      tri sail:
        points (-0.18,-0.10) (0.26,0.00) (-0.04,0.24)
        thickness 3.0
        colour blue

      pt tip:
        at (0.32,0.16)
        radius 7
        colour red

  move_patch parent:
    (0.0,0.0) -> (0.28,0.14)
    0.0s..1.2s

  turn_patch parent:
    0.0 -> 0.42
    0.0s..1.2s

  move_patch child:
    (0.0,0.0) -> (-0.16,0.34)
    0.0s..1.2s

  turn_patch child:
    0.0 -> -0.95
    0.0s..1.2s

  scale_patch child:
    (1.0,1.0) -> (1.45,0.72)
    0.0s..1.2s
