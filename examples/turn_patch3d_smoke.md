video:
  output "turn_patch3d_smoke.mp4"

scene "turn patch 3d smoke":
  duration 1.2s

  layer space 3d:
    camera:
      distance 7.0
      yaw 0.42
      center (0.50w,0.57h)

    patch rig:
      at (0.0,0.0,0.0)

      wire3d frame:
        points (-0.9,-0.5,-0.9) (0.9,-0.5,-0.9) (0.9,-0.5,0.9) (-0.9,-0.5,0.9)
        thickness 1.5
        colour grey

      line3d arm:
        from (-0.7,0.0,0.0) to (0.8,0.0,0.0)
        thickness 2.4
        colour blue

      point3d tip:
        at (0.95,0.0,0.0)
        radius 7
        colour red

    turn_patch rig 0.0 -> 1.1 0.0s..1.2s
