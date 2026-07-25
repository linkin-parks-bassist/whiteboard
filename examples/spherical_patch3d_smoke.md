video:
  output "spherical_patch3d_smoke.mp4"

scene "spherical 3d patch smoke" 1.6s:
  layer space 3d:
    camera:
      distance 7.8
      yaw 0.48
      center (0.50w,0.58h)

    patch rig:
      at (0.0,0.0,0.0)

      axes3d frame:
        at (-1.15,-1.00,-1.10)
        length 0.72
        thickness 1.8

      patch shell:
        coords spherical
        at (0.0,0.0,0.0)

        wire3d arc_a:
          points (1.05,-0.70,-0.18) (1.00,-0.18,0.08) (1.02,0.42,0.28) (0.98,0.98,0.12)
          thickness 2.0
          colour blue

        wire3d arc_b:
          points (1.04,0.38,-0.42) (1.02,1.08,-0.18) (1.00,1.74,0.06) (0.96,2.36,0.20)
          thickness 2.0
          colour red

        point3d north:
          at (1.08,1.10,0.82)
          radius 8
          colour green

        point3d south:
          at (1.08,1.10,-0.82)
          radius 8
          colour purple

    scale_patch shell:
      (1.0,1.0,1.0) -> (1.12,0.90,1.12)
      0.0s..1.6s

    turn_patch shell:
      0.0 -> -0.82
      0.0s..1.6s
