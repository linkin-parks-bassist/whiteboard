# Topology Demo

```whiteboard
video fps 60 jitter_fps 15 output "topology_demo.mp4"

scene "quotient circle" duration 0.35s
  background radial center #ffffff edge #eef1f6
  layer board 2d opacity 1 jitter 1.1
    math title "$S^1/\\sim$" at (250,210) size 82 colour black jitter 0.7
    shade_disc disk center (440,520) radius 138 colour green opacity 0.16
    circle source center (440,520) radius 140 thickness 4 colour blue jitter 1.4
    shade_triangle simplex_fill points (980,430) (1110,590) (850,590) colour green opacity 0.14
    triangle simplex points (980,430) (1110,590) (850,590) thickness 4 colour green jitter 1.2
    quad cell points (1160,420) (1265,448) (1240,592) (1135,566) thickness 4 colour blue jitter 1.0
    ray tangent from (580,520) through (665,455) thickness 4 colour red jitter 1.0
    point p at (580,520) radius 8 colour red jitter 1.0
    open_point q at (300,520) radius 11 thickness 3 colour red jitter 1.3
    line chord from (330,430) to (550,610) thickness 3 colour grey jitter 1.4
    dotted_line quotient from (300,520) to (580,520) thickness 3 gap 24 colour red jitter 1.2
    arrow collapse from (660,520) to (820,520) thickness 4 head 30 colour blue jitter 1.2
    draw source during 0s..0.25s
    draw disk during 0s..0.25s
    draw simplex_fill during 0.08s..0.35s
    draw simplex during 0.08s..0.35s
    draw cell during 0.12s..0.35s
    draw tangent during 0.12s..0.35s
    draw chord during 0.1s..0.35s
    draw quotient during 0.05s..0.35s
    draw collapse during 0.12s..0.35s
  layer glow 2d opacity 0.35 blur 10 jitter 0.6
    circle halo center (440,520) radius 148 thickness 9 colour green jitter 1.2

scene "projected path" duration 0.35s
  background radial center #ffffff edge #eef1f6
  layer model 3d opacity 1 jitter 1.2
    camera distance 4.5 scale 430 center (960,560)
    line3d base from (-1.2,-0.7,0) to (1.2,-0.7,0) thickness 3 colour grey jitter 1.0
    curve3d arc through (-1.1,-0.2,0) (0,1.1,1.4) (1.1,-0.2,0.2) thickness 4 colour blue jitter 1.3
    draw arc during 0s..0.35s
  layer labels 2d opacity 1 jitter 0.8
    math label "$\\gamma:[0,1]\\to X$" at (300,230) size 64 colour black jitter 0.8
```
