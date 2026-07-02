# Rotating Tetrahedron

```whiteboard
video fps 60 jitter_fps 15 output "tetrahedron_demo.mp4"

scene "rotating tetrahedron" duration 5s
  background radial center #ffffff edge #eef1f6
  layer model 3d opacity 1 jitter 1.0
    camera distance 4.8 scale 470 yaw -0.7 center (960,560)
    point3d a at (0.0,1.1,0.0) radius 8 colour red jitter 0.9
    point3d b at (-1.0,-0.6,-0.8) radius 8 colour blue jitter 0.9
    point3d c at (1.0,-0.6,-0.8) radius 8 colour green jitter 0.9
    point3d d at (0.0,-0.2,1.1) radius 8 colour grey jitter 0.9
    shade_triangle3d abc_fill points (0.0,1.1,0.0) (-1.0,-0.6,-0.8) (1.0,-0.6,-0.8) colour red opacity 0.08
    shade_triangle3d abd_fill points (0.0,1.1,0.0) (-1.0,-0.6,-0.8) (0.0,-0.2,1.1) colour blue opacity 0.08
    shade_triangle3d acd_fill points (0.0,1.1,0.0) (1.0,-0.6,-0.8) (0.0,-0.2,1.1) colour green opacity 0.08
    shade_triangle3d bcd_fill points (-1.0,-0.6,-0.8) (1.0,-0.6,-0.8) (0.0,-0.2,1.1) colour grey opacity 0.06
    triangle3d abc points (0.0,1.1,0.0) (-1.0,-0.6,-0.8) (1.0,-0.6,-0.8) thickness 3 colour red jitter 1.0
    triangle3d abd points (0.0,1.1,0.0) (-1.0,-0.6,-0.8) (0.0,-0.2,1.1) thickness 3 colour blue jitter 1.0
    triangle3d acd points (0.0,1.1,0.0) (1.0,-0.6,-0.8) (0.0,-0.2,1.1) thickness 3 colour green jitter 1.0
    triangle3d bcd points (-1.0,-0.6,-0.8) (1.0,-0.6,-0.8) (0.0,-0.2,1.1) thickness 3 colour grey jitter 1.0
    orbit_camera model from -0.7 to 5.1 during 0s..5s
  layer labels 2d opacity 1 jitter 0.7
    math title "$\Delta^3$" at (270,230) size 82 colour black jitter off
    math sub "$rotating$ $tetrahedron$" at (270,310) size 52 colour grey jitter off
    draw title during 0s..0.7s
    draw sub during 0.2s..0.9s
```
