#include "whiteboard.h"
#include "wb_spec.h"

typedef struct
{
	char name[64];
	int id;
} wb_spec_name;

typedef struct
{
	wb_scene *scene;
	wb_spec_name names[256];
	wb_spec_name layers[64];
	int n_names;
	int n_layers;
	float duration;
	int in_block;
	int saw_block;
	char error[256];
} wb_spec_parser;

static char *trim_left(char *s)
{
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
		s++;
	return s;
}

static void trim_right(char *s)
{
	int n = (int)strlen(s);
	while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n'))
		s[--n] = 0;
}

static int starts_with(const char *s, const char *prefix)
{
	return strncmp(s, prefix, strlen(prefix)) == 0;
}

static int set_error(wb_spec_parser *p, int line_no, const char *msg)
{
	if (p->error[0] == 0)
		snprintf(p->error, sizeof(p->error), "line %d: %s", line_no, msg);
	return 0;
}

static uint32_t parse_colour(const char *s)
{
	if (!s || strcmp(s, "blue") == 0)
		return NICE_BLUE;
	if (strcmp(s, "black") == 0)
		return 0x202124;
	if (strcmp(s, "red") == 0)
		return 0xe03131;
	if (strcmp(s, "green") == 0)
		return 0x2fb344;
	if (strcmp(s, "grey") == 0 || strcmp(s, "gray") == 0)
		return 0x6c757d;
	if (s[0] == '#')
	{
		unsigned int r = 0, g = 0, b = 0;
		if (sscanf(s + 1, "%02x%02x%02x", &r, &g, &b) == 3)
			return COLOUR(r, g, b);
	}
	return NICE_BLUE;
}

static void remember_name(wb_spec_parser *p, const char *name, int id)
{
	if (!name || !*name || id <= 0 || p->n_names >= (int)(sizeof(p->names) / sizeof(p->names[0])))
		return;
	snprintf(p->names[p->n_names].name, sizeof(p->names[p->n_names].name), "%s", name);
	p->names[p->n_names].id = id;
	p->n_names++;
}

static int find_name(wb_spec_parser *p, const char *name)
{
	for (int i = 0; i < p->n_names; i++)
	{
		if (strcmp(p->names[i].name, name) == 0)
			return p->names[i].id;
	}
	return 0;
}

static void remember_layer(wb_spec_parser *p, const char *name, int id)
{
	if (!name || !*name || id <= 0 || p->n_layers >= (int)(sizeof(p->layers) / sizeof(p->layers[0])))
		return;
	snprintf(p->layers[p->n_layers].name, sizeof(p->layers[p->n_layers].name), "%s", name);
	p->layers[p->n_layers].id = id;
	p->n_layers++;
}

static int find_layer_name(wb_spec_parser *p, const char *name)
{
	for (int i = 0; i < p->n_layers; i++)
	{
		if (strcmp(p->layers[i].name, name) == 0)
			return p->layers[i].id;
	}
	return 0;
}

static int parse_scene(wb_spec_parser *p, char *line, int line_no)
{
	char title[128];
	float duration = 0.0f;
	(void)title;
	
	if (sscanf(line, "scene \"%127[^\"]\" duration %fs", title, &duration) == 2 ||
		sscanf(line, "scene duration %fs", &duration) == 1)
	{
		if (duration > 0.0f)
		{
			p->duration = duration;
			p->scene->total_duration = binary_max(p->scene->total_duration, duration);
		}
		return 1;
	}
	return set_error(p, line_no, "expected scene \"title\" duration Ns");
}

