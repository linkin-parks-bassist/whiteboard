video:
  output "blob_smoke.mp4"

scene "blob smoke":
  duration 0.2s
  background radial:
    center #ffffff
    edge #eef1f6

  shade_blob patch:
    (0.18w,0.74h) (0.28w,0.58h) (0.46w,0.60h) (0.54w,0.78h) (0.36w,0.88h)
    colour green
    opacity 0.12

  blob outline:
    (0.18w,0.74h) (0.28w,0.58h) (0.46w,0.60h) (0.54w,0.78h) (0.36w,0.88h)
    colour green
    thickness 3
