video:
  output "patch_nested_generated_target_smoke.mp4"

scene "patch nested generated target smoke" 1.8s:
  layer space 3d:
    camera:
      distance 7.8
      yaw 0.52
      center (0.50w,0.58h)

    patch rig:
      at (0.0,0.0,0.0)
      rotate (0.18,0.26,-0.12)

      axes3d frame:
        at (-1.18,-0.96,-1.12)
        length 0.72
        thickness 1.8

      tetra3d tet:
        points (0.0,0.95,0.0) (-0.74,-0.35,-0.52) (0.78,-0.35,-0.46) (0.05,-0.08,0.80)
        thickness 2.0
        colour blue
        opacity 0.08

      patch pointer:
        at (0.18,-0.08,-0.06)

        line3d spine:
          from (-0.12,-0.10,0.0) to (0.74,0.10,0.0)
          thickness 2.2
          colour red

        point3d tip:
          at (0.86,0.14,0.0)
          radius 8
          colour purple

    draw rig:
      0.0s..0.9s

    move rig:
      (0.0,0.0,0.0) -> (0.30,0.24,0.16)
      0.5s..1.4s

    fade rig:
      1.0 -> 0.10
      1.1s..1.8s
