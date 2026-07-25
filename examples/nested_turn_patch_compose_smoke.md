video:
  output "nested_turn_patch_compose_smoke.mp4"

scene "nested turn patch compose":
  duration 1.2s

  patch parent:
    at (0.50w,0.56h)
    scale (220,-170)

    seg axis:
      from (-1.1,0.0) to (1.1,0.0)
      thickness 2.0
      colour grey

    patch child:
      at (0.45,0.0)

      seg arm:
        from (0.0,0.0) to (0.55,0.0)
        thickness 3.0
        colour blue

      pt tip:
        at (0.65,0.0)
        radius 7
        colour red

  turn_patch parent:
    0.0 -> 0.8
    0.0s..1.2s

  turn_patch child:
    0.0 -> -1.0
    0.0s..1.2s
