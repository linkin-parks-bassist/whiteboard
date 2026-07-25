video:
  output "manifold_patch_smoke.mp4"

scene "root patch":
  1s

  patch graph:
    at (0,0)
    scale 0.7

    seg x_axis:
      (-1,0) -> (1,0)
      thickness 0.008
      colour grey

    circ unit:
      (0,0)
      radius 0.5
      thickness 0.01
      colour blue
