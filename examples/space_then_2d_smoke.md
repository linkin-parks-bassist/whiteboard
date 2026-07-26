video:
  output "space_then_2d_smoke.mp4"

scene "space returns to root":
  1s

  space model:
    camera:
      distance 6

    axes:
      (0,0,0)
      len 1

  circ marker:
    (0,0)
    radius 0.25
    colour red
