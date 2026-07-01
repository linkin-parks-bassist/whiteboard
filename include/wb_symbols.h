#ifndef WB_SYMBOLS_H_
#define WB_SYMBOLS_H_

#define WB_SYMBOL_ALPHA 	256
#define WB_SYMBOL_BETA 		257
#define WB_SYMBOL_GAMMA 	258
#define WB_SYMBOL_DELTA 	259
#define WB_SYMBOL_EPSILON 	260
#define WB_SYMBOL_THETA 	261
#define WB_SYMBOL_LAMBDA 	262
#define WB_SYMBOL_MU 		263
#define WB_SYMBOL_PI 		264
#define WB_SYMBOL_SIGMA 	265
#define WB_SYMBOL_PHI 		266
#define WB_SYMBOL_OMEGA 	267
#define WB_SYMBOL_SUM 		268
#define WB_SYMBOL_PROD 		269
#define WB_SYMBOL_INT 		270
#define WB_SYMBOL_LEQ 		271
#define WB_SYMBOL_GEQ 		272
#define WB_SYMBOL_NEQ 		273
#define WB_SYMBOL_APPROX 	274
#define WB_SYMBOL_TO 		275
#define WB_SYMBOL_INFINITY 	276
#define WB_SYMBOL_CDOT 	277
#define WB_SYMBOL_TIMES 	278
#define WB_SYMBOL_PM 		279
#define WB_SYMBOL_SQRT		280
#define WB_SYMBOL_FRACBAR	281
#define WB_SYMBOL_PARTIAL	282
#define WB_SYMBOL_NABLA		283
#define WB_SYMBOL_FORALL	284
#define WB_SYMBOL_EXISTS	285
#define WB_SYMBOL_IN		286
#define WB_SYMBOL_NOTIN		287
#define WB_SYMBOL_SUBSET	288
#define WB_SYMBOL_SUBSETEQ	289
#define WB_SYMBOL_EMPTYSET	290
#define WB_SYMBOL_CUP		291
#define WB_SYMBOL_CAP		292
#define WB_SYMBOL_SIM		293
#define WB_SYMBOL_EQUIV		294
#define WB_SYMBOL_PROPORTO	295
#define WB_SYMBOL_PERP		296
#define WB_SYMBOL_PARALLEL	297
#define WB_SYMBOL_BIG_SUM	298
#define WB_SYMBOL_BIG_PROD	299
#define WB_SYMBOL_BIG_INT	300
#define WB_SYMBOL_PLACEHOLDER 301
#define WB_SYMBOL_CUSTOM_BASE 20000
#define WB_SYMBOL_ROMAN_BASE 10000

typedef struct
{
	int symbol_id;
	int variant_id;
	wb_plane_figure *figure;
	float advance;
	float left_bearing;
	float right_bearing;
	float ascent;
	float descent;
	float bounds_min_x;
	float bounds_min_y;
	float bounds_max_x;
	float bounds_max_y;
} wb_symbol_variant;

typedef struct
{
	int symbol_id;
	int n_variants;
	wb_symbol_variant *variants;
} wb_symbol;

typedef struct
{
	int left_symbol_id;
	int right_symbol_id;
	float adjustment;
} wb_kerning_pair;

const wb_symbol *wb_get_symbol(int symbol_id);
const wb_symbol_variant *wb_get_symbol_variant(int symbol_id, int variant_seed);
int wb_symbol_id_from_char(char c);
int wb_symbol_id_from_command(const char *cmd);
int wb_symbol_id_from_custom_name(const char *name);
float wb_symbol_spacing(int left_symbol_id, int right_symbol_id, float size);
float wb_marker_thickness(void);
void wb_set_symbol_jitter_strength(float strength);
void wb_debug_print_symbol_metrics(int symbol_id);
void wb_draw_symbol(uint8_t *buf, int symbol_id, float x, float baseline_y, float size, uint32_t colour, int seed);

#endif
