video:
  output "structured_patch3d_animation_smoke.mp4"

scene "structured patch 3d animation smoke":
  duration 1.2s

  patch space 3d:
    camera:
      distance 7.2
      yaw 0.46
      center (0.50w,0.57h)

    patch rig:
      at (0.0,0.0,0.0)

      wire3d floor:
        points (-0.9,-0.5,-0.9) (0.9,-0.5,-0.9) (0.9,-0.5,0.9) (-0.9,-0.5,0.9)
        thickness 1.5
        colour grey

      line3d boom:
        from (-0.7,0.0,0.0) to (0.8,0.0,0.0)
        thickness 2.3
        colour blue

      point3d tip:
        at (0.95,0.0,0.0)
        radius 7
        colour red

    move_patch rig:
      (0.0,0.0,0.0) -> (0.7,0.4,0.6)
      0.0s..1.2s

    turn_patch rig:
      0.0 -> 0.9
      0.0s..1.2s

    scale_patch rig:
      (1.0,1.0,1.0) -> (1.4,0.8,1.2)
      0.0s..1.2s
