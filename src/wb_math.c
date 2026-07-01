#include "whiteboard.h"

#define WB_MATH_SYMBOL 	1
#define WB_MATH_ROW 	2
#define WB_MATH_FRAC 	3
#define WB_MATH_SCRIPT 	4
#define WB_MATH_SQRT 	5

#define WB_MATH_SCRIPT_SCALE 0.65f
#define WB_MATH_FRAC_SCALE 0.78f

static float wb_math_jitter_strength = 1.0f;

typedef struct wb_math_node
{
	int type;
	int symbol_id;
	struct wb_math_node **children;
	int n_children;
	int cap_children;
	struct wb_math_node *base;
	struct wb_math_node *sup;
	struct wb_math_node *sub;
	struct wb_math_node *a;
	struct wb_math_node *b;
} wb_math_node;

struct wb_math_formula
{
	wb_math_node *root;
};

typedef struct
{
	const char *s;
	int math_mode;
	char error[128];
} wb_math_parser;

typedef struct
{
	float w;
	float ascent;
	float descent;
	int first_symbol;
	int last_symbol;
} wb_math_box;

static wb_math_node *new_node(int type)
{
	wb_math_node *n = malloc(sizeof(wb_math_node));
	
	if (!n)
		return NULL;
	
	memset(n, 0, sizeof(wb_math_node));
	n->type = type;
	return n;
}

static wb_math_node *new_symbol(int symbol_id)
{
	wb_math_node *n = new_node(WB_MATH_SYMBOL);
	
	if (n)
		n->symbol_id = symbol_id;
	
	return n;
}

static int row_append(wb_math_node *row, wb_math_node *child)
{
	if (!row || !child || row->type != WB_MATH_ROW)
		return 0;
	
	if (row->n_children >= row->cap_children)
	{
		int new_cap = row->cap_children ? row->cap_children * 2 : 8;
		wb_math_node **children = realloc(row->children, sizeof(wb_math_node*) * new_cap);
		
		if (!children)
			return 0;
		
		row->children = children;
		row->cap_children = new_cap;
	}
	
	row->children[row->n_children++] = child;
	return 1;
}

static void free_node(wb_math_node *n)
{
	if (!n)
		return;
	
	for (int i = 0; i < n->n_children; i++)
		free_node(n->children[i]);
	
	free(n->children);
	free_node(n->base);
	free_node(n->sup);
	free_node(n->sub);
	free_node(n->a);
	free_node(n->b);
	free(n);
}

static void skip_spaces(wb_math_parser *p)
{
	if (!p->math_mode)
		return;
	
	while (*p->s == ' ' || *p->s == '\t' || *p->s == '\n' || *p->s == '\r')
		p->s++;
}

