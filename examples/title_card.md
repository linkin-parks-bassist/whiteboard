# Title Card Demo

```whiteboard
video fps 60 jitter_fps 15 output "title_card.mp4"

scene "title card" duration 0.28s
  background radial center #fefefe edge #eef1f6
  layer glow 2d opacity 0.32 blur 18 jitter 0.4
    math title_glow "$x^2+y^2$" at (960,520) size 110 colour blue jitter off
    shade_disc halo center (960,520) radius 420 colour blue opacity 0.06
  layer board 2d opacity 1 jitter 1.0
    math brand "$Whiteboard$" at (520,530) size 220 colour black jitter off
    math accent_left "$\\alpha+\\beta$" at (300,300) size 78 colour red jitter 0.6
    math accent_right "$\\gamma$" at (1570,730) size 100 colour green jitter 0.6
    ellipse orbit_a center (500,360) radii (84,62) thickness 4 colour blue jitter 1.0
    circle orbit_b center (1400,355) radius 56 thickness 4 colour red jitter 1.0
    triangle tri_a points (470,760) (640,640) (760,810) thickness 4 colour grey jitter 0.9
    quad quad_a points (1160,760) (1310,650) (1460,785) (1285,860) thickness 4 colour blue jitter 0.9
    ray sweep from (1100,300) through (1510,230) thickness 3 colour grey jitter 0.7
    draw title_glow during 0s..0.28s
    draw brand during 0s..0.28s
    draw accent_left during 0.03s..0.28s
    draw accent_right during 0.06s..0.28s
    draw orbit_a during 0.08s..0.28s
    draw orbit_b during 0.08s..0.28s
    draw tri_a during 0.1s..0.28s
    draw quad_a during 0.12s..0.28s
    draw sweep during 0.09s..0.28s
```
