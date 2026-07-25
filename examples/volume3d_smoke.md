scene "volume3d smoke":
  0.10s

  camera:
    distance 6.5
    yaw 0.35
    look_at (0,0,0)
    center (0.50w,0.50h)

  volume3d cloud:
    (0,0,0)
    radii (0.85,0.60,0.72)
    shells 4
    u_steps 10
    v_steps 6
    wobble 0.10
    colour blue
    opacity 0.16
    thickness 1.8
