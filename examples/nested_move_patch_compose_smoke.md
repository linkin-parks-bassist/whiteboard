video:
  output "nested_move_patch_compose_smoke.mp4"

scene "nested move patch compose":
  duration 1.2s

  patch parent:
    at (0.50w,0.56h)
    scale (220,-170)

    seg x_axis:
      from (-1.2,0.0) to (1.2,0.0)
      thickness 2.0
      colour grey

    patch child:
      at (0.0,0.0)

      tri marker:
        points (-0.20,-0.12) (0.20,-0.12) (0.00,0.18)
        thickness 3.0
        colour blue

      pt tip:
        at (0.0,0.22)
        radius 7
        colour red

  move_patch parent:
    (0.0,0.0) -> (0.55,0.15)
    0.0s..1.2s

  move_patch child:
    (0.0,0.0) -> (-0.18,0.42)
    0.0s..1.2s
