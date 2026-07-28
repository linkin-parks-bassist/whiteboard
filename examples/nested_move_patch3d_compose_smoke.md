video:
  output "nested_move_patch3d_compose_smoke.mp4"

scene "nested move patch 3d compose":
  duration 1.2s

  patch space 3d:
    camera:
      distance 7.0
      yaw 0.44
      center (0.50w,0.57h)

    patch parent:
      at (0.0,0.0,0.0)

      wire3d floor:
        points (-0.9,-0.5,-0.9) (0.9,-0.5,-0.9) (0.9,-0.5,0.9) (-0.9,-0.5,0.9)
        thickness 1.5
        colour grey

      patch child:
        at (0.0,0.0,0.0)

        line3d arm:
          from (-0.6,0.0,0.0) to (0.7,0.0,0.0)
          thickness 2.4
          colour blue

        point3d tip:
          at (0.85,0.0,0.0)
          radius 7
          colour red

    move_patch parent:
      (0.0,0.0,0.0) -> (0.55,0.30,0.45)
      0.0s..1.2s

    move_patch child:
      (0.0,0.0,0.0) -> (-0.20,0.35,0.10)
      0.0s..1.2s
