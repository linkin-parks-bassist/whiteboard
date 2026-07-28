video:
  output "patch_showcase.mp4"

scene "patch showcase 2d":
  duration 6s
  background radial:
    center #ffffff
    edge #eef1f6

  defaults:
    thickness 3.0

  patch board:
    at (0,0)
    scale (0.60,0.60)

    seg x_axis:
      from (-1.4,0.0) to (1.4,0.0)
      thickness 2.0
      colour grey

    seg y_axis:
      from (0.0,-1.0) to (0.0,1.0)
      thickness 2.0
      colour grey

    patch graph:
      at (0.20,0.0)

      curve arc:
        through (-0.95,0.42) (-0.22,-0.18) (0.88,0.55)
        thickness 3.8
        colour blue

      poly region:
        points (-0.62,-0.16) (-0.18,-0.42) (0.44,-0.26) (0.74,0.18) (0.10,0.42)
        colour purple
        thickness 2.8

      shade_poly wash:
        points (-0.62,-0.16) (-0.18,-0.42) (0.44,-0.26) (0.74,0.18) (0.10,0.42)
        colour green
        opacity 0.08

      math eq "$y=f(x)$":
        at (0.10,0.72)
        size 0.12
        colour black
        thickness 1.5

    patch callout:
      at (-0.55,0.42)

      blob outline:
        points (-0.42,-0.16) (-0.12,-0.34) (0.22,-0.20) (0.32,0.08) (0.08,0.30) (-0.28,0.22)
        thickness 3.0
        colour red

      text note "local patch":
        at (-0.20,0.02)
        size 0.10
        colour purple

  move_patch board:
    (0.0,0.0) -> (0.12,0.05)
    0.0s..6.0s

  turn_patch graph:
    0.0 -> 0.38
    0.0s..6.0s

  scale_patch graph:
    (1.0,1.0) -> (1.22,0.82)
    0.0s..6.0s

  move_patch callout:
    (0.0,0.0) -> (-0.08,0.12)
    0.0s..6.0s

  turn_patch callout:
    0.0 -> -0.62
    0.0s..6.0s

transition:
  type crossfade
  duration 0.35s

scene "patch showcase 3d":
  duration 6.5s
  background radial:
    center #ffffff
    edge #eef1f6

  patch space 3d:
    camera:
      distance 7.4
      yaw 0.50
      center (0.50w,0.58h)

    patch world:
      at (0.0,0.0,0.0)

      axes3d frame:
        at (-1.15,-0.85,-0.95)
        length 0.65
        thickness 1.8

      patch solids:
        at (0.0,0.0,0.0)

        tetra3d tet:
          points (0.0,0.95,0.0) (-0.75,-0.35,-0.55) (0.78,-0.35,-0.48) (0.05,-0.10,0.82)
          thickness 2.2
          colour blue
          opacity 0.08

        cube3d box:
          center (1.30,0.10,0.18)
          size 0.82
          thickness 1.9
          colour grey
          opacity 0.05

        patch ribbon:
          at (0.20,0.10,-0.05)

          line3d arm:
            from (-0.10,-0.08,0.0) to (0.62,0.00,0.0)
            thickness 2.3
            colour purple

          triangle3d fin:
            points (0.12,-0.06,0.0) (0.26,0.20,0.0) (0.46,0.04,0.0)
            thickness 1.9
            colour green

          point3d tip:
            at (0.76,0.06,0.0)
            radius 7
            colour red

    move_patch solids:
      (0.0,0.0,0.0) -> (-0.12,0.20,0.12)
    0.0s..6.5s

    turn_patch solids:
      0.0 -> 0.58
    0.0s..6.5s

    scale_patch solids:
      (1.0,1.0,1.0) -> (1.14,0.88,1.08)
    0.0s..6.5s

    move_patch ribbon:
      (0.0,0.0,0.0) -> (0.00,0.28,0.10)
    0.0s..6.5s

    turn_patch ribbon:
      0.0 -> -0.92
    0.0s..6.5s

    scale_patch ribbon:
      (1.0,1.0,1.0) -> (1.28,0.80,1.16)
    0.0s..6.5s
