video:
  output "curve_smoke.mp4"

scene "curve smoke":
  duration 0.2s
  background radial:
    center #ffffff
    edge #eef1f6

  curve arc:
    (0.22w,0.66h) (0.50w,0.42h) (0.78w,0.66h)
    colour purple
    thickness 4
