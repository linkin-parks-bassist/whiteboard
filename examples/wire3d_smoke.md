video:
  output "wire3d_smoke.mp4"

scene "wire3d smoke":
  duration 0.2s
  background radial:
    center #ffffff
    edge #eef1f6

  camera:
    distance 6.5
    yaw 0.35
    center (0.50w,0.56h)

  wire3d loop:
    (0,0.9,0) (-0.8,0.1,-0.4) (-0.3,-0.7,0.6) (0.8,-0.5,-0.3) (0.7,0.4,0.5)
    colour purple
    thickness 3
