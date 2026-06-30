# Minimal Whiteboard Spec

```whiteboard
video fps 60 jitter_fps 15 output "scene_00.mp4"

scene "minimal parsed scene" duration 4s
  background radial center #ffffff edge #f1f2f4
  layer board 2d opacity 0.95 blur 1
    math eq "$\frac{1}{2}\int \mu(A)^{-1}\chi_A+\mu(B)^{-1}\chi_B d\mu$" at (220,540) size 70 colour blue
    line axis from (220,680) to (620,680) thickness 3 colour grey
    point p at (220,680) radius 7 colour blue
    open_point q at (620,680) radius 8 thickness 3 colour blue
    move eq from (220,540) to (420,420) during 1s..3s
    move_layer board from (0,0) to (0,-24) during 2s..4s
```
