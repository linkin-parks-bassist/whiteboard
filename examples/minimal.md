# Minimal Whiteboard Spec

```whiteboard
video fps 60 jitter_fps 15 output "scene_00.mp4"

scene "minimal parsed scene" duration 4s
  background radial center #ffffff edge #f1f2f4
  layer board 2d opacity 0.95 blur 1
    math eq "$\frac{1}{2}\int \mu(A)^{-1}\chi_A+\mu(B)^{-1}\chi_B d\mu$" at (220,540) size 70 colour blue
    seg axis (220,680) -> (620,680) t 3 c grey
    pt p (220,680) r 7 c blue
    opt q (620,680) r 8 t 3 c blue
    circ loop (420,680) r 54 t 3 c green
    ell halo (420,680) rx 90 ry 42 t 3 c red
    tri tri (760,620) (860,720) (700,740) t 3 c grey
    poly pent (980,600) (1060,560) (1140,620) (1110,710) (1010,720) t 3 c green
    move eq from (220,540) to (420,420) during 1s..3s
    move_layer board from (0,0) to (0,-24) during 2s..4s
```
