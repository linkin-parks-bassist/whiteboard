video:
  output "space_smoke.mp4"

scene "space shorthand":
  1s

  space model:
    camera:
      distance 6
      center (0,0)

    axes:
      (0,0,0)
      len 1.0

    cube:
      (0,0,0)
      size 1.0
