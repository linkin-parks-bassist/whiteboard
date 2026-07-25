video:
  output "manifold_simple_smoke.mp4"

scene "centred manifold":
  1s

  defaults:
    colour blue
    thickness 0.008

  circ origin:
    (0,0)
    radius 0.35

  seg x_axis:
    (-1.5,0) -> (1.5,0)
    colour grey

  seg y_axis:
    (0,-0.9) -> (0,0.9)
    colour grey

  math title "$f(x)$":
    (0,0.55)
    size 0.15
