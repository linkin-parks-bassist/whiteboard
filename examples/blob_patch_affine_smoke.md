video:
  output "blob_patch_affine_smoke.mp4"

scene "blob patch affine smoke":
  duration 1.2s

  patch blobber:
    at (0.50w,0.57h)
    scale (220,-170)

    blob outline:
      points (-0.75,-0.20) (-0.25,-0.48) (0.38,-0.32) (0.72,0.06) (0.40,0.46) (-0.22,0.54) (-0.70,0.18)
      thickness 3.0
      colour purple

    shade_blob fill:
      points (-0.75,-0.20) (-0.25,-0.48) (0.38,-0.32) (0.72,0.06) (0.40,0.46) (-0.22,0.54) (-0.70,0.18)
      colour blue
      opacity 0.10

  turn_patch blobber:
    0.0 -> -0.70
    0.0s..1.2s

  scale_patch blobber:
    (1.0,1.0) -> (0.78,1.34)
    0.0s..1.2s
