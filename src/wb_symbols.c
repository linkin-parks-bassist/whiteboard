#include "whiteboard.h"
#include "wb_captured_symbols.h"

static wb_symbol_variant scratch_variant;
static wb_symbol scratch_symbol;
static float wb_symbol_jitter_strength = 1.0f;

#define WB_MARKER_THICKNESS 2.0f

static uint32_t wb_hash_u32(uint32_t x)
{
	x ^= x >> 16;
	x *= 0x7feb352dU;
	x ^= x >> 15;
	x *= 0x846ca68bU;
	x ^= x >> 16;
	return x;
}

static float wb_seeded_unit(int seed)
{
	return (float)(wb_hash_u32((uint32_t)seed) & 0xffff) / 65535.0f;
}

static float wb_smoothstep(float t)
{
	t = binary_max(0.0f, binary_min(1.0f, t));
	return t * t * (3.0f - 2.0f * t);
}

static float wb_lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

static int wb_symbol_is_descender(int symbol_id)
{
	if (symbol_id >= WB_SYMBOL_ROMAN_BASE)
		symbol_id -= WB_SYMBOL_ROMAN_BASE;
	return symbol_id == 'g' || symbol_id == 'j' || symbol_id == 'p' || symbol_id == 'q' || symbol_id == 'y';
}

static int wb_symbol_is_tall_operator(int symbol_id)
{
	return symbol_id == WB_SYMBOL_SUM || symbol_id == WB_SYMBOL_PROD ||
		symbol_id == WB_SYMBOL_BIG_SUM || symbol_id == WB_SYMBOL_BIG_PROD ||
		symbol_id == WB_SYMBOL_INT || symbol_id == WB_SYMBOL_BIG_INT;
}

static float wb_symbol_baseline_offset(int symbol_id)
{
	if (symbol_id == WB_SYMBOL_MU)
		return 0.28f;
	return 0.0f;
}

static void wb_apply_symbol_metric_adjustments(wb_symbol_variant *v)
{
	float dy;
	
	if (!v)
		return;
	
	dy = wb_symbol_baseline_offset(v->symbol_id);
	if (dy != 0.0f)
	{
		v->bounds_min_y += dy;
		v->bounds_max_y += dy;
	}
	
	if (wb_symbol_is_tall_operator(v->symbol_id))
	{
		v->ascent = binary_max(v->ascent, 1.05f);
		v->descent = binary_max(v->descent, 0.25f);
		return;
	}
	
	if (wb_symbol_is_descender(v->symbol_id))
	{
		v->ascent = 0.72f;
		v->descent = 0.34f;
		return;
	}
	
	if (v->symbol_id == WB_SYMBOL_MU)
	{
		v->ascent = 0.62f;
		v->descent = 0.18f;
		return;
	}
	
	if ('0' <= v->symbol_id && v->symbol_id <= '9')
	{
		v->ascent = 0.86f;
		v->descent = 0.08f;
		return;
	}
	
	if (v->descent > 0.45f && v->ascent < 0.75f)
		v->descent = 0.34f;
}

static float wb_noise_lattice(int ix, int iy, int seed)
{
	uint32_t h = (uint32_t)seed;
	h ^= (uint32_t)ix * 0x9e3779b1U;
	h ^= (uint32_t)iy * 0x85ebca6bU;
	return (float)(wb_hash_u32(h) & 0xffff) / 32767.5f - 1.0f;
}

static float wb_value_noise(float x, float y, float frequency, int seed)
{
	float px = x * frequency;
	float py = y * frequency;
	int ix = (int)floorf(px);
	int iy = (int)floorf(py);
	float fx = wb_smoothstep(px - (float)ix);
	float fy = wb_smoothstep(py - (float)iy);
	float a = wb_noise_lattice(ix, iy, seed);
	float b = wb_noise_lattice(ix + 1, iy, seed);
	float c = wb_noise_lattice(ix, iy + 1, seed);
	float d = wb_noise_lattice(ix + 1, iy + 1, seed);
	return wb_lerp(wb_lerp(a, b, fx), wb_lerp(c, d, fx), fy);
}

