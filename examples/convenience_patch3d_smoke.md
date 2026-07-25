video:
  output "convenience_patch3d_smoke.mp4"

scene "convenience patch 3d smoke":
  duration 1.2s

  layer space 3d:
    camera:
      distance 7.4
      yaw 0.52
      center (0.50w,0.57h)

    patch world:
      at (0.0,0.0,0.0)

      axes3d frame:
        at (-1.15,-0.85,-0.95)
        length 0.65
        thickness 1.8

      patch solids:
        at (0.15,0.0,0.0)

        tetra3d tet:
          points (0.0,0.95,0.0) (-0.75,-0.35,-0.55) (0.78,-0.35,-0.48) (0.05,-0.10,0.82)
          thickness 2.2
          colour blue
          opacity 0.08

        cube3d box:
          center (1.35,0.08,0.18)
          size 0.82
          thickness 1.9
          colour grey
          opacity 0.05

    move_patch solids:
      (0.0,0.0,0.0) -> (-0.18,0.22,0.14)
      0.0s..1.2s

    turn_patch solids:
      0.0 -> 0.75
      0.0s..1.2s

    scale_patch solids:
      (1.0,1.0,1.0) -> (1.18,0.84,1.10)
      0.0s..1.2s
