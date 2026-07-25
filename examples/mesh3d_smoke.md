scene "mesh3d smoke":
  0.10s

  camera:
    distance 6.0
    yaw 0.35
    look_at (0,0,0)
    center (0.50w,0.50h)

  mesh3d octa:
    vertices (-0.8,0,0) (0.8,0,0) (0,-0.8,0) (0,0.8,0) (0,0,0.9) (0,0,-0.9)
    faces [0,2,4] [2,1,4] [1,3,4] [3,0,4] [2,0,5] [1,2,5] [3,1,5] [0,3,5]
    colour blue
    opacity 0.08
    thickness 2.5
