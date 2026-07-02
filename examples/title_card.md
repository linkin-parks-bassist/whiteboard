# Title Card Demo

```whiteboard
video fps 60 jitter_fps 15 output "title_card.mp4"

scene "title card" duration 0.28s
  background radial center #fcfbff edge #ebe7f7
  layer atmosphere 2d opacity 0.20 blur 22 jitter 0.3
    shade_disc title_halo center (960,540) radius 430 colour purple opacity 0.12
    draw title_halo during 0s..0.16s
  layer formulas 2d opacity 0.34 blur 10 jitter 0.2
    math top_seq "$0\to Hom(X,Y_n)\to C^n(X)\to H^n(X)\to 0$" at (400,250) size 54 colour purple jitter off
    math fourier_bg "$F(f)(\xi)=\int f(x)e^{-2\pi i x\xi}dx$" at (200,925) size 74 colour purple jitter off
    draw top_seq during 0.02s..0.18s
    draw fourier_bg during 0.04s..0.18s
  layer structure 2d opacity 0.52 blur 10 jitter 0.15
    line edge_a from (1420,235) to (1710,415) thickness 6 colour purple jitter 0.10
    line edge_b from (1710,415) to (1365,510) thickness 6 colour purple jitter 0.10
    line edge_c from (1365,510) to (1420,235) thickness 6 colour purple jitter 0.10
    point v0 at (1420,235) radius 10 colour purple jitter off
    point v1 at (1710,415) radius 10 colour purple jitter off
    point v2 at (1365,510) radius 10 colour purple jitter off
    math simplex_label "$\Delta^2$" at (1590,250) size 56 colour purple jitter off
    draw edge_a during 0.03s..0.12s
    draw edge_b during 0.05s..0.14s
    draw edge_c during 0.07s..0.16s
    draw v0 during 0.09s..0.14s
    draw v1 during 0.09s..0.14s
    draw v2 during 0.09s..0.14s
    draw simplex_label during 0.10s..0.16s
  layer board 2d opacity 1 jitter 0.7
    math brand "$Whiteboard$" at (305,590) size 230 colour purple thickness 5.8 jitter off
    math sub "$mathematical$ $animation$ $software$" at (460,690) size 56 colour grey jitter off
    draw brand during 0s..0.18s
    draw sub during 0.08s..0.18s
```