static int is_command_char(char c)
{
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

static void set_error(wb_math_parser *p, const char *msg)
{
	if (p->error[0] == 0)
		snprintf(p->error, sizeof(p->error), "%s", msg);
}

static wb_math_node *parse_row(wb_math_parser *p, int stop_at_brace);
static wb_math_node *parse_command(wb_math_parser *p);

static int parse_char_symbol_id(wb_math_parser *p, char c)
{
	int symbol_id = wb_symbol_id_from_char(c);
	
	if (!p->math_mode && symbol_id != ' ')
		symbol_id += WB_SYMBOL_ROMAN_BASE;
	
	return symbol_id;
}

static wb_math_node *parse_required_group(wb_math_parser *p)
{
	skip_spaces(p);
	
	if (*p->s == '{')
	{
		p->s++;
		wb_math_node *row = parse_row(p, 1);
		
		if (*p->s != '}')
		{
			set_error(p, "missing closing brace");
			free_node(row);
			return NULL;
		}
		
		p->s++;
		return row;
	}
	
	if (*p->s == 0)
	{
		set_error(p, "missing group");
		return NULL;
	}
	
	if (*p->s == '\\')
		return parse_command(p);
	
	return new_symbol(parse_char_symbol_id(p, *p->s++));
}

static wb_math_node *command_as_text(const char *cmd)
{
	wb_math_node *row = new_node(WB_MATH_ROW);
	
	if (!row)
		return NULL;
	
	for (int i = 0; cmd[i]; i++)
	{
		if (!row_append(row, new_symbol(wb_symbol_id_from_char(cmd[i]))))
		{
			free_node(row);
			return NULL;
		}
		row->children[row->n_children - 1]->symbol_id += WB_SYMBOL_ROMAN_BASE;
	}
	
	return row;
}

static wb_math_node *parse_command(wb_math_parser *p)
{
	char cmd[32];
	int i = 0;
	
	if (*p->s != '\\')
		return NULL;
	
	p->s++;
	
	if (!p->math_mode)
	{
		if (*p->s)
			return new_symbol(parse_char_symbol_id(p, *p->s++));
		
		set_error(p, "dangling escape");
		return NULL;
	}
	
	if (!is_command_char(*p->s))
	{
		if (*p->s)
			return new_symbol(wb_symbol_id_from_char(*p->s++));
		
		set_error(p, "dangling command");
		return NULL;
	}
	
	while (is_command_char(*p->s) && i < (int)sizeof(cmd) - 1)
		cmd[i++] = *p->s++;
	
	while (is_command_char(*p->s))
		p->s++;
	
	cmd[i] = 0;
	
	if (strcmp(cmd, "frac") == 0)
	{
		wb_math_node *n = new_node(WB_MATH_FRAC);
		
		if (!n)
			return NULL;
		
		n->a = parse_required_group(p);
		n->b = parse_required_group(p);
		
		if (!n->a || !n->b)
		{
			free_node(n);
			return NULL;
		}
		
		return n;
	}
	
	if (strcmp(cmd, "sqrt") == 0)
	{
		wb_math_node *n = new_node(WB_MATH_SQRT);
		
		if (!n)
			return NULL;
		
		n->a = parse_required_group(p);
		
		if (!n->a)
		{
			free_node(n);
			return NULL;
		}
		
		return n;
	}

	if (strcmp(cmd, "mathbb") == 0)
	{
		skip_spaces(p);
		if (*p->s != '{' || !p->s[1] || p->s[2] != '}')
		{
			set_error(p, "mathbb expects one-character group");
			return NULL;
		}
		char name[16];
		snprintf(name, sizeof(name), "mathbb:%c", p->s[1]);
		p->s += 3;
		return new_symbol(wb_symbol_id_from_custom_name(name));
	}
	
	if (strcmp(cmd, "sin") == 0 || strcmp(cmd, "cos") == 0 || strcmp(cmd, "tan") == 0 ||
		strcmp(cmd, "log") == 0 || strcmp(cmd, "ln") == 0 || strcmp(cmd, "exp") == 0 ||
		strcmp(cmd, "lim") == 0)
		return command_as_text(cmd);
	
	return new_symbol(wb_symbol_id_from_command(cmd));
}

static wb_math_node *parse_atom(wb_math_parser *p)
{
	skip_spaces(p);
	
	if (*p->s == 0 || *p->s == '}')
		return NULL;
	
	if (*p->s == '{')
	{
		p->s++;
		wb_math_node *row = parse_row(p, 1);
		
		if (*p->s != '}')
		{
			set_error(p, "missing closing brace");
			free_node(row);
			return NULL;
		}
		
		p->s++;
		return row;
	}
	
	if (*p->s == '\\')
		return parse_command(p);
	
	return new_symbol(parse_char_symbol_id(p, *p->s++));
}

static wb_math_node *parse_scripts(wb_math_parser *p, wb_math_node *base)
{
	wb_math_node *sup = NULL;
	wb_math_node *sub = NULL;
	int found = 0;
	
	while (1)
	{
		skip_spaces(p);
		
		if (*p->s == '^')
		{
			p->s++;
			free_node(sup);
			sup = parse_required_group(p);
			found = 1;
		}
		else if (*p->s == '_')
		{
			p->s++;
			free_node(sub);
			sub = parse_required_group(p);
			found = 1;
		}
		else
			break;
	}
	
	if (!found)
		return base;
	
	wb_math_node *n = new_node(WB_MATH_SCRIPT);
	
	if (!n)
	{
		free_node(base);
		free_node(sup);
		free_node(sub);
		return NULL;
	}
	
	n->base = base;
	n->sup = sup;
	n->sub = sub;
	return n;
}

static wb_math_node *parse_row(wb_math_parser *p, int stop_at_brace)
{
	wb_math_node *row = new_node(WB_MATH_ROW);
	
	if (!row)
		return NULL;
	
	while (*p->s)
	{
		skip_spaces(p);
		
		if (stop_at_brace && *p->s == '}')
			break;
		
		if (*p->s == 0)
			break;
		
		if (*p->s == '$')
		{
			p->math_mode = !p->math_mode;
			p->s++;
			continue;
		}
		
		wb_math_node *atom = parse_atom(p);
		
		if (!atom)
			break;
		
		if (p->math_mode)
			atom = parse_scripts(p, atom);
		
		if (!atom || !row_append(row, atom))
		{
			free_node(atom);
			free_node(row);
			return NULL;
		}
	}
	
	return row;
}

static wb_math_box measure_node(wb_math_node *n, float size);

static wb_math_box empty_box(void)
{
	wb_math_box b;
	b.w = 0.0f;
	b.ascent = 0.0f;
	b.descent = 0.0f;
	b.first_symbol = 0;
	b.last_symbol = 0;
	return b;
}

static wb_math_box measure_symbol_node(wb_math_node *n, float size)
{
	wb_math_box b = empty_box();
	const wb_symbol_variant *v = wb_get_symbol_variant(n->symbol_id, 0);
	
	if (!v)
		return b;
	
	b.w = v->advance * size;
	b.ascent = v->ascent * size;
	b.descent = v->descent * size;
	b.first_symbol = n->symbol_id;
	b.last_symbol = n->symbol_id;
	return b;
}

static wb_math_box measure_row_node(wb_math_node *n, float size)
{
	wb_math_box b = empty_box();
	int last_symbol = 0;
	
	for (int i = 0; i < n->n_children; i++)
	{
		wb_math_box cb = measure_node(n->children[i], size);
		
		if (i > 0)
			b.w += wb_symbol_spacing(last_symbol, cb.first_symbol, size);
		
		if (!b.first_symbol)
			b.first_symbol = cb.first_symbol;
		
		b.last_symbol = cb.last_symbol;
		last_symbol = cb.last_symbol;
		b.w += cb.w;
		b.ascent = binary_max(b.ascent, cb.ascent);
		b.descent = binary_max(b.descent, cb.descent);
	}
	
	return b;
}

static wb_math_box measure_node(wb_math_node *n, float size)
{
	wb_math_box b = empty_box();
	
	if (!n)
		return b;
	
	switch (n->type)
	{
		case WB_MATH_SYMBOL:
			return measure_symbol_node(n, size);
		case WB_MATH_ROW:
			return measure_row_node(n, size);
		case WB_MATH_FRAC:
		{
			wb_math_box top = measure_node(n->a, size * WB_MATH_FRAC_SCALE);
			wb_math_box bot = measure_node(n->b, size * WB_MATH_FRAC_SCALE);
			b.w = binary_max(top.w, bot.w) + size * 0.28f;
			b.ascent = top.ascent + top.descent + size * 0.42f;
			b.descent = bot.ascent + bot.descent + size * 0.30f;
			b.first_symbol = top.first_symbol ? top.first_symbol : bot.first_symbol;
			b.last_symbol = bot.last_symbol ? bot.last_symbol : top.last_symbol;
			return b;
		}
		case WB_MATH_SCRIPT:
		{
			wb_math_box base = measure_node(n->base, size);
			wb_math_box sup = measure_node(n->sup, size * WB_MATH_SCRIPT_SCALE);
			wb_math_box sub = measure_node(n->sub, size * WB_MATH_SCRIPT_SCALE);
			b.w = base.w + binary_max(sup.w, sub.w);
			b.ascent = binary_max(base.ascent, base.ascent * 0.72f + sup.ascent + sup.descent);
			b.descent = binary_max(base.descent, sub.ascent + sub.descent + size * 0.04f);
			b.first_symbol = base.first_symbol;
			b.last_symbol = sub.last_symbol ? sub.last_symbol : (sup.last_symbol ? sup.last_symbol : base.last_symbol);
			return b;
		}
		case WB_MATH_SQRT:
		{
			wb_math_box inner = measure_node(n->a, size);
			b.w = inner.w + size * 0.38f;
			b.ascent = inner.ascent + size * 0.12f;
			b.descent = inner.descent;
			b.first_symbol = WB_SYMBOL_PLACEHOLDER;
			b.last_symbol = inner.last_symbol;
			return b;
		}
	}
	
	return b;
}

static void draw_node(uint8_t *buf, wb_math_node *n, float x, float baseline_y, float size, uint32_t colour, int seed);

static void draw_row(uint8_t *buf, wb_math_node *n, float x, float baseline_y, float size, uint32_t colour, int seed)
{
	int last_symbol = 0;
	float cursor = x;
	
	for (int i = 0; i < n->n_children; i++)
	{
		wb_math_box cb = measure_node(n->children[i], size);
		
		if (i > 0)
			cursor += wb_symbol_spacing(last_symbol, cb.first_symbol, size);
		
		draw_node(buf, n->children[i], cursor, baseline_y, size, colour, seed + i * 359);
		cursor += cb.w;
		last_symbol = cb.last_symbol;
	}
}

static void draw_frac(uint8_t *buf, wb_math_node *n, float x, float baseline_y, float size, uint32_t colour, int seed)
{
	wb_math_box b = measure_node(n, size);
	wb_math_box top = measure_node(n->a, size * WB_MATH_FRAC_SCALE);
	wb_math_box bot = measure_node(n->b, size * WB_MATH_FRAC_SCALE);
	float top_x = x + (b.w - top.w) * 0.5f;
	float bot_x = x + (b.w - bot.w) * 0.5f;
	float line_y = baseline_y - size * 0.22f;
	float j0 = ((float)((seed * 1103515245U + 12345U) & 0xff) / 255.0f - 0.5f) * wb_marker_thickness() * 0.80f * wb_math_jitter_strength;
	float j1 = ((float)(((seed + 31) * 1103515245U + 12345U) & 0xff) / 255.0f - 0.5f) * wb_marker_thickness() * 0.80f * wb_math_jitter_strength;
	float jt_x = ((float)(((seed + 97) * 1103515245U + 12345U) & 0xff) / 255.0f - 0.5f) * size * 0.012f * wb_math_jitter_strength;
	float jt_y = ((float)(((seed + 193) * 1103515245U + 12345U) & 0xff) / 255.0f - 0.5f) * size * 0.012f * wb_math_jitter_strength;
	float hook = wb_marker_thickness() * 1.3f;
	
	draw_node(buf, n->a, top_x, line_y - top.descent - size * 0.18f, size * WB_MATH_FRAC_SCALE, colour, seed + 11);
	draw_sausage(buf, vec2(x + jt_x + size * 0.08f - hook, line_y + jt_y + j0 - hook * 0.25f), vec2(x + jt_x + size * 0.08f, line_y + jt_y + j0), wb_marker_thickness(), colour);
	draw_sausage(buf, vec2(x + jt_x + size * 0.08f, line_y + jt_y + j0), vec2(x + jt_x + b.w * 0.5f, line_y + jt_y - wb_marker_thickness() * 0.45f), wb_marker_thickness(), colour);
	draw_sausage(buf, vec2(x + jt_x + b.w * 0.5f, line_y + jt_y - wb_marker_thickness() * 0.45f), vec2(x + jt_x + b.w - size * 0.08f, line_y + jt_y + j1), wb_marker_thickness(), colour);
	draw_node(buf, n->b, bot_x, line_y + bot.ascent + size * 0.18f, size * WB_MATH_FRAC_SCALE, colour, seed + 23);
}

static void draw_script(uint8_t *buf, wb_math_node *n, float x, float baseline_y, float size, uint32_t colour, int seed)
{
	wb_math_box base = measure_node(n->base, size);
	float script_size = size * WB_MATH_SCRIPT_SCALE;
	float script_x = x + base.w;
	
	draw_node(buf, n->base, x, baseline_y, size, colour, seed);
	
	if (n->sup)
		draw_node(buf, n->sup, script_x, baseline_y - base.ascent * 0.58f, script_size, colour, seed + 101);
	
	if (n->sub)
		draw_node(buf, n->sub, script_x, baseline_y + size * 0.22f, script_size, colour, seed + 211);
}

static void draw_sqrt(uint8_t *buf, wb_math_node *n, float x, float baseline_y, float size, uint32_t colour, int seed)
{
	wb_math_box inner = measure_node(n->a, size);
	float rx = x + size * 0.34f;
	float top_y = baseline_y - inner.ascent - size * 0.08f;
	
	draw_sausage(buf, vec2(x + size * 0.05f, baseline_y - size * 0.28f), vec2(x + size * 0.16f, baseline_y - size * 0.08f), wb_marker_thickness(), colour);
	draw_sausage(buf, vec2(x + size * 0.16f, baseline_y - size * 0.08f), vec2(x + size * 0.32f, top_y), wb_marker_thickness(), colour);
	draw_sausage(buf, vec2(x + size * 0.32f, top_y), vec2(rx + inner.w + size * 0.04f, top_y), wb_marker_thickness(), colour);
	draw_node(buf, n->a, rx, baseline_y, size, colour, seed + 17);
}

static void draw_node(uint8_t *buf, wb_math_node *n, float x, float baseline_y, float size, uint32_t colour, int seed)
{
	if (!n)
		return;
	
	switch (n->type)
	{
		case WB_MATH_SYMBOL:
			wb_draw_symbol(buf, n->symbol_id, x, baseline_y, size, colour, seed);
			break;
		case WB_MATH_ROW:
			draw_row(buf, n, x, baseline_y, size, colour, seed);
			break;
		case WB_MATH_FRAC:
			draw_frac(buf, n, x, baseline_y, size, colour, seed);
			break;
		case WB_MATH_SCRIPT:
			draw_script(buf, n, x, baseline_y, size, colour, seed);
			break;
		case WB_MATH_SQRT:
			draw_sqrt(buf, n, x, baseline_y, size, colour, seed);
			break;
	}
}

wb_math_formula *wb_math_parse(const char *src)
{
	if (!src)
		return NULL;
	
	wb_math_formula *formula = malloc(sizeof(wb_math_formula));
	
	if (!formula)
		return NULL;
	
	wb_math_parser p;
	p.s = src;
	p.math_mode = strchr(src, '$') ? 0 : 1;
	p.error[0] = 0;
	formula->root = parse_row(&p, 0);
	
	if (strchr(src, '$') && p.math_mode)
		set_error(&p, "missing closing dollar");
	
	if (p.error[0] || !formula->root)
	{
		if (p.error[0])
			printf("math parse error: %s\n", p.error);
		
		wb_math_free(formula);
		return NULL;
	}
	
	return formula;
}

void wb_math_free(wb_math_formula *formula)
{
	if (!formula)
		return;
	
	free_node(formula->root);
	free(formula);
}

void wb_math_measure(wb_math_formula *formula, float size, float *w, float *h, float *baseline)
{
	wb_math_box b = empty_box();
	
	if (formula)
		b = measure_node(formula->root, size);
	
	if (w)
		*w = b.w;
	
	if (h)
		*h = b.ascent + b.descent;
	
	if (baseline)
		*baseline = b.ascent;
}

void wb_math_draw(uint8_t *buf, wb_math_formula *formula, float x, float baseline_y, float size, uint32_t colour)
{
	wb_math_draw_seeded(buf, formula, x, baseline_y, size, colour, 0x4d80e4);
}

void wb_set_math_jitter_strength(float strength)
{
	if (strength < 0.0f)
		strength = 0.0f;
	wb_math_jitter_strength = strength;
	wb_set_symbol_jitter_strength(strength);
}

void wb_math_draw_seeded(uint8_t *buf, wb_math_formula *formula, float x, float baseline_y, float size, uint32_t colour, int seed)
{
	if (!buf || !formula)
		return;
	
	draw_node(buf, formula->root, x, baseline_y, size, colour, seed);
}
