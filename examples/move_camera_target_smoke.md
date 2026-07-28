scene "move-camera target smoke":
  0.20s

  space space:

    camera:
      distance 6.0
      yaw 0.35
      center (0.50w,0.50h)

    point3d:
      (-1.2,0,0)
      colour red

    point3d:
      (1.2,0,0)
      colour blue

    move_camera space d 6 s 260 y 0.35 @ (0.50w,0.50h) -> d 6 s 260 y 0.35 @ (0.50w,0.50h) 0.0s..0.20s target (-1.2,0,0) -> (1.2,0,0)
