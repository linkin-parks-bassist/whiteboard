video:
  output "draw_fade_patch_smoke.mp4"

scene "draw fade patch smoke" 1.6s:
  patch board:
    at (0.50w,0.56h)
    scale (210,-170)

    patch motif:
      at (0.0,0.0)

      curve arc:
        through (-0.92,0.30) (-0.18,-0.24) (0.88,0.36)
        thickness 3.6
        colour blue

      blob shell:
        points (-0.58,-0.08) (-0.20,-0.34) (0.34,-0.22) (0.58,0.14) (0.08,0.34)
        thickness 2.8
        colour purple

      shade_blob wash:
        points (-0.58,-0.08) (-0.20,-0.34) (0.34,-0.22) (0.58,0.14) (0.08,0.34)
        colour green
        opacity 0.10

      text note "patch-level draw/fade":
        at (-0.42,-0.64)
        size 32
        colour red

  draw motif:
    0.0s..0.9s

  fade motif:
    1.0 -> 0.18
    0.9s..1.6s
