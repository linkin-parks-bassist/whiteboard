video:
  output "structured_mixed_patch_stack3d_smoke.mp4"

scene "structured mixed patch stack 3d smoke":
  duration 1.2s

  layer space 3d:
    camera:
      distance 7.1
      yaw 0.44
      center (0.50w,0.57h)

    patch parent:
      at (0.0,0.0,0.0)

      wire3d floor:
        points (-0.9,-0.5,-0.9) (0.9,-0.5,-0.9) (0.9,-0.5,0.9) (-0.9,-0.5,0.9)
        thickness 1.5
        colour grey

      patch child:
        at (0.48,0.0,0.0)

        line3d arm:
          from (0.0,0.0,0.0) to (0.62,0.0,0.0)
          thickness 2.3
          colour blue

        point3d tip:
          at (0.78,0.0,0.0)
          radius 7
          colour red

    move_patch parent:
      (0.0,0.0,0.0) -> (0.35,0.16,0.32)
      0.0s..1.2s

    turn_patch parent:
      0.0 -> 0.40
      0.0s..1.2s

    move_patch child:
      (0.0,0.0,0.0) -> (-0.12,0.26,0.10)
      0.0s..1.2s

    turn_patch child:
      0.0 -> -0.88
      0.0s..1.2s

    scale_patch child:
      (1.0,1.0,1.0) -> (1.28,0.82,1.16)
      0.0s..1.2s
