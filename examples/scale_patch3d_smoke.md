video:
  output "scale_patch3d_smoke.mp4"

scene "scale patch 3d smoke":
  duration 1.2s

  patch space 3d:
    camera:
      distance 7.1
      yaw 0.50
      center (0.50w,0.58h)

    patch shell:
      at (0.0,0.1,0.0)

      wire3d box:
        points (-0.6,-0.6,-0.6) (0.6,-0.6,-0.6) (0.6,0.6,-0.6) (-0.6,0.6,-0.6)
        thickness 1.7
        colour blue

      wire3d box_back:
        points (-0.6,-0.6,0.6) (0.6,-0.6,0.6) (0.6,0.6,0.6) (-0.6,0.6,0.6)
        thickness 1.7
        colour blue

      line3d join_a:
        from (-0.6,-0.6,-0.6) to (-0.6,-0.6,0.6)
        thickness 1.5
        colour grey

      line3d join_b:
        from (0.6,-0.6,-0.6) to (0.6,-0.6,0.6)
        thickness 1.5
        colour grey

      line3d join_c:
        from (0.6,0.6,-0.6) to (0.6,0.6,0.6)
        thickness 1.5
        colour grey

      line3d join_d:
        from (-0.6,0.6,-0.6) to (-0.6,0.6,0.6)
        thickness 1.5
        colour grey

    scale_patch shell (1.0,1.0,1.0) -> (1.6,0.7,1.3) 0.0s..1.2s
