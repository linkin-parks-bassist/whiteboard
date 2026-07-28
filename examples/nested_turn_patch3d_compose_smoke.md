video:
  output "nested_turn_patch3d_compose_smoke.mp4"

scene "nested turn patch 3d compose":
  duration 1.2s

  patch space 3d:
    camera:
      distance 7.0
      yaw 0.46
      center (0.50w,0.57h)

    patch parent:
      at (0.0,0.0,0.0)

      wire3d floor:
        points (-0.9,-0.5,-0.9) (0.9,-0.5,-0.9) (0.9,-0.5,0.9) (-0.9,-0.5,0.9)
        thickness 1.5
        colour grey

      patch child:
        at (0.45,0.0,0.0)

        line3d arm:
          from (0.0,0.0,0.0) to (0.65,0.0,0.0)
          thickness 2.4
          colour blue

        point3d tip:
          at (0.82,0.0,0.0)
          radius 7
          colour red

    turn_patch parent:
      0.0 -> 0.8
      0.0s..1.2s

    turn_patch child:
      0.0 -> -1.0
      0.0s..1.2s
