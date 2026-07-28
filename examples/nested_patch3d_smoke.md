video:
  output "nested_patch3d_smoke.mp4"

scene "nested 3d patch smoke":
  duration 0.2s

  patch space 3d:
    camera:
      distance 7.2
      yaw 0.55
      center (0.50w,0.58h)

    patch world:
      at (0.1,0.0,0.0)
      scale (1.1,1.1,1.1)

      wire3d base:
        points (-1.0,-0.6,-1.0) (1.0,-0.6,-1.0) (1.0,-0.6,1.0) (-1.0,-0.6,1.0)
        thickness 1.8
        colour grey

      patch cluster:
        at (0.3,0.0,0.2)
        scale (0.8,1.1,0.8)
        yaw 0.75

        cube3d box:
          at (0.0,0.0,0.0)
          size 1.1
          thickness 1.7
          colour blue
          opacity 0.09

        patch cap:
          at (0.0,0.9,0.0)
          scale 0.7
          yaw -0.45

          surface3d lid:
            points (-0.7, 0.0, -0.5) (0.7, 0.1, -0.3) (0.6, 0.0, 0.6) (-0.6, -0.1, 0.5)
            u_steps 4
            v_steps 3
            colour green
            opacity 0.10
            thickness 1.4
