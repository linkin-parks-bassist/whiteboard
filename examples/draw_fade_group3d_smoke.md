video:
  output "draw_fade_group3d_smoke.mp4"

scene "draw fade group 3d smoke" 1.7s:
  layer space 3d:
    camera:
      distance 7.6
      yaw 0.50
      center (0.50w,0.58h)

    patch rig:
      at (0.0,0.0,0.0)

      tetra3d tet:
        points (0.0,0.95,0.0) (-0.74,-0.35,-0.52) (0.78,-0.35,-0.46) (0.05,-0.08,0.80)
        thickness 2.0
        colour blue
        opacity 0.08

      patch pointer:
        at (0.18,-0.06,-0.04)

        line3d spine:
          from (-0.12,-0.10,0.0) to (0.74,0.10,0.0)
          thickness 2.2
          colour red

        point3d tip:
          at (0.86,0.14,0.0)
          radius 8
          colour purple

    draw tet:
      0.0s..0.9s

    draw pointer:
      0.2s..1.0s

    fade pointer:
      1.0 -> 0.10
      1.0s..1.7s
