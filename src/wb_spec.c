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
	wb_scene **scenes;
	float *durations;
	int n_scenes;
	int cap_scenes;
	wb_spec_name names[256];
	wb_spec_name layers[64];
	int n_names;
	int n_layers;
	float duration;
	int in_block;
	int saw_block;
	char output_path[256];
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

static int is_safe_output_path(const char *s)
{
	if (!s || !*s)
		return 0;
	
	for (int i = 0; s[i]; i++)
	{
		char c = s[i];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
			c == '_' || c == '-' || c == '.' || c == '/')
			continue;
		return 0;
	}
	
	return 1;
}

static int set_error(wb_spec_parser *p, int line_no, const char *msg)
{
	if (p->error[0] == 0)
		snprintf(p->error, sizeof(p->error), "line %d: %s", line_no, msg);
	return 0;
}

static int append_parser_scene(wb_spec_parser *p, wb_scene *scene, float duration)
{
	if (!p || !scene)
		return 0;
	
	if (p->n_scenes >= p->cap_scenes)
	{
		int new_cap = p->cap_scenes ? p->cap_scenes * 2 : 4;
		wb_scene **scenes = malloc(sizeof(wb_scene*) * new_cap);
		float *durations = malloc(sizeof(float) * new_cap);
		
		if (!scenes || !durations)
		{
			free(scenes);
			free(durations);
			return 0;
		}
		
		for (int i = 0; i < p->n_scenes; i++)
		{
			scenes[i] = p->scenes[i];
			durations[i] = p->durations[i];
		}
		
		free(p->scenes);
		free(p->durations);
		p->scenes = scenes;
		p->durations = durations;
		p->cap_scenes = new_cap;
	}
	
	p->scenes[p->n_scenes] = scene;
	p->durations[p->n_scenes] = duration;
	p->n_scenes++;
	return 1;
}

static int start_new_scene(wb_spec_parser *p, float duration)
{
	wb_scene *scene;
	
	if (!p)
		return 0;
	
	scene = new_scene();
	if (!scene)
		return 0;
	
	if (duration > 0.0f)
		scene->total_duration = binary_max(scene->total_duration, duration);
	
	if (!append_parser_scene(p, scene, duration > 0.0f ? duration : (float)FRAMES_PER_SCENE / FPS))
	{
		free_scene(scene);
		return 0;
	}
	
	p->scene = scene;
	p->duration = duration > 0.0f ? duration : (float)FRAMES_PER_SCENE / FPS;
	p->n_names = 0;
	p->n_layers = 0;
	return 1;
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

static int parse_jitter_token(char *line, float *strength)
{
	char value[32];
	char *jitter = strstr(line, " jitter ");
	
	if (!jitter || !strength)
		return 0;
	
	if (sscanf(jitter, " jitter %31s", value) != 1)
		return 0;
	
	if (strcmp(value, "off") == 0 || strcmp(value, "false") == 0 || strcmp(value, "none") == 0)
	{
		*strength = 0.0f;
		return 1;
	}
	if (strcmp(value, "on") == 0 || strcmp(value, "true") == 0)
	{
		*strength = 1.0f;
		return 1;
	}
	
	return sscanf(value, "%f", strength) == 1;
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
		if (!start_new_scene(p, duration))
			return set_error(p, line_no, "failed to create scene");
		return 1;
	}
	return set_error(p, line_no, "expected scene \"title\" duration Ns");
}

static int parse_video(wb_spec_parser *p, char *line, int line_no)
{
	char output_path[256];
	
	if (sscanf(line, "video%*[^\"]\"%255[^\"]\"", output_path) == 1)
	{
		if (!is_safe_output_path(output_path))
			return set_error(p, line_no, "video output path may only contain letters, digits, '/', '.', '_' and '-'");
		snprintf(p->output_path, sizeof(p->output_path), "%s", output_path);
		return 1;
	}
	
	return 1;
}