static void jitter_figure_seeded(wb_plane_figure *fig, float jitter, int seed)
{
	if (!fig || !fig->curves)
		return;
	
	for (int i = 0; i < fig->n_curves; i++)
	{
		wb_nurbs_pcurve *curve = fig->curves[i];
		
		if (!curve || !curve->nx || !curve->ny || !curve->nx->control_points || !curve->ny->control_points)
			continue;
		
		int n = binary_min(curve->nx->n_control_points, curve->ny->n_control_points);
		float tx = (wb_seeded_unit(seed + i * 101) * 2.0f - 1.0f) * jitter * 0.45f;
		float ty = (wb_seeded_unit(seed + i * 101 + 53) * 2.0f - 1.0f) * jitter * 0.45f;
		
		for (int j = 0; j < n; j++)
		{
			float x = curve->nx->control_points[j];
			float y = curve->ny->control_points[j];
			float nx = wb_value_noise(x, y, 0.026f, seed + i * 337 + 17);
			float ny = wb_value_noise(x, y, 0.026f, seed + i * 337 + 191);
			
			curve->nx->control_points[j] += tx + nx * jitter * 0.55f;
			curve->ny->control_points[j] += ty + ny * jitter * 0.55f;
		}
	}
}

static void draw_rel_line(uint8_t *buf, float x, float baseline_y, float size, float x0, float y0, float x1, float y1, uint32_t colour)
{
	draw_sausage(buf, vec2(x + x0 * size, baseline_y + y0 * size), vec2(x + x1 * size, baseline_y + y1 * size), wb_marker_thickness(), colour);
}

static void draw_digit(uint8_t *buf, int digit, float x, float baseline_y, float size, uint32_t colour)
{
	static int segs[10][7] = {
		{1,1,1,1,1,1,0}, {0,1,1,0,0,0,0}, {1,1,0,1,1,0,1}, {1,1,1,1,0,0,1}, {0,1,1,0,0,1,1},
		{1,0,1,1,0,1,1}, {1,0,1,1,1,1,1}, {1,1,1,0,0,0,0}, {1,1,1,1,1,1,1}, {1,1,1,1,0,1,1}
	};
	float l = 0.08f, r = 0.48f, t = -0.72f, m = -0.36f, b = 0.02f;
	
	if (segs[digit][0]) draw_rel_line(buf, x, baseline_y, size, l, t, r, t, colour);
	if (segs[digit][1]) draw_rel_line(buf, x, baseline_y, size, r, t, r, m, colour);
	if (segs[digit][2]) draw_rel_line(buf, x, baseline_y, size, r, m, r, b, colour);
	if (segs[digit][3]) draw_rel_line(buf, x, baseline_y, size, l, b, r, b, colour);
	if (segs[digit][4]) draw_rel_line(buf, x, baseline_y, size, l, m, l, b, colour);
	if (segs[digit][5]) draw_rel_line(buf, x, baseline_y, size, l, t, l, m, colour);
	if (segs[digit][6]) draw_rel_line(buf, x, baseline_y, size, l, m, r, m, colour);
}

const wb_symbol *wb_get_symbol(int symbol_id)
{
	const wb_symbol_variant *variant = wb_get_symbol_variant(symbol_id, 0);
	
	if (!variant)
		return NULL;
	
	scratch_symbol.symbol_id = symbol_id;
	scratch_symbol.n_variants = 1;
	scratch_symbol.variants = &scratch_variant;
	
	return &scratch_symbol;
}

const wb_symbol_variant *wb_get_symbol_variant(int symbol_id, int variant_seed)
{
	const wb_captured_symbol_variant *captured = wb_get_captured_symbol_variant(symbol_id, variant_seed);
	if (!captured && symbol_id >= WB_SYMBOL_ROMAN_BASE)
		captured = wb_get_captured_symbol_variant(symbol_id - WB_SYMBOL_ROMAN_BASE, variant_seed);
	
	memset(&scratch_variant, 0, sizeof(scratch_variant));
	scratch_variant.symbol_id = symbol_id;
	scratch_variant.variant_id = variant_seed & 1;
	scratch_variant.ascent = 0.82f;
	scratch_variant.descent = 0.18f;
	scratch_variant.bounds_min_x = 0.0f;
	scratch_variant.bounds_min_y = -0.82f;
	scratch_variant.bounds_max_y = 0.18f;
	
	if (captured)
	{
		scratch_variant.figure = captured->figure;
		scratch_variant.advance = captured->advance;
		scratch_variant.bounds_min_x = captured->min_x;
		scratch_variant.bounds_min_y = captured->min_y;
		scratch_variant.bounds_max_x = captured->max_x;
		scratch_variant.bounds_max_y = captured->max_y;
		scratch_variant.ascent = binary_max(0.0f, -captured->min_y);
		scratch_variant.descent = binary_max(0.0f, captured->max_y);
		wb_apply_symbol_metric_adjustments(&scratch_variant);
		return &scratch_variant;
	}
	
	if ('0' <= symbol_id && symbol_id <= '9')
		scratch_variant.advance = 0.62f;
	else if (symbol_id == 'i' || symbol_id == 'l')
		scratch_variant.advance = 0.35f;
	else if (symbol_id == '.' || symbol_id == ',')
		scratch_variant.advance = 0.25f;
	else if (symbol_id == '(' || symbol_id == ')' || symbol_id == '[' || symbol_id == ']')
		scratch_variant.advance = 0.35f;
	else if (symbol_id == WB_SYMBOL_INT)
		scratch_variant.advance = 0.45f;
	else if (symbol_id == WB_SYMBOL_SUM || symbol_id == WB_SYMBOL_PROD)
		scratch_variant.advance = 0.75f;
	else
		scratch_variant.advance = 0.7f;
	
	scratch_variant.bounds_max_x = scratch_variant.advance;
	wb_apply_symbol_metric_adjustments(&scratch_variant);
	return &scratch_variant;
}

