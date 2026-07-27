video:
  output "patch_effects_smoke.mp4"

scene "patch effects smoke" 1s:
  background radial #fbf6ea #dce8ef
  patch halo at (0, 0) glow 14 glow_opacity 0.30 opacity 0.75 jitter 0.25:
    text title "patch effects":
      at (0, 0)
      size 0.12
      colour #17324d
  patch soft_note at (0, -0.35) blur 2:
    text note "isolated subtree":
      at (0, 0)
      size 0.07
      colour #2e5f7e
  fade halo from 0.20 to 0.75 during 0s..1s