static int parse_math(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char latex[512];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, size = 70.0f;
	int n = 0;
	
	if (sscanf(line, "math %63s \"%511[^\"]\" at (%f,%f) size %f colour %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 5 ||
		sscanf(line, "math %63s \"%511[^\"]\" at (%f, %f) size %f colour %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 5)
	{
		int id = wb_scene_add_math(p->scene, latex, x, y, size, parse_colour(colour_name));
		if (!id)
			return set_error(p, line_no, "failed to create math object");
		remember_name(p, name, id);
		return 1;
	}
	
	return set_error(p, line_no, "expected math name \"$...$\" at (x,y) size N colour name");
}

static int parse_background(wb_spec_parser *p, char *line, int line_no)
{
	char center[64];
	char edge[64];
	
	if (sscanf(line, "background radial center %63s edge %63s", center, edge) == 2)
	{
		wb_scene_set_radial_background(p->scene, parse_colour(center), parse_colour(edge));
		return 1;
	}
	
	return set_error(p, line_no, "expected background radial center colour edge colour");
}

static int parse_layer(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char type_name[32] = "2d";
	char opacity_word[32];
	float opacity = 1.0f;
	float blur_radius = 0.0f;
	int matched = 0;
	int type = WB_LAYER_2D;
	int id = 0;
	
	matched = sscanf(line, "layer %63s %31s opacity %f", name, type_name, &opacity);
	if (matched < 2)
		matched = sscanf(line, "layer %63s %31s", name, type_name);
	if (matched < 1)
		return set_error(p, line_no, "expected layer name 2d|3d [opacity N]");
	
	if (strcmp(type_name, "3d") == 0)
		type = WB_LAYER_3D;
	else if (strcmp(type_name, "2d") == 0)
		type = WB_LAYER_2D;
	else if (sscanf(line, "layer %63s opacity %f", name, &opacity) == 2)
		type = WB_LAYER_2D;
	else if (sscanf(line, "layer %63s %31s", name, opacity_word) == 2 && strcmp(opacity_word, "opacity") == 0)
		return set_error(p, line_no, "expected opacity value after layer opacity");
	else
		return set_error(p, line_no, "layer type must be 2d or 3d");
	
	if (opacity < 0.0f)
		opacity = 0.0f;
	if (opacity > 1.0f)
		opacity = 1.0f;
	
	id = wb_scene_add_layer(p->scene, name, type, opacity);
	if (!id)
		return set_error(p, line_no, "failed to create layer");
	
	char *blur = strstr(line, " blur ");
	if (blur && sscanf(blur, " blur %f", &blur_radius) == 1)
		wb_scene_set_layer_blur(p->scene, id, blur_radius);
	
	remember_layer(p, name, id);
	return 1;
}

static int parse_move(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line, "move %63s from (%f,%f) to (%f,%f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move %63s from (%f, %f) to (%f, %f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7)
	{
		int id = find_name(p, name);
		if (!id)
			return set_error(p, line_no, "move references unknown object");
		wb_scene_move(p->scene, id, t0, t1, x1, y1, x2, y2);
		return 1;
	}
	
	return set_error(p, line_no, "expected move name from (x,y) to (x,y) during Ts..Ts");
}

static int parse_move_layer(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line, "move_layer %63s from (%f,%f) to (%f,%f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move_layer %63s from (%f, %f) to (%f, %f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7)
	{
		int id = find_layer_name(p, name);
		if (!id)
			return set_error(p, line_no, "move_layer references unknown layer");
		wb_scene_move_layer(p->scene, id, t0, t1, x1, y1, x2, y2);
		return 1;
	}
	
	return set_error(p, line_no, "expected move_layer name from (x,y) to (x,y) during Ts..Ts");
}

static int parse_line_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = 3.0f;
	int matched = 0;
	
	matched = sscanf(line, "line %63s from (%f,%f) to (%f,%f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "line %63s from (%f, %f) to (%f, %f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		return set_error(p, line_no, "expected line name from (x,y) to (x,y) thickness N colour name");
	
	int id = wb_scene_add_line(p->scene, x0, y0, x1, y1, thickness, parse_colour(matched >= 7 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create line object");
	remember_name(p, name, id);
	return 1;
}

static int parse_point_object(wb_spec_parser *p, char *line, int line_no, int open)
{
	char name[64];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, radius = 6.0f, thickness = 2.5f;
	int matched = 0;
	int id = 0;
	
	if (open)
	{
		matched = sscanf(line, "open_point %63s at (%f,%f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3)
			matched = sscanf(line, "open_point %63s at (%f, %f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3)
			return set_error(p, line_no, "expected open_point name at (x,y) radius N thickness N colour name");
		id = wb_scene_add_open_point(p->scene, x, y, radius, matched >= 5 ? thickness : 2.5f, parse_colour(matched >= 6 ? colour_name : "blue"));
	}
	else
	{
		matched = sscanf(line, "point %63s at (%f,%f) radius %f colour %63s", name, &x, &y, &radius, colour_name);
		if (matched < 3)
			matched = sscanf(line, "point %63s at (%f, %f) radius %f colour %63s", name, &x, &y, &radius, colour_name);
		if (matched < 3)
			return set_error(p, line_no, "expected point name at (x,y) radius N colour name");
		id = wb_scene_add_point(p->scene, x, y, matched >= 4 ? radius : 6.0f, parse_colour(matched >= 5 ? colour_name : "blue"));
	}
	
	if (!id)
		return set_error(p, line_no, open ? "failed to create open_point object" : "failed to create point object");
	remember_name(p, name, id);
	return 1;
}

static int parse_spec_line(wb_spec_parser *p, char *line, int line_no)
{
	char *s = trim_left(line);
	trim_right(s);
	
	if (*s == 0 || *s == '#')
		return 1;
	
	if (starts_with(s, "```"))
	{
		if (!p->in_block && (strcmp(s, "```whiteboard") == 0 || strcmp(s, "```wb") == 0 || strcmp(s, "```Whiteboard") == 0))
		{
			p->in_block = 1;
			p->saw_block = 1;
		}
		else if (p->in_block && strcmp(s, "```") == 0)
			p->in_block = 0;
		return 1;
	}
	
	if (p->saw_block && !p->in_block)
		return 1;
	
	if (starts_with(s, "video "))
		return 1;
	if (starts_with(s, "scene "))
		return parse_scene(p, s, line_no);
	if (starts_with(s, "layer "))
		return parse_layer(p, s, line_no);
	if (starts_with(s, "background "))
		return parse_background(p, s, line_no);
	if (starts_with(s, "math "))
		return parse_math(p, s, line_no);
	if (starts_with(s, "line "))
		return parse_line_object(p, s, line_no);
	if (starts_with(s, "point "))
		return parse_point_object(p, s, line_no, 0);
	if (starts_with(s, "open_point "))
		return parse_point_object(p, s, line_no, 1);
	if (starts_with(s, "move_layer "))
		return parse_move_layer(p, s, line_no);
	if (starts_with(s, "move "))
		return parse_move(p, s, line_no);
	
	return set_error(p, line_no, "unknown Whiteboard spec command");
}

wb_loaded_video wb_load_video_spec(const char *path)
{
	wb_loaded_video result;
	wb_spec_parser parser;
	char line[2048];
	int line_no = 0;
	FILE *f = fopen(path, "r");
	
	memset(&result, 0, sizeof(result));
	memset(&parser, 0, sizeof(parser));
	
	if (!f)
	{
		snprintf(result.error, sizeof(result.error), "could not open %s", path ? path : "(null)");
		return result;
	}
	
	parser.scene = new_scene();
	parser.duration = (float)FRAMES_PER_SCENE / FPS;
	
	if (!parser.scene)
	{
		fclose(f);
		snprintf(result.error, sizeof(result.error), "could not allocate scene");
		return result;
	}
	
	while (fgets(line, sizeof(line), f))
	{
		line_no++;
		if (!parse_spec_line(&parser, line, line_no))
			break;
	}
	
	fclose(f);
	
	if (parser.error[0])
	{
		snprintf(result.error, sizeof(result.error), "%s", parser.error);
		free_scene(parser.scene);
		return result;
	}
	
	result.scene = parser.scene;
	result.duration = binary_max(parser.duration, parser.scene->total_duration);
	return result;
}
