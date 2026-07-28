video:
  output "world_turn_scale3d_smoke.mp4"

scene "world turn scale 3d smoke" 1.6s:
  patch space 3d:
    camera:
      distance 7.8
      yaw 0.50
      center (0.50w,0.58h)

    patch rig:
      at (0.16,-0.08,-0.04)

      tetra3d tet:
        points (0.0,0.95,0.0) (-0.74,-0.35,-0.52) (0.78,-0.35,-0.46) (0.05,-0.08,0.80)
        thickness 2.0
        colour blue
        opacity 0.08

      patch pointer:
        at (0.20,-0.08,-0.06)

        line3d spine:
          from (-0.12,-0.10,0.0) to (0.74,0.10,0.0)
          thickness 2.2
          colour red

        point3d tip:
          at (0.86,0.14,0.0)
          radius 8
          colour purple

    turn_patch rig (0.0,0.0,0.0) -> (0.76,0.28,-0.18) 0.0s..1.6s

    scale_patch rig (1.0,1.0,1.0) -> (1.14,0.90,1.10) 0.0s..1.6s
