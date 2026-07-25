video:
  output "cylindrical_patch3d_smoke.mp4"

scene "cylindrical 3d patch smoke" 1.5s:
  layer space 3d:
    camera:
      distance 7.4
      yaw 0.56
      center (0.50w,0.58h)

    patch rig:
      at (0.0,0.0,0.0)

      axes3d frame:
        at (-1.25,-0.95,-1.15)
        length 0.72
        thickness 1.8

      patch orbit:
        coords cylindrical
        at (0.0,0.0,0.0)

        line3d spoke_a:
          from (0.00,0.00,0.00) to (1.15,0.00,0.15)
          thickness 2.2
          colour blue

        line3d spoke_b:
          from (0.00,1.57,0.00) to (0.92,1.57,0.36)
          thickness 2.2
          colour red

        curve3d ring:
          through (0.86,0.00,-0.12) (0.96,0.92,-0.06) (1.00,1.84,0.04) (0.90,2.76,0.10) (0.78,3.60,0.02)
          thickness 2.0
          colour purple

        point3d marker:
          at (1.02,0.78,0.18)
          radius 8
          colour green

    turn_patch orbit:
      0.0 -> 0.95
      0.0s..1.5s

    move_patch orbit:
      (0.0,0.0,0.0) -> (0.0,0.0,0.22)
      0.0s..1.5s
