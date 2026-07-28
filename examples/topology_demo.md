# Topology Demo

```whiteboard
video fps 60 jitter_fps 15 output "topology_demo.mp4"

scene "quotient circle" duration 6s
  background radial center #ffffff edge #eef1f6
  patch board opacity 1 jitter 1.1
    math title "$S^1/\sim$" at (-0.74,0.61) size 0.15 colour black jitter 0.7
    shade_disc disk center (-0.52,0.04) radius 0.26 colour green opacity 0.16
    circle source center (-0.52,0.04) radius 0.26 thickness 0.007 colour blue jitter 1.4
    ellipse neighbourhood center (0.39,0.39) radii (0.22,0.14) thickness 0.007 colour grey jitter 0.9
    shade_polygon region_fill points (0.31,-0.30) (0.42,-0.20) (0.52,-0.31) (0.49,-0.52) (0.35,-0.56) colour red opacity 0.12
    polygon region points (0.31,-0.30) (0.42,-0.20) (0.52,-0.31) (0.49,-0.52) (0.35,-0.56) thickness 0.007 colour red jitter 0.9
    shade_triangle simplex_fill points (0.02,0.20) (0.16,-0.09) (-0.11,-0.09) colour green opacity 0.14
    triangle simplex points (0.02,0.20) (0.16,-0.09) (-0.11,-0.09) thickness 0.007 colour green jitter 1.2
    quad cell points (0.21,0.22) (0.32,0.17) (0.29,-0.10) (0.18,-0.05) thickness 0.007 colour blue jitter 1.0
    ray tangent from (-0.40,0.04) through (-0.31,0.16) thickness 0.007 colour red jitter 1.0
    point p at (-0.40,0.04) radius 0.015 colour red jitter 1.0
    open_point q at (-0.69,0.04) radius 0.020 thickness 0.006 colour red jitter 1.3
    line chord from (-0.66,0.20) to (-0.43,-0.13) thickness 0.006 colour grey jitter 1.4
    dotted_line quotient from (-0.69,0.04) to (-0.40,0.04) thickness 0.006 gap 0.044 colour red jitter 1.2
    arrow collapse from (-0.31,0.04) to (-0.15,0.04) thickness 0.007 head 0.056 colour blue jitter 1.2
    draw source during 0s..0.25s
    draw disk during 0s..0.25s
    draw neighbourhood during 0.04s..0.35s
    draw region_fill during 0.08s..0.35s
    draw region during 0.08s..0.35s
    draw simplex_fill during 0.08s..0.35s
    draw simplex during 0.08s..0.35s
    draw cell during 0.12s..0.35s
    draw tangent during 0.12s..0.35s
    draw chord during 0.1s..0.35s
    draw quotient during 0.05s..0.35s
    draw collapse during 0.12s..0.35s
    fade simplex_fill from 0 to 1 during 0.08s..0.22s
  patch glow opacity 0.35 blur 10 jitter 0.6
    circle halo center (-0.52,0.04) radius 0.27 thickness 0.017 colour green jitter 1.2

scene "projected path" duration 6s
  background radial center #ffffff edge #eef1f6
  patch model 3d opacity 1 jitter 1.2
    camera distance 4.5 scale 430
    axes3d frame at (-1.35,-0.95,-0.35) length 0.75 thickness 2 jitter 0.7
    point3d apex at (0,1.1,1.4) radius 9 colour red jitter 1.0
    open_point3d source3 at (-1.1,-0.2,0) radius 10 thickness 3 colour grey jitter 0.9
    shade_triangle3d sheet_fill points (-1.1,-0.2,0) (0,1.1,1.4) (1.1,-0.2,0.2) colour green opacity 0.10
    triangle3d sheet points (-1.1,-0.2,0) (0,1.1,1.4) (1.1,-0.2,0.2) thickness 3 colour green jitter 0.9
    line3d base from (-1.2,-0.7,0) to (1.2,-0.7,0) thickness 3 colour grey jitter 1.0
    curve3d arc through (-1.1,-0.2,0) (0,1.1,1.4) (1.1,-0.2,0.2) thickness 4 colour blue jitter 1.3
    orbit_camera model from 0.0 to 1.2 during 0s..4.0s
    draw sheet_fill during 0.04s..0.28s
    draw arc during 0s..0.35s
  patch labels opacity 1 jitter 0.8
    math label "$\gamma:[0,1]\to X$" at (-0.69,0.57) size 0.12 colour black jitter 0.8
  fade labels from 0 to 1 during 0.08s..0.28s
```
