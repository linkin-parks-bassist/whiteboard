video:
  output "move_group3d_patch_smoke.mp4"

scene "move group 3d patch smoke" 1.4s:
  layer space 3d:
    camera:
      distance 7.4
      yaw 0.52
      center (0.50w,0.58h)

    patch rig:
      at (0.0,0.0,0.0)

      axes3d frame:
        at (-1.15,-0.95,-1.10)
        length 0.72
        thickness 1.8

      wire3d sail:
        points (-0.62,-0.14,0.0) (0.68,-0.14,0.0) (0.36,0.40,0.0)
        thickness 2.0
        colour blue

      point3d knot:
        at (0.0,0.08,0.0)
        radius 8
        colour red

    move rig:
      (0.0,0.0,0.0) -> (0.42,0.26,0.20)
      0.0s..1.4s
