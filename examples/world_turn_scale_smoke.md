video:
  output "world_turn_scale_smoke.mp4"

scene "world turn scale smoke" 1.5s:
  patch board:
    at (0.50w,0.56h)
    scale (210,-170)

    patch motif:
      at (0.26,0.10)

      curve arc:
        through (-0.82,0.28) (-0.12,-0.22) (0.74,0.34)
        thickness 3.5
        colour blue

      blob shell:
        points (-0.54,-0.08) (-0.16,-0.32) (0.30,-0.20) (0.54,0.14) (0.10,0.34)
        thickness 2.7
        colour purple

      text note "world turn/scale":
        at (-0.34,-0.64)
        size 30
        colour red

  turn motif:
    around (0.50w,0.56h)
    0.0 -> 0.58
    0.0s..1.5s

  scale motif:
    around (0.50w,0.56h)
    1.0 -> 1.18
    0.0s..1.5s
