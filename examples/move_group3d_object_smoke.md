video:
  output "move_group3d_object_smoke.mp4"

scene "move group 3d object smoke" 1.5s:
  layer space 3d:
    camera:
      distance 7.6
      yaw 0.46
      center (0.50w,0.58h)

    tetra3d tet:
      points (0.0,0.95,0.0) (-0.74,-0.35,-0.52) (0.78,-0.35,-0.46) (0.05,-0.08,0.80)
      thickness 2.0
      colour purple
      opacity 0.06

    move tet:
      (0.0,0.0,0.0) -> (0.36,0.22,0.18)
      0.0s..1.5s
