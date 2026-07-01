# Smoke Whiteboard Spec

```whiteboard
video fps 60 jitter_fps 15 output "smoke.mp4"

scene "smoke" duration 0.25s
  background radial center #ffffff edge #f1f2f4
  layer board 2d opacity 0.95 blur 1 jitter 1.25
    math eq "$\mu+x$" at (220,420) size 70 colour black jitter off
    shade_disc region center (340,500) radius 92 colour green opacity 0.18
    circle loop center (340,500) radius 88 thickness 3 colour green jitter 1.2
    shade_triangle tri_fill points (720,450) (840,530) (680,560) colour blue opacity 0.14
    triangle tri points (720,450) (840,530) (680,560) thickness 3 colour blue jitter 1.2
    quad panel points (1000,430) (1120,445) (1100,585) (980,570) thickness 3 colour grey jitter 1.1
    ray direction from (860,610) through (980,660) thickness 3 colour red jitter 1.2
    line axis from (220,500) to (420,500) thickness 3 colour blue jitter 1.5
    dotted_line guide from (220,450) to (460,450) thickness 3 gap 22 colour grey jitter 1.2
    arrow map from (500,500) to (650,450) thickness 3 head 24 colour red jitter 1.3
    open_point q at (460,500) radius 10 thickness 3 colour blue jitter 1.5
    draw loop during 0s..0.20s
    draw region during 0s..0.20s
    draw tri_fill during 0.05s..0.25s
    draw tri during 0.05s..0.25s
    draw panel during 0.05s..0.25s
    draw direction during 0.08s..0.25s
    draw axis during 0.05s..0.25s
    draw guide during 0.05s..0.25s
    draw map during 0.05s..0.25s
  layer model 3d opacity 1 jitter 1.1
    camera distance 5 scale 300 center (960,540)
    line3d edge from (-1,-0.5,0) to (1,0.8,1.5) thickness 3 colour green jitter 1.2
    curve3d arc through (-1,0.7,0) (0,1.3,1.4) (1,0.4,0.4) thickness 3 colour red jitter 1.1
    draw arc during 0s..0.25s

scene "smoke second scene" duration 0.20s
  background radial center #ffffff edge #f1f2f4
  layer board 2d opacity 1 jitter off
    math label "$x^2+y^2$" at (300,450) size 64 colour blue
```
