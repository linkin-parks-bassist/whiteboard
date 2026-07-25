video:
  output "surface3d_smoke.mp4"

scene "surface3d smoke":
  duration 0.2s
  background radial:
    center #ffffff
    edge #eef1f6

  camera:
    distance 6.5
    yaw 0.35
    center (0.50w,0.56h)

  surface3d sheet:
    ( -0.9,  0.7, -0.2) ( 0.9,  0.6,  0.2) ( 0.7, -0.7,  0.7) (-0.8, -0.6, -0.1)
    u_steps 4
    v_steps 3
    colour blue
    opacity 0.10
    thickness 2.2
