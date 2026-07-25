video:
  output "mixed_patch_stack3d_smoke.mp4"

scene "mixed patch stack 3d smoke":
  duration 1.2s

  layer space 3d:
    camera:
      distance 7.3
      yaw 0.48
      center (0.50w,0.57h)

    patch parent:
      at (0.0,0.0,0.0)

      wire3d floor:
        points (-1.0,-0.5,-1.0) (1.0,-0.5,-1.0) (1.0,-0.5,1.0) (-1.0,-0.5,1.0)
        thickness 1.5
        colour grey

      patch child:
        at (0.45,0.0,0.0)

        line3d mast:
          from (-0.10,-0.05,0.0) to (0.55,0.0,0.0)
          thickness 2.4
          colour blue

        triangle3d fin:
          points (0.10,-0.08,0.0) (0.22,0.18,0.0) (0.44,0.03,0.0)
          thickness 1.9
          colour green

        point3d tip:
          at (0.68,0.04,0.0)
          radius 7
          colour red

    move_patch parent:
      (0.0,0.0,0.0) -> (0.40,0.22,0.38)
      0.0s..1.2s

    turn_patch parent:
      0.0 -> 0.38
      0.0s..1.2s

    move_patch child:
      (0.0,0.0,0.0) -> (-0.14,0.28,0.08)
      0.0s..1.2s

    turn_patch child:
      0.0 -> -0.82
      0.0s..1.2s

    scale_patch child:
      (1.0,1.0,1.0) -> (1.34,0.78,1.18)
      0.0s..1.2s
