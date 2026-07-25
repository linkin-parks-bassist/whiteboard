video:
  output "polygon_patch_affine_smoke.mp4"

scene "polygon patch affine smoke":
  duration 1.2s

  patch shape:
    at (0.50w,0.56h)
    scale (210,-165)

    poly frame:
      points (-0.60,-0.35) (0.40,-0.42) (0.78,0.10) (0.15,0.56) (-0.68,0.18)
      thickness 3.0
      colour blue

    shade_poly fill:
      points (-0.60,-0.35) (0.40,-0.42) (0.78,0.10) (0.15,0.56) (-0.68,0.18)
      colour green
      opacity 0.10

  turn_patch shape:
    0.0 -> 0.85
    0.0s..1.2s

  scale_patch shape:
    (1.0,1.0) -> (1.35,0.72)
    0.0s..1.2s
