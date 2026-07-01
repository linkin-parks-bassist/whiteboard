# Smoke Whiteboard Spec

```whiteboard
video fps 60 jitter_fps 15 output "smoke.mp4"

scene "smoke" duration 0.25s
  background radial center #ffffff edge #f1f2f4
  layer board 2d opacity 0.95 blur 1 jitter 1.25
    math eq "$\mu+x$" at (220,420) size 70 colour black jitter off
    circle loop center (340,500) radius 88 thickness 3 colour green jitter 1.2
    line axis from (220,500) to (420,500) thickness 3 colour blue jitter 1.5
    open_point q at (460,500) radius 10 thickness 3 colour blue jitter 1.5
  layer model 3d opacity 1 jitter 1.1
    camera distance 5 scale 300 center (960,540)
    line3d edge from (-1,-0.5,0) to (1,0.8,1.5) thickness 3 colour green jitter 1.2
    curve3d arc through (-1,0.7,0) (0,1.3,1.4) (1,0.4,0.4) thickness 3 colour red jitter 1.1

scene "smoke second scene" duration 0.20s
  background radial center #ffffff edge #f1f2f4
  layer board 2d opacity 1 jitter off
    math label "$x^2+y^2$" at (300,450) size 64 colour blue
```
