# Structured Whiteboard Spec

```whiteboard
video:
  output "structured_demo.mp4"

scene "structured demo":
  2s

  defaults:
    colour purple

  shade_disc:
    (0.50w,0.72h)
    radius 0.06m
    colour green
    opacity 0.10

  math title "$A \to B$":
    at (0.50w,0.42h)
    size 110

  patch halo glow 14 glow_opacity 0.30:

    text note "plain subtitle":
      (0.50w,0.60h)
      size 42
      colour grey

  group motif:
    defaults:
      colour grey
      thickness 2.5

    seg:
      (0.36w,0.72h) -> (0.64w,0.72h)

    pt left:
      (0.36w,0.72h)
      colour blue

    opt right:
      (0.64w,0.72h)
      colour blue

    circ:
      (0.50w,0.72h)
      radius 0.048m
      colour green

    ell:
      (0.50w,0.72h)
      rx 0.078m
      ry 0.031m

    group highlight:
      defaults:
        colour red
        thickness 4.0

      ray:
        (0.50w,0.72h) -> (0.58w,0.64h)

    seg:
      (0.34w,0.67h) -> (0.66w,0.67h)

  tri:
    {0.78,0.32} {1.02,0.56} {0.58,0.56}
    colour blue

  shade_triangle:
    {0.78,0.32} {1.02,0.56} {0.58,0.56}
    colour blue
    opacity 0.08

  quad:
    (0.18w,0.30h) (0.28w,0.32h) (0.26w,0.46h) (0.16w,0.44h)
    colour blue

  shade_polygon:
    (0.70w,0.64h) (0.78w,0.60h) (0.84w,0.68h) (0.80w,0.78h) (0.71w,0.76h)
    colour red
    opacity 0.10

  shade_blob:
    (0.12w,0.76h) (0.18w,0.66h) (0.28w,0.68h) (0.30w,0.80h) (0.20w,0.86h)
    colour green
    opacity 0.09

  blob:
    (0.12w,0.76h) (0.18w,0.66h) (0.28w,0.68h) (0.30w,0.80h) (0.20w,0.86h)
    colour green
    thickness 3

  poly:
    (0.70w,0.64h) (0.78w,0.60h) (0.84w,0.68h) (0.80w,0.78h) (0.71w,0.76h)
    colour red

  ray:
    (0.60w,0.72h) -> (0.68w,0.64h)
    colour red

  curve:
    (0.18w,0.62h) (0.26w,0.54h) (0.34w,0.64h)
    colour purple
    thickness 3.5

  arrow:
    (0.66w,0.42h) -> (0.82w,0.42h)
    colour purple
    head 0.026m

  dotted_line:
    (0.36w,0.72h) -> (0.64w,0.72h)
    colour red
    gap 0.020m

  draw title:
    0.0s..0.6s

  draw note:
    0.4s..1.1s

  move note:
    (0.50w,0.60h) -> (0.50w,0.56h)
    0.9s..1.6s

  fade note:
    1.0 -> 0.25
    1.1s..1.8s

  fade motif:
    0.15 -> 1.0
    0.2s..0.9s

  draw highlight:
    0.3s..0.8s

transition:
  type crossfade
  duration 0.35s

scene "structured 3d":
  2s
  background:

  defaults:
    colour blue
    thickness 2.5

  camera:
    distance 7.0
    yaw 0.45
    projection perspective
    look_at (0,0,0)
    center (0.55w,0.58h)

  axes:
    (0,0,0)
    len 1.2

  point3d:
    (0,0.85,-0.4)
    colour red

  line3d:
    (-0.8,-0.6,-0.4) -> (0.8,-0.6,-0.4)
    colour grey

  curve3d:
    through (-0.6,-0.3,-0.4) (-0.1,0.45,0.2) (0.55,0.20,0.55)
    colour purple

  shade_triangle3d:
    (0,0,0.9) (-0.8,-0.6,-0.4) (0.8,-0.6,-0.4)
    opacity 0.10

  triangle3d:
    (0,0,0.9) (-0.8,-0.6,-0.4) (0.8,-0.6,-0.4)

  wire3d:
    (0,0.9,0) (-0.8,0.1,-0.4) (-0.3,-0.7,0.6) (0.8,-0.5,-0.3) (0.7,0.4,0.5)
    colour purple
    thickness 2.5

  shade_poly3d:
    (0,0.9,0) (-0.8,0.1,-0.4) (-0.3,-0.7,0.6) (0.8,-0.5,-0.3) (0.7,0.4,0.5)
    colour green
    opacity 0.10

  surface3d:
    (-0.9,0.7,-0.2) (0.9,0.6,0.2) (0.7,-0.7,0.7) (-0.8,-0.6,-0.1)
    u_steps 4
    v_steps 3
    colour blue
    opacity 0.08
    thickness 2.0

  mesh3d:
    vertices (-0.2,0.0,0.9) (-0.9,-0.2,0.1) (0.0,-0.9,0.1) (0.9,-0.2,0.1) (0.55,0.75,-0.15) (-0.55,0.75,-0.15)
    faces [0,1,2] [0,2,3] [0,3,4] [0,4,5] [0,5,1] [1,5,2] [2,5,4,3]
    colour purple
    opacity 0.06
    thickness 2.2

  blob3d:
    (0.0,0.15,0.0)
    radii (0.48,0.34,0.40)
    u_steps 10
    v_steps 6
    wobble 0.20
    colour green
    opacity 0.05
    thickness 2.0

  param3d:
    (0.0,0.0,0.0)
    family helix
    radius 0.62
    height 1.5
    turns 1.8
    steps 18
    colour red
    thickness 2.0

  param_surface3d:
    (0.0,0.0,0.0)
    family saddle
    rx 0.75
    ry 0.75
    amp 0.22
    u_steps 8
    v_steps 8
    colour blue
    opacity 0.05
    thickness 1.8

  volume3d:
    (0.0,0.0,0.0)
    radii (0.55,0.40,0.48)
    shells 3
    u_steps 8
    v_steps 5
    wobble 0.10
    colour purple
    opacity 0.12
    thickness 1.6

  tetra3d:
    (0,0,0.9) (-0.8,-0.6,-0.4) (0.8,-0.6,-0.4) (0,0.85,-0.4)
    opacity 0.08

  cube3d:
    (0.85,0.15,-0.10)
    size 0.45
    colour green
    opacity 0.06
```