static int parse_math(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char latex[512];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, size = 70.0f;
	float jitter_strength = 1.0f;
	int n = 0;
	
	if (sscanf(line, "math %63s \"%511[^\"]\" at (%f,%f) size %f colour %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 5 ||
		sscanf(line, "math %63s \"%511[^\"]\" at (%f, %f) size %f colour %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 5)
	{
		int id = wb_scene_add_math(p->scene, latex, x, y, size, parse_colour(colour_name));
		if (!id)
			return set_error(p, line_no, "failed to create math object");
		if (parse_jitter_token(line, &jitter_strength))
			wb_scene_set_object_jitter(p->scene, id, jitter_strength);
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
	float jitter_strength = 1.0f;
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
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_layer_jitter(p->scene, id, jitter_strength);
	
	remember_layer(p, name, id);
	return 1;
}

static int parse_camera(wb_spec_parser *p, char *line, int line_no)
{
	float distance = 5.0f;
	float scale = 260.0f;
	float cx = WIDTH * 0.5f;
	float cy = HEIGHT * 0.5f;
	int matched = 0;
	
	matched = sscanf(line, "camera distance %f scale %f center (%f,%f)", &distance, &scale, &cx, &cy);
	if (matched < 4)
		matched = sscanf(line, "camera distance %f scale %f center (%f, %f)", &distance, &scale, &cx, &cy);
	if (matched == 4)
	{
		wb_scene_set_layer_camera(p->scene, p->scene->current_layer_id, distance, scale, cx, cy);
		return 1;
	}
	
	return set_error(p, line_no, "expected camera distance D scale S center (x,y)");
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

static int parse_draw(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line, "draw %63s during %fs..%fs", name, &t0, &t1) == 3)
	{
		int id = find_name(p, name);
		if (!id)
			return set_error(p, line_no, "draw references unknown object");
		wb_scene_draw_in(p->scene, id, t0, t1);
		return 1;
	}
	
	return set_error(p, line_no, "expected draw name during Ts..Ts");
}

static int parse_line_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = 3.0f;
	float jitter_strength = 1.0f;
	int matched = 0;
	
	matched = sscanf(line, "line %63s from (%f,%f) to (%f,%f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "line %63s from (%f, %f) to (%f, %f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		return set_error(p, line_no, "expected line name from (x,y) to (x,y) thickness N colour name");
	
	int id = wb_scene_add_line(p->scene, x0, y0, x1, y1, thickness, parse_colour(matched >= 7 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create line object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_dotted_line_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = 3.0f, gap = 18.0f;
	float jitter_strength = 1.0f;
	int matched = 0;
	
	matched = sscanf(line, "dotted_line %63s from (%f,%f) to (%f,%f) thickness %f gap %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		matched = sscanf(line, "dotted_line %63s from (%f, %f) to (%f, %f) thickness %f gap %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		return set_error(p, line_no, "expected dotted_line name from (x,y) to (x,y) thickness N gap N colour name");
	
	int id = wb_scene_add_dotted_line(p->scene, x0, y0, x1, y1, matched >= 6 ? thickness : 3.0f, matched >= 7 ? gap : 18.0f, parse_colour(matched >= 8 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create dotted_line object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_arrow_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = 3.0f, head_size = 24.0f;
	float jitter_strength = 1.0f;
	int matched = 0;
	
	matched = sscanf(line, "arrow %63s from (%f,%f) to (%f,%f) thickness %f head %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &head_size, colour_name);
	if (matched < 5)
		matched = sscanf(line, "arrow %63s from (%f, %f) to (%f, %f) thickness %f head %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &head_size, colour_name);
	if (matched < 5)
		return set_error(p, line_no, "expected arrow name from (x,y) to (x,y) thickness N head N colour name");
	
	int id = wb_scene_add_arrow(p->scene, x0, y0, x1, y1, matched >= 6 ? thickness : 3.0f, matched >= 7 ? head_size : 24.0f, parse_colour(matched >= 8 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create arrow object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_line3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, thickness = 3.0f;
	float jitter_strength = 1.0f;
	int matched = 0;
	
	matched = sscanf(line, "line3d %63s from (%f,%f,%f) to (%f,%f,%f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "line3d %63s from (%f, %f, %f) to (%f, %f, %f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &thickness, colour_name);
	if (matched < 7)
		return set_error(p, line_no, "expected line3d name from (x,y,z) to (x,y,z) thickness N colour name");
	
	int id = wb_scene_add_line3d(p->scene, x0, y0, z0, x1, y1, z1, matched >= 8 ? thickness : 3.0f, parse_colour(matched >= 9 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create line3d object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_curve3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, x2 = 0.0f, y2 = 0.0f, z2 = 0.0f, thickness = 3.0f;
	float jitter_strength = 1.0f;
	int matched = 0;
	
	matched = sscanf(line, "curve3d %63s through (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10)
		matched = sscanf(line, "curve3d %63s through (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10)
		return set_error(p, line_no, "expected curve3d name through (x,y,z) (x,y,z) (x,y,z) thickness N colour name");
	
	int id = wb_scene_add_curve3d(p->scene, x0, y0, z0, x1, y1, z1, x2, y2, z2, matched >= 11 ? thickness : 3.0f, parse_colour(matched >= 12 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create curve3d object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_point_object(wb_spec_parser *p, char *line, int line_no, int open)
{
	char name[64];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, radius = 6.0f, thickness = 2.5f;
	float jitter_strength = 1.0f;
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
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_circle_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, radius = 40.0f, thickness = 3.0f;
	float jitter_strength = 1.0f;
	int matched = 0;
	
	matched = sscanf(line, "circle %63s center (%f,%f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4)
		matched = sscanf(line, "circle %63s center (%f, %f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4)
		return set_error(p, line_no, "expected circle name center (x,y) radius N thickness N colour name");
	
	int id = wb_scene_add_circle(p->scene, x, y, radius, matched >= 5 ? thickness : 3.0f, parse_colour(matched >= 6 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create circle object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_disc_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, radius = 40.0f, opacity = 0.25f;
	float jitter_strength = 1.0f;
	int matched = 0;
	
	matched = sscanf(line, "shade_disc %63s center (%f,%f) radius %f colour %63s opacity %f", name, &x, &y, &radius, colour_name, &opacity);
	if (matched < 4)
		matched = sscanf(line, "shade_disc %63s center (%f, %f) radius %f colour %63s opacity %f", name, &x, &y, &radius, colour_name, &opacity);
	if (matched < 4)
		return set_error(p, line_no, "expected shade_disc name center (x,y) radius N colour name opacity A");
	
	if (opacity < 0.0f)
		opacity = 0.0f;
	if (opacity > 1.0f)
		opacity = 1.0f;
	
	int id = wb_scene_add_shade_disc(p->scene, x, y, radius, parse_colour(matched >= 5 ? colour_name : "blue"), matched >= 6 ? opacity : 0.25f);
	if (!id)
		return set_error(p, line_no, "failed to create shade_disc object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
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
		return parse_video(p, s, line_no);
	if (starts_with(s, "scene "))
		return parse_scene(p, s, line_no);
	if (!p->scene && !start_new_scene(p, (float)FRAMES_PER_SCENE / FPS))
		return set_error(p, line_no, "failed to create default scene");
	if (starts_with(s, "layer "))
		return parse_layer(p, s, line_no);
	if (starts_with(s, "camera "))
		return parse_camera(p, s, line_no);
	if (starts_with(s, "background "))
		return parse_background(p, s, line_no);
	if (starts_with(s, "math "))
		return parse_math(p, s, line_no);
	if (starts_with(s, "line3d "))
		return parse_line3d_object(p, s, line_no);
	if (starts_with(s, "curve3d "))
		return parse_curve3d_object(p, s, line_no);
	if (starts_with(s, "dotted_line "))
		return parse_dotted_line_object(p, s, line_no);
	if (starts_with(s, "arrow "))
		return parse_arrow_object(p, s, line_no);
	if (starts_with(s, "line "))
		return parse_line_object(p, s, line_no);
	if (starts_with(s, "circle "))
		return parse_circle_object(p, s, line_no);
	if (starts_with(s, "shade_disc "))
		return parse_shade_disc_object(p, s, line_no);
	if (starts_with(s, "point "))
		return parse_point_object(p, s, line_no, 0);
	if (starts_with(s, "open_point "))
		return parse_point_object(p, s, line_no, 1);
	if (starts_with(s, "move_layer "))
		return parse_move_layer(p, s, line_no);
	if (starts_with(s, "move "))
		return parse_move(p, s, line_no);
	if (starts_with(s, "draw "))
		return parse_draw(p, s, line_no);
	
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
	
	parser.duration = (float)FRAMES_PER_SCENE / FPS;
	
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
		for (int i = 0; i < parser.n_scenes; i++)
			free_scene(parser.scenes[i]);
		free(parser.scenes);
		free(parser.durations);
		return result;
	}
	
	if (parser.n_scenes == 0 && !start_new_scene(&parser, (float)FRAMES_PER_SCENE / FPS))
	{
		snprintf(result.error, sizeof(result.error), "could not allocate scene");
		return result;
	}
	
	result.scene = parser.scenes[0];
	result.scenes = parser.scenes;
	result.durations = parser.durations;
	result.n_scenes = parser.n_scenes;
	snprintf(result.output_path, sizeof(result.output_path), "%s", parser.output_path);
	for (int i = 0; i < result.n_scenes; i++)
		result.durations[i] = binary_max(result.durations[i], result.scenes[i]->total_duration);
	result.duration = result.durations[0];
	return result;
}

void wb_free_loaded_video(wb_loaded_video *video)
{
	if (!video)
		return;
	
	if (video->scenes)
	{
		for (int i = 0; i < video->n_scenes; i++)
			free_scene(video->scenes[i]);
		free(video->scenes);
		free(video->durations);
	}
	else if (video->scene)
		free_scene(video->scene);
	
	memset(video, 0, sizeof(*video));
}
