// ============================================================================
// ARCADE_UNDEF.H - Clear shared macros between game .cpp files
// Include this at the top of each game's .cpp BEFORE defining new macros
// Required because Arduino IDE includes all .cpp in a single translation unit
// ============================================================================
#undef ROM_ARR
#undef ROM_CPU1_ARR
#undef ROM_CPU2_ARR
#undef ROM_CPU3_ARR
#undef ROM_CPU1_B0_ARR
#undef ROM_CPU1_B1_ARR
#undef ROM_CPU1_B2_ARR
#undef TILEMAP_ARR
#undef SPRITES_ARR
#undef COLORMAP_ARR
#undef CMAP_TILES_ARR
#undef CMAP_SPRITES_ARR
#undef CMAP_CHARS_ARR
#undef CMAP_SPRITE_ARR
#undef CHARMAP_ARR
#undef WAVE_ARR
#undef LOGO_ARR
#undef SAMPLE_BOOM_ARR
#undef SAMPLE_BOOM_SIZE
#undef STARSEED_ARR
#undef PFTILES_ARR
#undef COLORMAPS_ARR
#undef PLAYFIELD_ARR
#undef SAMPLE_WALK0_ARR
#undef SAMPLE_WALK0_SIZE
#undef SAMPLE_WALK1_ARR
#undef SAMPLE_WALK1_SIZE
#undef SAMPLE_WALK2_ARR
#undef SAMPLE_WALK2_SIZE
#undef SAMPLE_JUMP_ARR
#undef SAMPLE_JUMP_SIZE
#undef SAMPLE_STOMP_ARR
#undef SAMPLE_STOMP_SIZE
