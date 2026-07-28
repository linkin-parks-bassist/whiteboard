video:
  output "patch_group_target_showcase.mp4"

scene "patch group target showcase 2d" 1.7s:
  background radial:
    center #ffffff
    edge #eef1f6

  patch board:
    at (0.50w,0.56h)
    scale (210,-170)

    patch motif:
      at (-0.10,0.06)
      rotate 0.24

      curve arc:
        through (-0.92,0.30) (-0.18,-0.26) (0.88,0.38)
        thickness 3.5
        colour blue

      poly frame:
        points (-0.58,-0.10) (-0.18,-0.34) (0.38,-0.22) (0.62,0.16) (0.08,0.36)
        thickness 2.7
        colour purple

      shade_poly wash:
        points (-0.58,-0.10) (-0.18,-0.34) (0.38,-0.22) (0.62,0.16) (0.08,0.36)
        colour green
        opacity 0.10

      text note "draw/fade/move on patch name":
        at (-0.44,-0.66)
        size 30
        colour red

  draw motif:
    0.0s..0.8s

  move motif:
    (0.0,0.0) -> (0.30,0.14)
    0.4s..1.3s

  fade motif:
    1.0 -> 0.18
    1.0s..1.7s

transition:
  type crossfade
  duration 0.35s

scene "patch group target showcase 3d" 1.9s:
  background radial:
    center #ffffff
    edge #eef1f6

  patch space 3d:
    camera:
      distance 7.8
      yaw 0.52
      center (0.50w,0.58h)

    patch world:
      at (0.0,0.0,0.0)

      axes3d frame:
        at (-1.15,-0.95,-1.10)
        length 0.72
        thickness 1.8

      tetra3d tet:
        points (0.0,0.95,0.0) (-0.74,-0.35,-0.52) (0.78,-0.35,-0.46) (0.05,-0.08,0.80)
        thickness 2.0
        colour blue
        opacity 0.08

      patch pointer:
        at (0.18,-0.08,-0.06)
        rotate (0.12,0.36,-0.22)

        line3d spine:
          from (-0.12,-0.10,0.0) to (0.74,0.10,0.0)
          thickness 2.2
          colour red

        point3d tip:
          at (0.86,0.14,0.0)
          radius 8
          colour purple

    draw tet:
      0.0s..0.8s

    draw pointer:
      0.2s..1.0s

    move tet:
      (0.0,0.0,0.0) -> (0.22,0.18,0.14)
      0.5s..1.5s

    move pointer:
      (0.0,0.0,0.0) -> (0.34,0.22,0.18)
      0.5s..1.5s

    fade pointer:
      1.0 -> 0.10
      1.1s..1.9s
