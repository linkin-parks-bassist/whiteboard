video:
  output "patch3d_smoke.mp4"

scene "3d patch smoke":
  duration 0.2s

  layer space 3d:
    camera:
      distance 6.8
      yaw 0.30
      center (0.50w,0.56h)

    patch rig:
      at (0.0,0.1,0.2)
      scale 1.2
      yaw 0.45

      axes3d frame:
        at (0,0,0)
        length 1.0
        thickness 1.6

      line3d spine:
        from (-0.8, -0.5, -0.2) to (0.7, 0.6, 0.4)
        thickness 2.4
        colour blue

      triangle3d face:
        points (-0.4, -0.2, 0.3) (0.5, -0.1, 0.0) (0.0, 0.6, -0.4)
        thickness 2.0
        colour purple

      point3d tip:
        at (0.65,0.55,0.45)
        radius 7
        colour red
