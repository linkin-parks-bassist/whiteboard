video:
  output "move_patch3d_smoke.mp4"

scene "move patch 3d smoke":
  duration 1.2s

  layer space 3d:
    camera:
      distance 7.0
      yaw 0.48
      center (0.50w,0.57h)

    patch rig:
      at (0.0,0.0,0.0)
      yaw 0.35

      wire3d guide:
        points (-0.8,-0.5,-0.8) (0.8,-0.5,-0.8) (0.8,-0.5,0.8) (-0.8,-0.5,0.8)
        thickness 1.6
        colour grey

      line3d mast:
        from (0.0,-0.4,0.0) to (0.0,0.7,0.0)
        thickness 2.6
        colour blue

      point3d beacon:
        at (0.0,0.85,0.0)
        radius 7
        colour red

    move_patch rig (0.0,0.0,0.0) -> (0.9,0.5,0.7) 0.0s..1.2s
