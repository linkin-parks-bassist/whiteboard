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
	wb_scene_transition *transitions;
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

static void expand_relative_coords(char *line, size_t cap)
{
	char out[2048];
	size_t i = 0;
	size_t j = 0;
	int in_quotes = 0;
	
	if (!line || cap == 0)
		return;
	
	while (line[i] && j + 1 < sizeof(out))
	{
		if (line[i] == '"')
		{
			out[j++] = line[i++];
			in_quotes = !in_quotes;
			continue;
		}
		
		if (!in_quotes && line[i] == '[')
		{
			float rx = 0.0f;
			float ry = 0.0f;
			int consumed = 0;
			int matched = sscanf(line + i, "[%f,%f]%n", &rx, &ry, &consumed);
			
			if (matched != 2)
				matched = sscanf(line + i, "[%f, %f]%n", &rx, &ry, &consumed);
			if (matched == 2 && consumed > 0)
			{
				int written = snprintf(out + j, sizeof(out) - j, "(%.3f,%.3f)", rx * WIDTH, ry * HEIGHT);
				
				if (written <= 0 || (size_t)written >= sizeof(out) - j)
					break;
				j += (size_t)written;
				i += (size_t)consumed;
				continue;
			}
		}
		
		out[j++] = line[i++];
	}
	
	out[j] = 0;
	snprintf(line, cap, "%s", out);
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
		wb_scene_transition *transitions = malloc(sizeof(wb_scene_transition) * new_cap);
		
		if (!scenes || !durations || !transitions)
		{
			free(scenes);
			free(durations);
			free(transitions);
			return 0;
		}
		
		for (int i = 0; i < p->n_scenes; i++)
		{
			scenes[i] = p->scenes[i];
			durations[i] = p->durations[i];
			transitions[i] = p->transitions[i];
		}
		
		free(p->scenes);
		free(p->durations);
		free(p->transitions);
		p->scenes = scenes;
		p->durations = durations;
		p->transitions = transitions;
		p->cap_scenes = new_cap;
	}
	
	p->scenes[p->n_scenes] = scene;
	p->durations[p->n_scenes] = duration;
	p->transitions[p->n_scenes].type = WB_TRANSITION_NONE;
	p->transitions[p->n_scenes].duration = 0.0f;
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
	if (strcmp(s, "purple") == 0 || strcmp(s, "violet") == 0)
		return 0x7c4dff;
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
		*strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
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

static int parse_transition(wb_spec_parser *p, char *line, int line_no)
{
	char type_name[32];
	float duration = 0.0f;
	int type = WB_TRANSITION_NONE;
	
	if (!p->scene || p->n_scenes <= 0)
		return set_error(p, line_no, "transition must appear after a scene");
	
	if (sscanf(line, "transition %31s %fs", type_name, &duration) != 2)
		return set_error(p, line_no, "expected transition fade|crossfade Ns");
	
	if (strcmp(type_name, "fade") == 0)
		type = WB_TRANSITION_FADE;
	else if (strcmp(type_name, "crossfade") == 0)
		type = WB_TRANSITION_CROSSFADE;
	else
		return set_error(p, line_no, "transition type must be fade or crossfade");
	
	if (duration <= 0.0f)
		return set_error(p, line_no, "transition duration must be positive");
	
	p->transitions[p->n_scenes - 1].type = type;
	p->transitions[p->n_scenes - 1].duration = duration;
	return 1;
}

