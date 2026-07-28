# Title Card Demo

```whiteboard
video fps 60 jitter_fps 15 output "title_card.mp4"

scene "title card" duration 5s
  background radial center #fcfbff edge #ebe7f7
  patch atmosphere opacity 0.20 blur 22 jitter 0.3
    shade_disc title_halo center (0,0) radius 0.80 colour purple opacity 0.12
    draw title_halo during 0s..1.6s
  patch formulas opacity 0.34 blur 10 jitter 0.2
    math top_seq "$0\to Hom(X,Y_n)\to C^n(X)\to H^n(X)\to 0$" at (-0.58,0.54) size 0.10 colour purple jitter off
    math fourier_bg "$F(f)(\xi)=\int f(x)e^{-2\pi i x\xi}dx$" at (-0.79,-0.71) size 0.13 colour purple jitter off
    draw top_seq during 0.2s..1.8s
    draw fourier_bg during 0.4s..2.0s
  patch structure opacity 0.52 blur 10 jitter 0.15
    line edge_a from (0.48,0.56) to (0.78,0.23) thickness 0.011 colour purple jitter 0.10
    line edge_b from (0.78,0.23) to (0.42,0.06) thickness 0.011 colour purple jitter 0.10
    line edge_c from (0.42,0.06) to (0.48,0.56) thickness 0.011 colour purple jitter 0.10
    point v0 at (0.48,0.56) radius 0.019 colour purple jitter off
    point v1 at (0.78,0.23) radius 0.019 colour purple jitter off
    point v2 at (0.42,0.06) radius 0.019 colour purple jitter off
    math simplex_label "$\Delta^2$" at (0.66,0.54) size 0.10 colour purple jitter off
    draw edge_a during 0.3s..1.2s
    draw edge_b during 0.6s..1.5s
    draw edge_c during 0.9s..1.8s
    draw v0 during 1.0s..1.6s
    draw v1 during 1.1s..1.7s
    draw v2 during 1.2s..1.8s
    draw simplex_label during 1.3s..2.0s
  patch board opacity 1 jitter 0.7
    math brand "$Whiteboard$" at (-0.68,-0.09) size 0.32 colour purple thickness 0.011 jitter off
    math sub "$mathematical$ $animation$ $software$" at (-0.52,-0.28) size 0.10 colour grey jitter off
    draw brand during 0s..2.2s
    draw sub during 1.2s..2.6s
```
