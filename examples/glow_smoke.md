video:
  output "glow_smoke.mp4"

scene "glow smoke":
  duration 0.2s
  background radial:
    center #ffffff
    edge #eef1f6

  layer halo:
    glow 16
    glow_opacity 0.35

    text note "glow":
      (0.50w,0.52h)
      size 54
      colour purple