static int parse_math(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char latex[512];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, size = WB_DEFAULT_MATH_SIZE;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	float thickness = WB_DEFAULT_MATH_THICKNESS;
	int n = 0;
	
	if (sscanf(line, "math %63s \"%511[^\"]\" at (%f,%f) size %f colour %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 5 ||
		sscanf(line, "math %63s \"%511[^\"]\" at (%f, %f) size %f colour %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 5 ||
		sscanf(line, "math %63s \"%511[^\"]\" @ (%f,%f) s %f c %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 4 ||
		sscanf(line, "math %63s \"%511[^\"]\" @ (%f, %f) s %f c %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 4)
	{
		int id = wb_scene_add_math(p->scene, latex, x, y, size, parse_colour(colour_name));
		if (!id)
			return set_error(p, line_no, "failed to create math object");
		if (sscanf(line + n, " thickness %f", &thickness) == 1)
			p->scene->objects[p->scene->n_objects - 1].thickness = thickness;
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
	float opacity = WB_DEFAULT_LAYER_OPACITY;
	float blur_radius = WB_DEFAULT_LAYER_BLUR_RADIUS;
	float jitter_strength = WB_DEFAULT_LAYER_JITTER_STRENGTH;
	int matched = 0;
	int type = WB_LAYER_2D;
	int id = 0;
	
	matched = sscanf(line, "layer %63s %31s opacity %f", name, type_name, &opacity);
	if (matched < 2)
		matched = sscanf(line, "layer %63s %31s", name, type_name);
	if (matched < 2)
		matched = sscanf(line, "layer %63s %31s o %f", name, type_name, &opacity);
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
	
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
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
	float distance = WB_DEFAULT_LAYER_CAMERA_DISTANCE;
	float scale = WB_DEFAULT_LAYER_CAMERA_SCALE;
	float yaw = WB_DEFAULT_LAYER_CAMERA_YAW;
	float cx = WB_DEFAULT_LAYER_CAMERA_CENTER_X;
	float cy = WB_DEFAULT_LAYER_CAMERA_CENTER_Y;
	int matched = 0;
	
	matched = sscanf(line, "camera distance %f scale %f yaw %f center (%f,%f)", &distance, &scale, &yaw, &cx, &cy);
	if (matched < 5)
		matched = sscanf(line, "camera distance %f scale %f yaw %f center (%f, %f)", &distance, &scale, &yaw, &cx, &cy);
	if (matched < 4)
		matched = sscanf(line, "cam d %f s %f y %f @ (%f,%f)", &distance, &scale, &yaw, &cx, &cy);
	if (matched < 4)
		matched = sscanf(line, "cam d %f s %f y %f @ (%f, %f)", &distance, &scale, &yaw, &cx, &cy);
	if (matched < 4)
		matched = sscanf(line, "camera distance %f scale %f center (%f,%f)", &distance, &scale, &cx, &cy);
	if (matched < 4)
		matched = sscanf(line, "camera distance %f scale %f center (%f, %f)", &distance, &scale, &cx, &cy);
	if (matched < 2)
		matched = sscanf(line, "cam d %f s %f @ (%f,%f)", &distance, &scale, &cx, &cy);
	if (matched < 2)
		matched = sscanf(line, "cam d %f s %f @ (%f, %f)", &distance, &scale, &cx, &cy);
	if (matched == 5 || matched == 4)
	{
		wb_scene_set_layer_camera(p->scene, p->scene->current_layer_id, distance, scale, matched == 5 ? yaw : 0.0f, cx, cy);
		return 1;
	}
	
	return set_error(p, line_no, "expected camera distance D scale S [yaw A] center (x,y)");
}

static int parse_move(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line, "move %63s from (%f,%f) to (%f,%f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move %63s from (%f, %f) to (%f, %f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move %63s (%f,%f) -> (%f,%f) %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move %63s (%f, %f) -> (%f, %f) %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7)
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
		sscanf(line, "move_layer %63s from (%f, %f) to (%f, %f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move_layer %63s (%f,%f) -> (%f,%f) %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move_layer %63s (%f, %f) -> (%f, %f) %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7)
	{
		int id = find_layer_name(p, name);
		if (!id)
			return set_error(p, line_no, "move_layer references unknown layer");
		wb_scene_move_layer(p->scene, id, t0, t1, x1, y1, x2, y2);
		return 1;
	}
	
	return set_error(p, line_no, "expected move_layer name from (x,y) to (x,y) during Ts..Ts");
}

static int parse_move_camera(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float d1 = WB_DEFAULT_LAYER_CAMERA_DISTANCE, s1 = WB_DEFAULT_LAYER_CAMERA_SCALE, y1 = WB_DEFAULT_LAYER_CAMERA_YAW, cx1 = WB_DEFAULT_LAYER_CAMERA_CENTER_X, cy1 = WB_DEFAULT_LAYER_CAMERA_CENTER_Y;
	float d2 = WB_DEFAULT_LAYER_CAMERA_DISTANCE, s2 = WB_DEFAULT_LAYER_CAMERA_SCALE, y2 = WB_DEFAULT_LAYER_CAMERA_YAW, cx2 = WB_DEFAULT_LAYER_CAMERA_CENTER_X, cy2 = WB_DEFAULT_LAYER_CAMERA_CENTER_Y;
	float t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line,
		"move_camera %63s from distance %f scale %f yaw %f center (%f,%f) to distance %f scale %f yaw %f center (%f,%f) during %fs..%fs",
		name, &d1, &s1, &y1, &cx1, &cy1, &d2, &s2, &y2, &cx2, &cy2, &t0, &t1) == 13 ||
		sscanf(line,
		"move_camera %63s from distance %f scale %f yaw %f center (%f, %f) to distance %f scale %f yaw %f center (%f, %f) during %fs..%fs",
		name, &d1, &s1, &y1, &cx1, &cy1, &d2, &s2, &y2, &cx2, &cy2, &t0, &t1) == 13 ||
		sscanf(line,
		"move_camera %63s d %f s %f y %f @ (%f,%f) -> d %f s %f y %f @ (%f,%f) %fs..%fs",
		name, &d1, &s1, &y1, &cx1, &cy1, &d2, &s2, &y2, &cx2, &cy2, &t0, &t1) == 13 ||
		sscanf(line,
		"move_camera %63s d %f s %f y %f @ (%f, %f) -> d %f s %f y %f @ (%f, %f) %fs..%fs",
		name, &d1, &s1, &y1, &cx1, &cy1, &d2, &s2, &y2, &cx2, &cy2, &t0, &t1) == 13 ||
		sscanf(line,
		"move_camera %63s from distance %f scale %f center (%f,%f) to distance %f scale %f center (%f,%f) during %fs..%fs",
		name, &d1, &s1, &cx1, &cy1, &d2, &s2, &cx2, &cy2, &t0, &t1) == 11 ||
		sscanf(line,
		"move_camera %63s from distance %f scale %f center (%f, %f) to distance %f scale %f center (%f, %f) during %fs..%fs",
		name, &d1, &s1, &cx1, &cy1, &d2, &s2, &cx2, &cy2, &t0, &t1) == 11 ||
		sscanf(line,
		"move_camera %63s d %f s %f @ (%f,%f) -> d %f s %f @ (%f,%f) %fs..%fs",
		name, &d1, &s1, &cx1, &cy1, &d2, &s2, &cx2, &cy2, &t0, &t1) == 11 ||
		sscanf(line,
		"move_camera %63s d %f s %f @ (%f, %f) -> d %f s %f @ (%f, %f) %fs..%fs",
		name, &d1, &s1, &cx1, &cy1, &d2, &s2, &cx2, &cy2, &t0, &t1) == 11)
	{
		int id = find_layer_name(p, name);
		if (!id)
			return set_error(p, line_no, "move_camera references unknown layer");
		wb_scene_move_camera(p->scene, id, t0, t1, d1, s1, y1, cx1, cy1, d2, s2, y2, cx2, cy2);
		return 1;
	}
	
	return set_error(p, line_no, "expected move_camera layer from distance D scale S [yaw A] center (x,y) to distance D scale S [yaw A] center (x,y) during Ts..Ts");
}

static int parse_orbit_camera(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float y0 = WB_DEFAULT_LAYER_CAMERA_YAW, y1 = WB_DEFAULT_LAYER_CAMERA_YAW, t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line, "orbit_camera %63s from %f to %f during %fs..%fs", name, &y0, &y1, &t0, &t1) == 5 ||
		sscanf(line, "orbit_camera %63s %f -> %f %fs..%fs", name, &y0, &y1, &t0, &t1) == 5)
	{
		int id = find_layer_name(p, name);
		if (!id)
			return set_error(p, line_no, "orbit_camera references unknown layer");
		wb_scene_orbit_camera(p->scene, id, t0, t1, y0, y1);
		return 1;
	}
	
	return set_error(p, line_no, "expected orbit_camera layer from A to A during Ts..Ts");
}

static int parse_fade_layer(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float a0 = WB_DEFAULT_LAYER_OPACITY, a1 = WB_DEFAULT_LAYER_OPACITY, t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line, "fade_layer %63s from %f to %f during %fs..%fs", name, &a0, &a1, &t0, &t1) == 5 ||
		sscanf(line, "fade_layer %63s %f -> %f %fs..%fs", name, &a0, &a1, &t0, &t1) == 5)
	{
		int id = find_layer_name(p, name);
		if (!id)
			return set_error(p, line_no, "fade_layer references unknown layer");
		if (a0 < WB_MIN_OPACITY)
			a0 = WB_MIN_OPACITY;
		if (a0 > WB_MAX_OPACITY)
			a0 = WB_MAX_OPACITY;
		if (a1 < WB_MIN_OPACITY)
			a1 = WB_MIN_OPACITY;
		if (a1 > WB_MAX_OPACITY)
			a1 = WB_MAX_OPACITY;
		wb_scene_fade_layer(p->scene, id, t0, t1, a0, a1);
		return 1;
	}
	
	return set_error(p, line_no, "expected fade_layer layer from A to A during Ts..Ts");
}

static int parse_fade(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float a0 = WB_DEFAULT_LAYER_OPACITY, a1 = WB_DEFAULT_LAYER_OPACITY, t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line, "fade %63s from %f to %f during %fs..%fs", name, &a0, &a1, &t0, &t1) == 5 ||
		sscanf(line, "fade %63s %f -> %f %fs..%fs", name, &a0, &a1, &t0, &t1) == 5)
	{
		int id = find_name(p, name);
		if (!id)
			return set_error(p, line_no, "fade references unknown object");
		if (a0 < WB_MIN_OPACITY)
			a0 = WB_MIN_OPACITY;
		if (a0 > WB_MAX_OPACITY)
			a0 = WB_MAX_OPACITY;
		if (a1 < WB_MIN_OPACITY)
			a1 = WB_MIN_OPACITY;
		if (a1 > WB_MAX_OPACITY)
			a1 = WB_MAX_OPACITY;
		wb_scene_fade_object(p->scene, id, t0, t1, a0, a1);
		return 1;
	}
	
	return set_error(p, line_no, "expected fade name from A to A during Ts..Ts");
}

