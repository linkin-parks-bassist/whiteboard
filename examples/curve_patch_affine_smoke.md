video:
  output "curve_patch_affine_smoke.mp4"

scene "curve patch affine smoke":
  duration 1.2s

  patch graph:
    at (0.50w,0.56h)
    scale (240,-180)

    curve arc:
      through (-0.85,0.30) (-0.10,-0.28) (0.90,0.46)
      thickness 4.2
      colour purple

  turn_patch graph:
    0.0 -> -0.62
    0.0s..1.2s

  scale_patch graph:
    (1.0,1.0) -> (0.82,1.30)
    0.0s..1.2s
