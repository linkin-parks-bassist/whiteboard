#ifndef WB_CAPTURED_SYMBOLS_H_
#define WB_CAPTURED_SYMBOLS_H_

typedef struct
{
	wb_plane_figure *figure;
	float advance;
	float min_x;
	float min_y;
	float max_x;
	float max_y;
} wb_captured_symbol_variant;

wb_plane_figure *wb_get_captured_symbol_figure(int symbol_id, int variant_seed);
const wb_captured_symbol_variant *wb_get_captured_symbol_variant(int symbol_id, int variant_seed);

#endif