static int parse_draw(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line, "draw %63s during %fs..%fs", name, &t0, &t1) == 3 ||
		sscanf(line, "draw %63s %fs..%fs", name, &t0, &t1) == 3)
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
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "line %63s from (%f,%f) to (%f,%f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "line %63s from (%f, %f) to (%f, %f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "seg %63s (%f,%f) -> (%f,%f) t %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "seg %63s (%f, %f) -> (%f, %f) t %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		return set_error(p, line_no, "expected line/seg name from (x,y) to (x,y) thickness N colour name");
	
	int id = wb_scene_add_line(p->scene, x0, y0, x1, y1, thickness, parse_colour(matched >= 7 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create line object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_ray_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "ray %63s from (%f,%f) through (%f,%f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ray %63s from (%f, %f) through (%f, %f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ray %63s (%f,%f) -> (%f,%f) t %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ray %63s (%f, %f) -> (%f, %f) t %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		return set_error(p, line_no, "expected ray name from (x,y) through (x,y) thickness N colour name");
	
	int id = wb_scene_add_ray(p->scene, x0, y0, x1, y1, matched >= 6 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 7 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create ray object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_dotted_line_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS, gap = WB_DEFAULT_DOTTED_GAP;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
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

static int parse_dashed_line_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS, gap = WB_DEFAULT_DASHED_GAP;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "dashed_line %63s from (%f,%f) to (%f,%f) thickness %f gap %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		matched = sscanf(line, "dashed_line %63s from (%f, %f) to (%f, %f) thickness %f gap %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		matched = sscanf(line, "dash %63s (%f,%f) -> (%f,%f) t %f g %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		matched = sscanf(line, "dash %63s (%f, %f) -> (%f, %f) t %f g %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		return set_error(p, line_no, "expected dashed_line/dash name from (x,y) to (x,y) thickness N gap N colour name");
	
	int id = wb_scene_add_dashed_line(p->scene, x0, y0, x1, y1, matched >= 6 ? thickness : WB_DEFAULT_LINE_THICKNESS, matched >= 7 ? gap : WB_DEFAULT_DASHED_GAP, parse_colour(matched >= 8 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create dashed_line object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_arrow_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS, head_size = WB_DEFAULT_ARROW_HEAD_SIZE;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "arrow %63s from (%f,%f) to (%f,%f) thickness %f head %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &head_size, colour_name);
	if (matched < 5)
		matched = sscanf(line, "arrow %63s from (%f, %f) to (%f, %f) thickness %f head %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &head_size, colour_name);
	if (matched < 5)
		return set_error(p, line_no, "expected arrow name from (x,y) to (x,y) thickness N head N colour name");
	
	int id = wb_scene_add_arrow(p->scene, x0, y0, x1, y1, matched >= 6 ? thickness : WB_DEFAULT_LINE_THICKNESS, matched >= 7 ? head_size : WB_DEFAULT_ARROW_HEAD_SIZE, parse_colour(matched >= 8 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create arrow object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_triangle_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "triangle %63s points (%f,%f) (%f,%f) (%f,%f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "triangle %63s points (%f, %f) (%f, %f) (%f, %f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "tri %63s (%f,%f) (%f,%f) (%f,%f) t %f c %63s", name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "tri %63s (%f, %f) (%f, %f) (%f, %f) t %f c %63s", name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7)
		return set_error(p, line_no, "expected triangle/tri name points (x,y) (x,y) (x,y) thickness N colour name");
	
	int id = wb_scene_add_triangle(p->scene, x0, y0, x1, y1, x2, y2, matched >= 8 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 9 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create triangle object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_triangle_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, opacity = WB_DEFAULT_SHADE_OPACITY;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "shade_triangle %63s points (%f,%f) (%f,%f) (%f,%f) colour %63s opacity %f", name, &x0, &y0, &x1, &y1, &x2, &y2, colour_name, &opacity);
	if (matched < 7)
		matched = sscanf(line, "shade_triangle %63s points (%f, %f) (%f, %f) (%f, %f) colour %63s opacity %f", name, &x0, &y0, &x1, &y1, &x2, &y2, colour_name, &opacity);
	if (matched < 7)
		return set_error(p, line_no, "expected shade_triangle name points (x,y) (x,y) (x,y) colour name opacity A");
	
	if (opacity < 0.0f)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	int id = wb_scene_add_shade_triangle(p->scene, x0, y0, x1, y1, x2, y2, parse_colour(matched >= 8 ? colour_name : "blue"), matched >= 9 ? opacity : WB_DEFAULT_SHADE_OPACITY);
	if (!id)
		return set_error(p, line_no, "failed to create shade_triangle object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_quad_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, x3 = 0.0f, y3 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;

	matched = sscanf(line, "quad %63s points (%f,%f) (%f,%f) (%f,%f) (%f,%f) thickness %f colour %63s",
		name, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3, &thickness, colour_name);
	if (matched < 9)
		matched = sscanf(line, "quad %63s points (%f, %f) (%f, %f) (%f, %f) (%f, %f) thickness %f colour %63s",
			name, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3, &thickness, colour_name);
	if (matched < 9)
		return set_error(p, line_no, "expected quad name points (x,y) (x,y) (x,y) (x,y) thickness N colour name");

	int id = wb_scene_add_quad(p->scene, x0, y0, x1, y1, x2, y2, x3, y3, matched >= 10 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 11 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create quad object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_polygon_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	wb_vec2 points[7];
	float thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	char *cursor;
	char *tok;
	int n_points = 0;
	int id = 0;
	
	if (sscanf(line, "polygon %63s", name) != 1 && sscanf(line, "poly %63s", name) != 1)
		return set_error(p, line_no, "expected polygon/poly name ...");
	
	cursor = strstr(line, " points ");
	if (!cursor)
		cursor = strchr(line, ' ');
	if (!cursor)
		return set_error(p, line_no, "expected polygon/poly name points ...");
	if (starts_with(line, "polygon "))
		cursor = strstr(line, " points ");
	else
		cursor = strchr(cursor + 1, ' ');
	if (!cursor)
		return set_error(p, line_no, "expected polygon/poly name points ...");
	
	while (*cursor && n_points < 7)
	{
		float x = 0.0f, y = 0.0f;
		char *open = strchr(cursor, '(');
		if (!open)
			break;
		if (sscanf(open, "(%f,%f)", &x, &y) == 2 || sscanf(open, "(%f, %f)", &x, &y) == 2)
		{
			points[n_points++] = vec2(x, y);
			cursor = open + 1;
			continue;
		}
		break;
	}
	
	if (n_points < 3)
		return set_error(p, line_no, "expected polygon/poly with 3 to 7 points");
	
	tok = strstr(line, " thickness ");
	if (tok)
		sscanf(tok, " thickness %f", &thickness);
	else
	{
		tok = strstr(line, " t ");
		if (tok)
			sscanf(tok, " t %f", &thickness);
	}
	tok = strstr(line, " colour ");
	if (tok)
		sscanf(tok, " colour %63s", colour_name);
	else
	{
		tok = strstr(line, " c ");
		if (tok)
			sscanf(tok, " c %63s", colour_name);
	}
	
	id = wb_scene_add_polygon(p->scene, points, n_points, thickness, parse_colour(colour_name));
	if (!id)
		return set_error(p, line_no, "failed to create polygon object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_polygon_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	wb_vec2 points[7];
	float opacity = WB_DEFAULT_SHADE_OPACITY;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	char *cursor;
	char *tok;
	int n_points = 0;
	int id = 0;
	
	if (sscanf(line, "shade_polygon %63s", name) != 1 && sscanf(line, "shade_poly %63s", name) != 1)
		return set_error(p, line_no, "expected shade_polygon/shade_poly name ...");
	
	cursor = strstr(line, " points ");
	if (!cursor)
		cursor = strchr(line, ' ');
	if (!cursor)
		return set_error(p, line_no, "expected shade_polygon/shade_poly name points ...");
	if (starts_with(line, "shade_polygon "))
		cursor = strstr(line, " points ");
	else
		cursor = strchr(cursor + 1, ' ');
	if (!cursor)
		return set_error(p, line_no, "expected shade_polygon/shade_poly name points ...");
	
	while (*cursor && n_points < 7)
	{
		float x = 0.0f, y = 0.0f;
		char *open = strchr(cursor, '(');
		if (!open)
			break;
		if (sscanf(open, "(%f,%f)", &x, &y) == 2 || sscanf(open, "(%f, %f)", &x, &y) == 2)
		{
			points[n_points++] = vec2(x, y);
			cursor = open + 1;
			continue;
		}
		break;
	}
	
	if (n_points < 3)
		return set_error(p, line_no, "expected shade_polygon/shade_poly with 3 to 7 points");
	
	tok = strstr(line, " colour ");
	if (tok)
		sscanf(tok, " colour %63s", colour_name);
	else
	{
		tok = strstr(line, " c ");
		if (tok)
			sscanf(tok, " c %63s", colour_name);
	}
	tok = strstr(line, " opacity ");
	if (tok)
		sscanf(tok, " opacity %f", &opacity);
	else
	{
		tok = strstr(line, " a ");
		if (tok)
			sscanf(tok, " a %f", &opacity);
	}
	if (opacity < 0.0f)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	id = wb_scene_add_shade_polygon(p->scene, points, n_points, parse_colour(colour_name), opacity);
	if (!id)
		return set_error(p, line_no, "failed to create shade_polygon object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_line3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "line3d %63s from (%f,%f,%f) to (%f,%f,%f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "line3d %63s from (%f, %f, %f) to (%f, %f, %f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &thickness, colour_name);
	if (matched < 7)
		return set_error(p, line_no, "expected line3d name from (x,y,z) to (x,y,z) thickness N colour name");
	
	int id = wb_scene_add_line3d(p->scene, x0, y0, z0, x1, y1, z1, matched >= 8 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 9 ? colour_name : "blue"));
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
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, x2 = 0.0f, y2 = 0.0f, z2 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "curve3d %63s through (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10)
		matched = sscanf(line, "curve3d %63s through (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10)
		return set_error(p, line_no, "expected curve3d name through (x,y,z) (x,y,z) (x,y,z) thickness N colour name");
	
	int id = wb_scene_add_curve3d(p->scene, x0, y0, z0, x1, y1, z1, x2, y2, z2, matched >= 11 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 12 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create curve3d object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_point3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, z = 0.0f, radius = WB_DEFAULT_POINT_RADIUS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "point3d %63s at (%f,%f,%f) radius %f colour %63s", name, &x, &y, &z, &radius, colour_name);
	if (matched < 4)
		matched = sscanf(line, "point3d %63s at (%f, %f, %f) radius %f colour %63s", name, &x, &y, &z, &radius, colour_name);
	if (matched < 4)
		return set_error(p, line_no, "expected point3d name at (x,y,z) radius N colour name");
	
	int id = wb_scene_add_point3d(p->scene, x, y, z, matched >= 5 ? radius : WB_DEFAULT_POINT_RADIUS, parse_colour(matched >= 6 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create point3d object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_open_point3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, z = 0.0f, radius = WB_DEFAULT_OPEN_POINT_RADIUS, thickness = WB_DEFAULT_OPEN_POINT_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "open_point3d %63s at (%f,%f,%f) radius %f thickness %f colour %63s", name, &x, &y, &z, &radius, &thickness, colour_name);
	if (matched < 4)
		matched = sscanf(line, "open_point3d %63s at (%f, %f, %f) radius %f thickness %f colour %63s", name, &x, &y, &z, &radius, &thickness, colour_name);
	if (matched < 4)
		return set_error(p, line_no, "expected open_point3d name at (x,y,z) radius N thickness N colour name");
	
	int id = wb_scene_add_open_point3d(p->scene, x, y, z, matched >= 5 ? radius : WB_DEFAULT_OPEN_POINT_RADIUS, matched >= 6 ? thickness : WB_DEFAULT_OPEN_POINT_THICKNESS, parse_colour(matched >= 7 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create open_point3d object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_triangle3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, x2 = 0.0f, y2 = 0.0f, z2 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "triangle3d %63s points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10)
		matched = sscanf(line, "triangle3d %63s points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10)
		return set_error(p, line_no, "expected triangle3d name points (x,y,z) (x,y,z) (x,y,z) thickness N colour name");
	
	int id = wb_scene_add_triangle3d(p->scene, x0, y0, z0, x1, y1, z1, x2, y2, z2, matched >= 11 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 12 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create triangle3d object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_triangle3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, x2 = 0.0f, y2 = 0.0f, z2 = 0.0f, opacity = WB_DEFAULT_SHADE_OPACITY;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "shade_triangle3d %63s points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) colour %63s opacity %f", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, colour_name, &opacity);
	if (matched < 10)
		matched = sscanf(line, "shade_triangle3d %63s points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) colour %63s opacity %f", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, colour_name, &opacity);
	if (matched < 10)
		return set_error(p, line_no, "expected shade_triangle3d name points (x,y,z) (x,y,z) (x,y,z) colour name opacity A");
	if (opacity < 0.0f)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	int id = wb_scene_add_shade_triangle3d(p->scene, x0, y0, z0, x1, y1, z1, x2, y2, z2, parse_colour(matched >= 11 ? colour_name : "blue"), matched >= 12 ? opacity : WB_DEFAULT_SHADE_OPACITY);
	if (!id)
		return set_error(p, line_no, "failed to create shade_triangle3d object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int add_tetrahedron3d_objects(wb_spec_parser *p, const char *name,
	float ax, float ay, float az,
	float bx, float by, float bz,
	float cx, float cy, float cz,
	float dx, float dy, float dz,
	float thickness, uint32_t colour, float opacity, float jitter_strength)
{
	char part_name[64];
	int id = 0;
	
	if (!p || !name || !*name)
		return 0;
	
	if (opacity > 0.0f)
	{
		if (!wb_scene_add_shade_triangle3d(p->scene, ax, ay, az, bx, by, bz, cx, cy, cz, colour, opacity))
			return 0;
		if (!wb_scene_add_shade_triangle3d(p->scene, ax, ay, az, bx, by, bz, dx, dy, dz, colour, opacity * 0.92f))
			return 0;
		if (!wb_scene_add_shade_triangle3d(p->scene, ax, ay, az, cx, cy, cz, dx, dy, dz, colour, opacity * 0.84f))
			return 0;
		if (!wb_scene_add_shade_triangle3d(p->scene, bx, by, bz, cx, cy, cz, dx, dy, dz, colour, opacity * 0.76f))
			return 0;
	}
	
	if (!wb_scene_add_line3d(p->scene, ax, ay, az, bx, by, bz, thickness, colour) ||
		!wb_scene_add_line3d(p->scene, ax, ay, az, cx, cy, cz, thickness, colour) ||
		!wb_scene_add_line3d(p->scene, ax, ay, az, dx, dy, dz, thickness, colour) ||
		!wb_scene_add_line3d(p->scene, bx, by, bz, cx, cy, cz, thickness, colour) ||
		!wb_scene_add_line3d(p->scene, bx, by, bz, dx, dy, dz, thickness, colour) ||
		!wb_scene_add_line3d(p->scene, cx, cy, cz, dx, dy, dz, thickness, colour))
		return 0;
	
	for (int i = p->scene->n_objects - 6; i < p->scene->n_objects; i++)
	{
		if (i >= 0)
			wb_scene_set_object_jitter(p->scene, p->scene->objects[i].id, jitter_strength);
	}
	
	id = wb_scene_add_point3d(p->scene, ax, ay, az, WB_DEFAULT_POINT_RADIUS, colour);
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_a", name);
	remember_name(p, part_name, id);
	id = wb_scene_add_point3d(p->scene, bx, by, bz, WB_DEFAULT_POINT_RADIUS, colour);
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_b", name);
	remember_name(p, part_name, id);
	id = wb_scene_add_point3d(p->scene, cx, cy, cz, WB_DEFAULT_POINT_RADIUS, colour);
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_c", name);
	remember_name(p, part_name, id);
	id = wb_scene_add_point3d(p->scene, dx, dy, dz, WB_DEFAULT_POINT_RADIUS, colour);
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_d", name);
	remember_name(p, part_name, id);
	
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static int parse_tetrahedron3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float ax = 0.0f, ay = 0.0f, az = 0.0f;
	float bx = 0.0f, by = 0.0f, bz = 0.0f;
	float cx = 0.0f, cy = 0.0f, cz = 0.0f;
	float dx = 0.0f, dy = 0.0f, dz = 0.0f;
	float thickness = WB_DEFAULT_LINE_THICKNESS;
	float opacity = 0.10f;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "tetrahedron3d %63s points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) thickness %f colour %63s opacity %f",
		name, &ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz, &thickness, colour_name, &opacity);
	if (matched < 13)
		matched = sscanf(line, "tetrahedron3d %63s points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) thickness %f colour %63s opacity %f",
			name, &ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz, &thickness, colour_name, &opacity);
	if (matched < 13)
		matched = sscanf(line, "tetra3d %63s (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) t %f c %63s a %f",
			name, &ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz, &thickness, colour_name, &opacity);
	if (matched < 13)
		matched = sscanf(line, "tetra3d %63s (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) t %f c %63s a %f",
			name, &ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz, &thickness, colour_name, &opacity);
	if (matched < 13)
		return set_error(p, line_no, "expected tetrahedron3d/tetra3d name points (x,y,z) (x,y,z) (x,y,z) (x,y,z) thickness N colour name opacity A");
	
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	if (!add_tetrahedron3d_objects(p, name, ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy, dz, thickness, parse_colour(matched >= 15 ? colour_name : "blue"), matched >= 16 ? opacity : 0.10f, jitter_strength))
		return set_error(p, line_no, "failed to create tetrahedron3d object");
	if (parse_jitter_token(line, &jitter_strength))
	{
		for (int i = p->scene->n_objects - 10; i < p->scene->n_objects; i++)
		{
			if (i >= 0)
				wb_scene_set_object_jitter(p->scene, p->scene->objects[i].id, jitter_strength);
		}
	}
	return 1;
}

static int add_axes3d_objects(wb_spec_parser *p, const char *name, float x, float y, float z, float length, float thickness, float jitter_strength)
{
	char part_name[64];
	int id = 0;
	
	if (!p || !name || !*name || length <= 0.0f)
		return 0;
	
	id = wb_scene_add_line3d(p->scene, x, y, z, x + length, y, z, thickness, parse_colour("red"));
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_x", name);
	remember_name(p, part_name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	
	id = wb_scene_add_line3d(p->scene, x, y, z, x, y + length, z, thickness, parse_colour("green"));
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_y", name);
	remember_name(p, part_name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	
	id = wb_scene_add_line3d(p->scene, x, y, z, x, y, z + length, thickness, parse_colour("blue"));
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_z", name);
	remember_name(p, part_name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	
	id = wb_scene_add_point3d(p->scene, x, y, z, WB_DEFAULT_POINT_RADIUS, parse_colour("black"));
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_o", name);
	remember_name(p, part_name, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_axes3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float length = 1.0f;
	float thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "axes3d %63s at (%f,%f,%f) length %f thickness %f", name, &x, &y, &z, &length, &thickness);
	if (matched < 4)
		matched = sscanf(line, "axes3d %63s at (%f, %f, %f) length %f thickness %f", name, &x, &y, &z, &length, &thickness);
	if (matched < 4)
		matched = sscanf(line, "axes3d %63s origin (%f,%f,%f) length %f thickness %f", name, &x, &y, &z, &length, &thickness);
	if (matched < 4)
		matched = sscanf(line, "axes3d %63s origin (%f, %f, %f) length %f thickness %f", name, &x, &y, &z, &length, &thickness);
	if (matched < 4)
		matched = sscanf(line, "axes %63s (%f,%f,%f) len %f t %f", name, &x, &y, &z, &length, &thickness);
	if (matched < 4)
		matched = sscanf(line, "axes %63s (%f, %f, %f) len %f t %f", name, &x, &y, &z, &length, &thickness);
	if (matched < 4)
		return set_error(p, line_no, "expected axes3d/axes name at (x,y,z) length N thickness N");
	
	if (parse_jitter_token(line, &jitter_strength) == 0)
		jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	if (!add_axes3d_objects(p, name, x, y, z, matched >= 5 ? length : 1.0f, matched >= 6 ? thickness : WB_DEFAULT_LINE_THICKNESS, jitter_strength))
		return set_error(p, line_no, "failed to create axes3d object");
	return 1;
}

static int add_cube3d_objects(wb_spec_parser *p, const char *name, float cx, float cy, float cz, float size, float thickness, uint32_t colour, float opacity, float jitter_strength)
{
	char part_name[64];
	float h = size * 0.5f;
	wb_vec3 v[8];
	int edge_pairs[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};
	int face_tris[12][3] = {
		{0, 1, 2}, {0, 2, 3},
		{4, 5, 6}, {4, 6, 7},
		{0, 1, 5}, {0, 5, 4},
		{1, 2, 6}, {1, 6, 5},
		{2, 3, 7}, {2, 7, 6},
		{3, 0, 4}, {3, 4, 7}
	};
	int id = 0;
	
	if (!p || !name || !*name || size <= 0.0f)
		return 0;
	
	v[0] = vec3(cx - h, cy - h, cz - h);
	v[1] = vec3(cx + h, cy - h, cz - h);
	v[2] = vec3(cx + h, cy + h, cz - h);
	v[3] = vec3(cx - h, cy + h, cz - h);
	v[4] = vec3(cx - h, cy - h, cz + h);
	v[5] = vec3(cx + h, cy - h, cz + h);
	v[6] = vec3(cx + h, cy + h, cz + h);
	v[7] = vec3(cx - h, cy + h, cz + h);
	
	if (opacity > 0.0f)
	{
		for (int i = 0; i < 12; i++)
		{
			float face_opacity = opacity * (0.78f + 0.02f * (float)(i % 6));
			if (!wb_scene_add_shade_triangle3d(p->scene,
				v[face_tris[i][0]].x, v[face_tris[i][0]].y, v[face_tris[i][0]].z,
				v[face_tris[i][1]].x, v[face_tris[i][1]].y, v[face_tris[i][1]].z,
				v[face_tris[i][2]].x, v[face_tris[i][2]].y, v[face_tris[i][2]].z,
				colour, face_opacity))
				return 0;
		}
	}
	
	for (int i = 0; i < 12; i++)
	{
		id = wb_scene_add_line3d(p->scene,
			v[edge_pairs[i][0]].x, v[edge_pairs[i][0]].y, v[edge_pairs[i][0]].z,
			v[edge_pairs[i][1]].x, v[edge_pairs[i][1]].y, v[edge_pairs[i][1]].z,
			thickness, colour);
		if (!id)
			return 0;
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	
	for (int i = 0; i < 8; i++)
	{
		id = wb_scene_add_point3d(p->scene, v[i].x, v[i].y, v[i].z, WB_DEFAULT_POINT_RADIUS, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_v%d", name, i);
		remember_name(p, part_name, id);
	}
	
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static int parse_cube3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float cx = 0.0f, cy = 0.0f, cz = 0.0f;
	float size = 1.0f;
	float thickness = WB_DEFAULT_LINE_THICKNESS;
	float opacity = 0.08f;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "cube3d %63s center (%f,%f,%f) size %f thickness %f colour %63s opacity %f", name, &cx, &cy, &cz, &size, &thickness, colour_name, &opacity);
	if (matched < 4)
		matched = sscanf(line, "cube3d %63s center (%f, %f, %f) size %f thickness %f colour %63s opacity %f", name, &cx, &cy, &cz, &size, &thickness, colour_name, &opacity);
	if (matched < 4)
		matched = sscanf(line, "cube %63s (%f,%f,%f) s %f t %f c %63s a %f", name, &cx, &cy, &cz, &size, &thickness, colour_name, &opacity);
	if (matched < 4)
		matched = sscanf(line, "cube %63s (%f, %f, %f) s %f t %f c %63s a %f", name, &cx, &cy, &cz, &size, &thickness, colour_name, &opacity);
	if (matched < 4)
		return set_error(p, line_no, "expected cube3d/cube name center (x,y,z) size N thickness N colour name opacity A");
	
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	if (parse_jitter_token(line, &jitter_strength) == 0)
		jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	if (!add_cube3d_objects(p, name, cx, cy, cz, matched >= 5 ? size : 1.0f, matched >= 6 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 7 ? colour_name : "blue"), matched >= 8 ? opacity : 0.08f, jitter_strength))
		return set_error(p, line_no, "failed to create cube3d object");
	return 1;
}

static int parse_point_object(wb_spec_parser *p, char *line, int line_no, int open)
{
	char name[64];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, radius = WB_DEFAULT_POINT_RADIUS, thickness = WB_DEFAULT_OPEN_POINT_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	int id = 0;
	
	if (open)
	{
		matched = sscanf(line, "open_point %63s at (%f,%f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3)
		matched = sscanf(line, "open_point %63s at (%f, %f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3)
			matched = sscanf(line, "opt %63s (%f,%f) r %f t %f c %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3)
			matched = sscanf(line, "opt %63s (%f, %f) r %f t %f c %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3)
			return set_error(p, line_no, "expected open_point/opt name at (x,y) radius N thickness N colour name");
		id = wb_scene_add_open_point(p->scene, x, y, radius, matched >= 5 ? thickness : WB_DEFAULT_OPEN_POINT_THICKNESS, parse_colour(matched >= 6 ? colour_name : "blue"));
	}
	else
	{
		matched = sscanf(line, "point %63s at (%f,%f) radius %f colour %63s", name, &x, &y, &radius, colour_name);
		if (matched < 3)
			matched = sscanf(line, "point %63s at (%f, %f) radius %f colour %63s", name, &x, &y, &radius, colour_name);
		if (matched < 3)
			matched = sscanf(line, "pt %63s (%f,%f) r %f c %63s", name, &x, &y, &radius, colour_name);
		if (matched < 3)
			matched = sscanf(line, "pt %63s (%f, %f) r %f c %63s", name, &x, &y, &radius, colour_name);
		if (matched < 3)
			return set_error(p, line_no, "expected point/pt name at (x,y) radius N colour name");
		id = wb_scene_add_point(p->scene, x, y, matched >= 4 ? radius : WB_DEFAULT_POINT_RADIUS, parse_colour(matched >= 5 ? colour_name : "blue"));
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
	float x = 0.0f, y = 0.0f, radius = WB_DEFAULT_CIRCLE_RADIUS, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "circle %63s center (%f,%f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4)
		matched = sscanf(line, "circle %63s center (%f, %f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4)
		matched = sscanf(line, "circ %63s (%f,%f) r %f t %f c %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4)
		matched = sscanf(line, "circ %63s (%f, %f) r %f t %f c %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4)
		return set_error(p, line_no, "expected circle/circ name center (x,y) radius N thickness N colour name");
	
	int id = wb_scene_add_circle(p->scene, x, y, radius, matched >= 5 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 6 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create circle object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_ellipse_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, radius_x = WB_DEFAULT_ELLIPSE_RADIUS_X, radius_y = WB_DEFAULT_ELLIPSE_RADIUS_Y, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "ellipse %63s center (%f,%f) radii (%f,%f) thickness %f colour %63s", name, &x, &y, &radius_x, &radius_y, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ellipse %63s center (%f, %f) radii (%f, %f) thickness %f colour %63s", name, &x, &y, &radius_x, &radius_y, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ell %63s (%f,%f) rx %f ry %f t %f c %63s", name, &x, &y, &radius_x, &radius_y, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ell %63s (%f, %f) rx %f ry %f t %f c %63s", name, &x, &y, &radius_x, &radius_y, &thickness, colour_name);
	if (matched < 5)
		return set_error(p, line_no, "expected ellipse/ell name center (x,y) radii (rx,ry) thickness N colour name");
	
	int id = wb_scene_add_ellipse(p->scene, x, y, radius_x, radius_y, matched >= 6 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 7 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create ellipse object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_disc_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	char colour_name[64] = "blue";
	float x = 0.0f, y = 0.0f, radius = WB_DEFAULT_CIRCLE_RADIUS, opacity = WB_DEFAULT_SHADE_OPACITY;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "shade_disc %63s center (%f,%f) radius %f colour %63s opacity %f", name, &x, &y, &radius, colour_name, &opacity);
	if (matched < 4)
		matched = sscanf(line, "shade_disc %63s center (%f, %f) radius %f colour %63s opacity %f", name, &x, &y, &radius, colour_name, &opacity);
	if (matched < 4)
		return set_error(p, line_no, "expected shade_disc name center (x,y) radius N colour name opacity A");
	
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	int id = wb_scene_add_shade_disc(p->scene, x, y, radius, parse_colour(matched >= 5 ? colour_name : "blue"), matched >= 6 ? opacity : WB_DEFAULT_SHADE_OPACITY);
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
	if (starts_with(s, "transition "))
		return parse_transition(p, s, line_no);
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
	if (starts_with(s, "point3d "))
		return parse_point3d_object(p, s, line_no);
	if (starts_with(s, "open_point3d "))
		return parse_open_point3d_object(p, s, line_no);
	if (starts_with(s, "axes3d ") || starts_with(s, "axes "))
		return parse_axes3d_object(p, s, line_no);
	if (starts_with(s, "cube3d ") || starts_with(s, "cube "))
		return parse_cube3d_object(p, s, line_no);
	if (starts_with(s, "tetrahedron3d ") || starts_with(s, "tetra3d "))
		return parse_tetrahedron3d_object(p, s, line_no);
	if (starts_with(s, "shade_triangle3d "))
		return parse_shade_triangle3d_object(p, s, line_no);
	if (starts_with(s, "triangle3d "))
		return parse_triangle3d_object(p, s, line_no);
	if (starts_with(s, "dotted_line "))
		return parse_dotted_line_object(p, s, line_no);
	if (starts_with(s, "dashed_line ") || starts_with(s, "dash "))
		return parse_dashed_line_object(p, s, line_no);
	if (starts_with(s, "arrow "))
		return parse_arrow_object(p, s, line_no);
	if (starts_with(s, "triangle ") || starts_with(s, "tri "))
		return parse_triangle_object(p, s, line_no);
	if (starts_with(s, "shade_triangle "))
		return parse_shade_triangle_object(p, s, line_no);
	if (starts_with(s, "shade_polygon ") || starts_with(s, "shade_poly "))
		return parse_shade_polygon_object(p, s, line_no);
	if (starts_with(s, "quad "))
		return parse_quad_object(p, s, line_no);
	if (starts_with(s, "polygon ") || starts_with(s, "poly "))
		return parse_polygon_object(p, s, line_no);
	if (starts_with(s, "ray "))
		return parse_ray_object(p, s, line_no);
	if (starts_with(s, "line ") || starts_with(s, "seg "))
		return parse_line_object(p, s, line_no);
	if (starts_with(s, "circle ") || starts_with(s, "circ "))
		return parse_circle_object(p, s, line_no);
	if (starts_with(s, "ellipse ") || starts_with(s, "ell "))
		return parse_ellipse_object(p, s, line_no);
	if (starts_with(s, "shade_disc "))
		return parse_shade_disc_object(p, s, line_no);
	if (starts_with(s, "point ") || starts_with(s, "pt "))
		return parse_point_object(p, s, line_no, 0);
	if (starts_with(s, "open_point ") || starts_with(s, "opt "))
		return parse_point_object(p, s, line_no, 1);
	if (starts_with(s, "move_layer "))
		return parse_move_layer(p, s, line_no);
	if (starts_with(s, "move_camera "))
		return parse_move_camera(p, s, line_no);
	if (starts_with(s, "orbit_camera "))
		return parse_orbit_camera(p, s, line_no);
	if (starts_with(s, "fade_layer "))
		return parse_fade_layer(p, s, line_no);
	if (starts_with(s, "fade "))
		return parse_fade(p, s, line_no);
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
		expand_relative_coords(line, sizeof(line));
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
		free(parser.transitions);
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
	result.transitions = parser.transitions;
	result.n_scenes = parser.n_scenes;
	snprintf(result.output_path, sizeof(result.output_path), "%s", parser.output_path);
	for (int i = 0; i < result.n_scenes; i++)
		result.durations[i] = binary_max(result.durations[i], result.scenes[i]->total_duration);
	result.duration = 0.0f;
	for (int i = 0; i < result.n_scenes; i++)
	{
		result.duration += result.durations[i];
		if (i + 1 < result.n_scenes)
			result.duration -= binary_min(result.transitions[i].duration, binary_min(result.durations[i], result.durations[i + 1]));
	}
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
		free(video->transitions);
	}
	else if (video->scene)
		free_scene(video->scene);
	
	memset(video, 0, sizeof(*video));
}
