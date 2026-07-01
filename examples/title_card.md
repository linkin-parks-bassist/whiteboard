# Title Card Demo

```whiteboard
video fps 60 jitter_fps 15 output "title_card.mp4"

scene "title card" duration 0.28s
  background radial center #fefefe edge #eef1f6
  layer glow 2d opacity 0.32 blur 18 jitter 0.4
    math title_glow "$\\int_0^\\infty e^{-x^2}\\,dx$" at (960,520) size 130 colour blue jitter off
    shade_disc halo center (960,520) radius 420 colour blue opacity 0.08
  layer board 2d opacity 1 jitter 1.0
    math brand "$\\mathrm{Whiteboard}$" at (960,520) size 180 colour black jitter off
    math accent_left "$\\mathbb{R}$" at (330,290) size 90 colour red jitter 0.7
    math accent_right "$\\sum$" at (1570,760) size 110 colour green jitter 0.7
    circle orbit_a center (520,360) radius 78 thickness 4 colour blue jitter 1.2
    circle orbit_b center (1410,360) radius 58 thickness 4 colour red jitter 1.2
    triangle tri_a points (480,770) (650,640) (760,820) thickness 4 colour grey jitter 1.0
    quad quad_a points (1170,760) (1320,650) (1470,790) (1280,860) thickness 4 colour blue jitter 1.0
    draw title_glow during 0s..0.28s
    draw brand during 0s..0.28s
    draw accent_left during 0.03s..0.28s
    draw accent_right during 0.06s..0.28s
    draw orbit_a during 0.08s..0.28s
    draw orbit_b during 0.08s..0.28s
    draw tri_a during 0.1s..0.28s
    draw quad_a during 0.12s..0.28s
```
