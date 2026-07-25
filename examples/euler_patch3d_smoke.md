video:
  output "euler_patch3d_smoke.mp4"

scene "euler patch 3d smoke" 1.4s:
  layer space 3d:
    camera:
      distance 7.6
      yaw 0.54
      center (0.50w,0.58h)

    patch rig:
      at (0.0,0.0,0.0)
      yaw 0.40
      pitch 0.55
      roll -0.32

      axes3d frame:
        at (-1.15,-0.95,-1.10)
        length 0.72
        thickness 1.8

      wire3d panel:
        points (-0.75,-0.18,0.0) (0.78,-0.18,0.0) (0.78,0.22,0.0) (-0.75,0.22,0.0)
        thickness 2.1
        colour blue

      triangle3d fin:
        points (-0.18,-0.12,0.0) (0.12,0.44,0.0) (0.42,-0.08,0.0)
        thickness 1.9
        colour red

      point3d tip:
        at (0.88,0.16,0.0)
        radius 8
        colour green

    move_patch rig:
      (0.0,0.0,0.0) -> (0.28,0.18,0.22)
      0.0s..1.4s
