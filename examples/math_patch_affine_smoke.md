video:
  output "math_patch_affine_smoke.mp4"

scene "math patch affine smoke":
  duration 1.2s

  patch board:
    at (0.50w,0.56h)
    scale (1.0,1.0)

    math formula "$\\int_0^1 x^2\\,dx$":
      at (0.0,0.0)
      size 110
      colour blue
      thickness 1.6

  turn_patch board:
    0.0 -> 0.45
    0.0s..1.2s

  scale_patch board:
    (1.0,1.0) -> (1.28,0.74)
    0.0s..1.2s
