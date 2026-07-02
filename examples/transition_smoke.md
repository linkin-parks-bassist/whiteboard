video fps 60 jitter_fps 15 output "transition_smoke.mp4"

scene "one" duration 1.0s
background radial center #f7f3ff edge #ddd5ff
math a "$A$" at (680,540) size 180 colour purple
transition crossfade 0.4s

scene "two" duration 1.0s
background radial center #ffffff edge #dcecff
math b "$B$" at (1240,540) size 180 colour blue
