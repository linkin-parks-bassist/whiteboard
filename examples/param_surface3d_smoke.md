scene "param surface smoke":
  0.10s

  camera:
    distance 7.0
    yaw 0.35
    look_at (0,0,0)
    center (0.50w,0.50h)

  param_surface3d torus_demo:
    (0,0,0)
    family torus
    major 1.1
    minor 0.32
    rz 1.0
    u_steps 14
    v_steps 9
    colour purple
    opacity 0.08
    thickness 2.0