int wb_symbol_id_from_char(char c)
{
	if (c == ' ')
		return ' ';
	
	if (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9'))
		return c;
	
	switch (c)
	{
		case '+': case '-': case '=': case '<': case '>': case '(': case ')': case '[': case ']':
		case '{': case '}': case '|': case ',': case '.': case ':': case ';': case '*': case '/':
		case '!': case '?':
			return c;
	}
	
	return WB_SYMBOL_PLACEHOLDER;
}

int wb_symbol_id_from_custom_name(const char *name)
{
	uint32_t h = 2166136261U;
	if (!name)
		return WB_SYMBOL_PLACEHOLDER;
	while (*name)
	{
		h ^= (uint8_t)*name++;
		h *= 16777619U;
	}
	return WB_SYMBOL_CUSTOM_BASE + (int)(h % 100000U);
}

int wb_symbol_id_from_command(const char *cmd)
{
	if (!cmd)
		return WB_SYMBOL_PLACEHOLDER;
	
	if (strcmp(cmd, "alpha") == 0) return WB_SYMBOL_ALPHA;
	if (strcmp(cmd, "beta") == 0) return WB_SYMBOL_BETA;
	if (strcmp(cmd, "gamma") == 0) return WB_SYMBOL_GAMMA;
	if (strcmp(cmd, "delta") == 0) return WB_SYMBOL_DELTA;
	if (strcmp(cmd, "epsilon") == 0) return WB_SYMBOL_EPSILON;
	if (strcmp(cmd, "theta") == 0) return WB_SYMBOL_THETA;
	if (strcmp(cmd, "lambda") == 0) return WB_SYMBOL_LAMBDA;
	if (strcmp(cmd, "mu") == 0) return WB_SYMBOL_MU;
	if (strcmp(cmd, "pi") == 0) return WB_SYMBOL_PI;
	if (strcmp(cmd, "sigma") == 0) return WB_SYMBOL_SIGMA;
	if (strcmp(cmd, "phi") == 0) return WB_SYMBOL_PHI;
	if (strcmp(cmd, "omega") == 0) return WB_SYMBOL_OMEGA;
	if (strcmp(cmd, "sum") == 0) return WB_SYMBOL_SUM;
	if (strcmp(cmd, "Sum") == 0) return WB_SYMBOL_BIG_SUM;
	if (strcmp(cmd, "prod") == 0) return WB_SYMBOL_PROD;
	if (strcmp(cmd, "Prod") == 0) return WB_SYMBOL_BIG_PROD;
	if (strcmp(cmd, "int") == 0) return WB_SYMBOL_BIG_INT;
	if (strcmp(cmd, "leq") == 0) return WB_SYMBOL_LEQ;
	if (strcmp(cmd, "geq") == 0) return WB_SYMBOL_GEQ;
	if (strcmp(cmd, "neq") == 0) return WB_SYMBOL_NEQ;
	if (strcmp(cmd, "approx") == 0) return WB_SYMBOL_APPROX;
	if (strcmp(cmd, "to") == 0) return WB_SYMBOL_TO;
	if (strcmp(cmd, "infty") == 0) return WB_SYMBOL_INFINITY;
	if (strcmp(cmd, "cdot") == 0) return WB_SYMBOL_CDOT;
	if (strcmp(cmd, "times") == 0) return WB_SYMBOL_TIMES;
	if (strcmp(cmd, "pm") == 0) return WB_SYMBOL_PM;
	if (strcmp(cmd, "partial") == 0) return WB_SYMBOL_PARTIAL;
	if (strcmp(cmd, "nabla") == 0) return WB_SYMBOL_NABLA;
	if (strcmp(cmd, "forall") == 0) return WB_SYMBOL_FORALL;
	if (strcmp(cmd, "exists") == 0) return WB_SYMBOL_EXISTS;
	if (strcmp(cmd, "in") == 0) return WB_SYMBOL_IN;
	if (strcmp(cmd, "notin") == 0) return WB_SYMBOL_NOTIN;
	if (strcmp(cmd, "subset") == 0) return WB_SYMBOL_SUBSET;
	if (strcmp(cmd, "subseteq") == 0) return WB_SYMBOL_SUBSETEQ;
	if (strcmp(cmd, "emptyset") == 0) return WB_SYMBOL_EMPTYSET;
	if (strcmp(cmd, "cup") == 0) return WB_SYMBOL_CUP;
	if (strcmp(cmd, "cap") == 0) return WB_SYMBOL_CAP;
	if (strcmp(cmd, "sim") == 0) return WB_SYMBOL_SIM;
	if (strcmp(cmd, "equiv") == 0) return WB_SYMBOL_EQUIV;
	if (strcmp(cmd, "propto") == 0) return WB_SYMBOL_PROPORTO;
	if (strcmp(cmd, "perp") == 0) return WB_SYMBOL_PERP;
	if (strcmp(cmd, "parallel") == 0) return WB_SYMBOL_PARALLEL;
	
	return wb_symbol_id_from_custom_name(cmd);
}

float wb_symbol_spacing(int left_symbol_id, int right_symbol_id, float size)
{
	if (left_symbol_id == ' ' || right_symbol_id == ' ')
		return size * 0.18f;
	
	(void)left_symbol_id;
	(void)right_symbol_id;
	return size * 0.04f;
}

float wb_marker_thickness(void)
{
	return WB_MARKER_THICKNESS;
}

void wb_set_symbol_jitter_strength(float strength)
{
	if (strength < 0.0f)
		strength = 0.0f;
	wb_symbol_jitter_strength = strength;
}

void wb_debug_print_symbol_metrics(int symbol_id)
{
	const wb_symbol_variant *v = wb_get_symbol_variant(symbol_id, 0);
	
	if (!v)
	{
		printf("symbol %d: missing\n", symbol_id);
		return;
	}
	
	printf("symbol %d: advance=%.3f ascent=%.3f descent=%.3f bounds=(%.3f, %.3f)..(%.3f, %.3f)\n",
		symbol_id, v->advance, v->ascent, v->descent,
		v->bounds_min_x, v->bounds_min_y, v->bounds_max_x, v->bounds_max_y);
}

void wb_draw_symbol(uint8_t *buf, int symbol_id, float x, float baseline_y, float size, uint32_t colour, int seed)
{
	const wb_captured_symbol_variant *captured = wb_get_captured_symbol_variant(symbol_id, seed);
	if (!captured && symbol_id >= WB_SYMBOL_ROMAN_BASE)
		captured = wb_get_captured_symbol_variant(symbol_id - WB_SYMBOL_ROMAN_BASE, seed);
	
	if (captured)
	{
		wb_plane_figure *fig = clone_plane_figure(captured->figure);
		
		if (!fig)
			return;
		
		scale_plane_figure(fig, size);
		translate_plane_figure(fig, x, baseline_y);
		translate_plane_figure(fig, 0.0f, wb_symbol_baseline_offset(symbol_id) * size);
		if (wb_symbol_jitter_strength > 0.0f)
		{
			jitter_figure_seeded(fig, size * 0.024f * wb_symbol_jitter_strength, seed);
			translate_plane_figure(fig,
				(wb_seeded_unit(seed + 7001) * 2.0f - 1.0f) * size * 0.006f * wb_symbol_jitter_strength,
				(wb_seeded_unit(seed + 7009) * 2.0f - 1.0f) * size * 0.006f * wb_symbol_jitter_strength);
		}
		draw_plane_figure_in_colour(buf, fig, colour);
		free_plane_figure(fig);
		return;
	}
	
	(void)buf;
	(void)x;
	(void)baseline_y;
	(void)size;
	(void)colour;
}
