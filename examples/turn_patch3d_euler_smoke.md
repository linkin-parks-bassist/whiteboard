video:
  output "turn_patch3d_euler_smoke.mp4"

scene "turn patch 3d euler smoke" 1.5s:
  layer space 3d:
    camera:
      distance 7.5
      yaw 0.50
      center (0.50w,0.58h)

    patch rig:
      at (0.0,0.0,0.0)

      axes3d frame:
        at (-1.15,-0.95,-1.10)
        length 0.72
        thickness 1.8

      wire3d sail:
        points (-0.65,-0.15,0.0) (0.70,-0.15,0.0) (0.40,0.42,0.0)
        thickness 2.1
        colour purple

      line3d mast:
        from (0.0,-0.34,0.0) to (0.0,0.52,0.0)
        thickness 2.0
        colour grey

      point3d knot:
        at (0.0,0.08,0.0)
        radius 8
        colour red

    turn_patch rig:
      (0.0,0.0,0.0) -> (0.92,0.48,-0.36)
      0.0s..1.5s

    move_patch rig:
      (0.0,0.0,0.0) -> (0.30,0.24,0.16)
      0.0s..1.5s
