# Rotating Tetrahedron

```whiteboard
video fps 60 jitter_fps 15 output "tetrahedron_demo.mp4"

scene "rotating tetrahedron" duration 5s
  background radial center #ffffff edge #eef1f6
  layer model 3d opacity 1 jitter 1.0
    camera distance 4.8 scale 470 yaw -0.7 center (960,560)
    axes3d frame at (-1.55,-1.15,-1.25) length 0.65 thickness 2 jitter 0.7
    cube3d backdrop center (1.55,0.05,0.25) size 0.7 thickness 2 colour grey opacity 0.04 jitter 0.7
    tetrahedron3d tet points (0.0,1.1,0.0) (-1.0,-0.6,-0.8) (1.0,-0.6,-0.8) (0.0,-0.2,1.1) thickness 3 colour blue opacity 0.09 jitter 1.0
    orbit_camera model from -0.7 to 5.1 during 0s..5s
    draw tet during 0s..0.9s
    fade backdrop from 0 to 1 during 0.15s..1.0s
  layer labels 2d opacity 1 jitter 0.7
    math title "$\Delta^3$" at (270,230) size 82 colour black jitter off
    math sub "$rotating$ $tetrahedron$" at (270,310) size 52 colour grey jitter off
    draw title during 0s..0.7s
    draw sub during 0.2s..0.9s
```
