# Minimal Whiteboard Spec

```whiteboard
video fps 60 jitter_fps 15 output "scene_00.mp4"

scene "minimal parsed scene" duration 4s
  background radial center #ffffff edge #f1f2f4
  patch board o 0.95 blur 1
    math eq "$\frac{1}{2}\int \mu(A)^{-1}\chi_A+\mu(B)^{-1}\chi_B d\mu$" @ [0.115,0.500] s 70 c blue
    seg axis [0.115,0.630] -> [0.323,0.630] t 3 c grey
    pt p [0.115,0.630] r 7 c blue
    opt q [0.323,0.630] r 8 t 3 c blue
    circ loop [0.219,0.630] r 54 t 3 c green
    ell halo [0.219,0.630] rx 90 ry 42 t 3 c red
    tri tri [0.396,0.574] [0.448,0.667] [0.365,0.685] t 3 c grey
    shade_poly pent_fill [0.510,0.556] [0.552,0.519] [0.594,0.574] [0.578,0.657] [0.526,0.667] c green a 0.12
    poly pent [0.510,0.556] [0.552,0.519] [0.594,0.574] [0.578,0.657] [0.526,0.667] t 3 c green
    dash guide [0.115,0.685] -> [0.323,0.685] t 3 g 18 c grey
    fade pent_fill 0 -> 1 0.5s..2.0s
    move eq [0.115,0.500] -> [0.219,0.389] 1s..3s
    move_patch board [0.000,0.000] -> [0.000,-0.022] 2s..4s
```
