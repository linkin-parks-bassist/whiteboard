video:
  output "structured_convenience_patch3d_smoke.mp4"

scene "structured convenience patch 3d smoke":
  duration 1.2s

  patch space 3d:
    camera:
      distance 7.2
      yaw 0.48
      center (0.50w,0.57h)

    patch cluster:
      at (0.0,0.0,0.0)

      axes3d frame:
        at (-1.05,-0.80,-0.90)
        length 0.60
        thickness 1.7

      cube3d shell:
        center (1.15,0.05,0.20)
        size 0.76
        thickness 1.8
        colour grey
        opacity 0.05

      tetra3d gem:
        points (0.0,0.90,0.0) (-0.70,-0.30,-0.52) (0.70,-0.32,-0.44) (0.02,-0.08,0.78)
        thickness 2.1
        colour purple
        opacity 0.08

    move_patch cluster:
      (0.0,0.0,0.0) -> (0.22,0.18,0.24)
      0.0s..1.2s

    turn_patch cluster:
      0.0 -> -0.68
      0.0s..1.2s

    scale_patch cluster:
      (1.0,1.0,1.0) -> (0.86,1.16,1.22)
      0.0s..1.2s
