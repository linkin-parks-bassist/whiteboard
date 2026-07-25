video:
  output "advanced_patch_showcase.mp4"

scene "advanced patch showcase 2d" 1.7s:
  background radial:
    center #ffffff
    edge #eef1f6

  defaults:
    thickness 3.0

  patch board:
    at (0.50w,0.56h)
    scale (220,-170)

    patch polar_field:
      coords polar

      curve orbit:
        through (0.68,-0.10) (0.94,0.78) (1.06,1.52)
        thickness 3.5
        colour blue

      blob island:
        points (0.32,0.60) (0.44,1.12) (0.82,1.44) (1.16,1.02) (0.92,0.52) (0.50,0.34)
        thickness 2.6
        colour purple

      shade_blob wash:
        points (0.32,0.60) (0.44,1.12) (0.82,1.44) (1.16,1.02) (0.92,0.52) (0.50,0.34)
        colour green
        opacity 0.08

    patch label:
      at (-0.62,0.42)

      text note "polar local space":
        at (-0.18,0.02)
        size 34
        colour red

      arrow cue:
        from (0.05,-0.04) to (0.42,-0.18)
        thickness 2.1
        colour grey

  turn_patch polar_field:
    0.0 -> 0.62
    0.0s..1.7s

  scale_patch polar_field:
    (1.0,1.0) -> (1.16,0.86)
    0.0s..1.7s

  move_patch label:
    (0.0,0.0) -> (-0.12,0.14)
    0.0s..1.7s

transition:
  type crossfade
  duration 0.35s

scene "advanced patch showcase 3d" 2.0s:
  background radial:
    center #ffffff
    edge #eef1f6

  layer space 3d:
    camera:
      distance 8.0
      yaw 0.54
      center (0.50w,0.58h)

    patch world:
      at (0.0,0.0,0.0)

      axes3d frame:
        at (-1.25,-1.00,-1.20)
        length 0.72
        thickness 1.8

      patch orbit:
        coords cylindrical
        yaw 0.32
        pitch 0.24

        wire3d rail:
          points (0.96,0.00,-0.16) (1.04,1.00,-0.08) (1.06,2.02,0.04) (0.96,3.04,0.12)
          thickness 2.0
          colour blue

        point3d beacon:
          at (1.10,1.10,0.20)
          radius 8
          colour red

      patch shell:
        coords spherical
        at (0.18,0.06,0.12)
        rotate (0.12,0.50,-0.28)

        wire3d seam_a:
          points (1.02,-0.52,-0.18) (1.00,0.18,0.04) (1.02,0.86,0.28) (0.98,1.42,0.12)
          thickness 1.9
          colour purple

        wire3d seam_b:
          points (1.00,0.26,-0.46) (1.02,1.10,-0.18) (0.98,1.84,0.08) (0.94,2.40,0.24)
          thickness 1.9
          colour green

      patch rig:
        at (0.20,-0.12,-0.08)

        tetra3d tet:
          points (0.0,0.95,0.0) (-0.72,-0.35,-0.52) (0.78,-0.35,-0.46) (0.05,-0.08,0.80)
          thickness 2.0
          colour grey
          opacity 0.06

        line3d spine:
          from (-0.10,-0.10,0.0) to (0.74,0.12,0.0)
          thickness 2.2
          colour red

        point3d tip:
          at (0.88,0.16,0.0)
          radius 7
          colour blue

    turn_patch orbit:
      (0.0,0.0,0.0) -> (0.74,0.28,-0.12)
      0.0s..2.0s

    move_patch orbit:
      (0.0,0.0,0.0) -> (0.00,0.12,0.18)
      0.0s..2.0s

    turn_patch shell:
      (0.0,0.0,0.0) -> (-0.64,0.42,0.18)
      0.0s..2.0s

    scale_patch shell:
      (1.0,1.0,1.0) -> (1.10,0.92,1.12)
      0.0s..2.0s

    move_patch rig:
      (0.0,0.0,0.0) -> (0.28,0.22,0.18)
      0.0s..2.0s

    turn_patch rig:
      (0.0,0.0,0.0) -> (0.88,0.46,-0.30)
      0.0s..2.0s
