#include "whiteboard.h"
#include "wb_spec.h"

typedef struct
{
	char name[64];
	int id;
} wb_spec_name;

typedef struct
{
	char name[64];
	int ids[64];
	int n_ids;
} wb_spec_group;

typedef struct
{
	char name[64];
	int indent;
} wb_spec_group_scope;

typedef struct
{
	char name[64];
	int indent;
	int dimension;
	int coord_type;
	wb_vec2 origin;
	wb_vec2 scale;
	wb_vec3 origin3;
	wb_vec3 scale3;
	float rotation;
	wb_vec3 rotation3;
	int root_manifold;
	int patch_id;
} wb_spec_patch_scope;

typedef struct
{
	char name[64];
	wb_spec_patch_scope scopes[16];
	int n_scopes;
} wb_spec_patch_def;

typedef struct
{
	int indent;
	char colour_name[64];
	int colour_set;
	float thickness;
	int thickness_set;
	float opacity;
	int opacity_set;
	float jitter_strength;
	int jitter_set;
} wb_spec_defaults_scope;

typedef struct
{
	wb_scene *scene;
	wb_scene **scenes;
	float *durations;
	wb_scene_transition *transitions;
	int n_scenes;
	int cap_scenes;
	wb_spec_name names[256];
	wb_spec_group groups[64];
	wb_spec_name layers[64];
	int n_names;
	int n_groups;
	int n_layers;
	float duration;
	int in_block;
	int saw_block;
	char pending_line[2048];
	int pending_line_no;
	int pending_block_type;
	int pending_block_indent;
	wb_spec_group_scope group_scopes[16];
	int n_group_scopes;
	wb_spec_patch_scope patch_scopes[16];
	int n_patch_scopes;
	wb_spec_patch_def patch_defs[64];
	int n_patch_defs;
	int current_line_indent;
	wb_spec_defaults_scope default_scopes[16];
	int n_default_scopes;
	char output_path[256];
	char error[256];
} wb_spec_parser;

enum
{
	WB_PENDING_BLOCK_NONE = 0,
	WB_PENDING_BLOCK_SCENE = 13,
	WB_PENDING_BLOCK_VIDEO = 14,
	WB_PENDING_BLOCK_MATH = 1,
	WB_PENDING_BLOCK_TEXT = 36,
	WB_PENDING_BLOCK_CAMERA = 2,
	WB_PENDING_BLOCK_LAYER = 10,
	WB_PENDING_BLOCK_BACKGROUND = 11,
	WB_PENDING_BLOCK_TRANSITION = 12,
	WB_PENDING_BLOCK_RAY = 15,
	WB_PENDING_BLOCK_DOTTED = 16,
	WB_PENDING_BLOCK_DASHED = 17,
	WB_PENDING_BLOCK_ARROW = 18,
	WB_PENDING_BLOCK_TRIANGLE = 19,
	WB_PENDING_BLOCK_SHADE_TRIANGLE = 20,
	WB_PENDING_BLOCK_ELLIPSE = 21,
	WB_PENDING_BLOCK_SHADE_DISC = 22,
	WB_PENDING_BLOCK_QUAD = 23,
	WB_PENDING_BLOCK_POLYGON = 24,
	WB_PENDING_BLOCK_SHADE_POLYGON = 25,
	WB_PENDING_BLOCK_BLOB = 38,
	WB_PENDING_BLOCK_SHADE_BLOB = 39,
	WB_PENDING_BLOCK_LINE3D = 26,
	WB_PENDING_BLOCK_CURVE3D = 27,
	WB_PENDING_BLOCK_WIRE3D = 40,
	WB_PENDING_BLOCK_SHADE_POLY3D = 41,
	WB_PENDING_BLOCK_SURFACE3D = 42,
	WB_PENDING_BLOCK_MESH3D = 43,
	WB_PENDING_BLOCK_BLOB3D = 44,
	WB_PENDING_BLOCK_PARAM3D = 45,
	WB_PENDING_BLOCK_PARAM_SURFACE3D = 46,
	WB_PENDING_BLOCK_VOLUME3D = 47,
	WB_PENDING_BLOCK_POINT3D = 28,
	WB_PENDING_BLOCK_OPEN_POINT3D = 29,
	WB_PENDING_BLOCK_TRIANGLE3D = 30,
	WB_PENDING_BLOCK_SHADE_TRIANGLE3D = 31,
	WB_PENDING_BLOCK_AXES3D = 32,
	WB_PENDING_BLOCK_TETRA3D = 33,
	WB_PENDING_BLOCK_CUBE3D = 34,
	WB_PENDING_BLOCK_LINE = 3,
	WB_PENDING_BLOCK_POINT = 4,
	WB_PENDING_BLOCK_OPEN_POINT = 5,
	WB_PENDING_BLOCK_CIRCLE = 6,
	WB_PENDING_BLOCK_DRAW = 7,
	WB_PENDING_BLOCK_FADE = 8,
	WB_PENDING_BLOCK_MOVE = 9,
	WB_PENDING_BLOCK_MOVE_PATCH = 49,
	WB_PENDING_BLOCK_TURN_PATCH = 50,
	WB_PENDING_BLOCK_SCALE_PATCH = 51,
	WB_PENDING_BLOCK_TURN = 52,
	WB_PENDING_BLOCK_SCALE = 53,
	WB_PENDING_BLOCK_DEFAULTS = 35,
	WB_PENDING_BLOCK_CURVE = 37,
	WB_PENDING_BLOCK_PATCH = 48,
};

enum
{
	WB_PATCH_COORD_CARTESIAN = 0,
	WB_PATCH_COORD_POLAR = 1,
	WB_PATCH_COORD_CYLINDRICAL = 2,
	WB_PATCH_COORD_SPHERICAL = 3,
};

static void remember_layer(wb_spec_parser *p, const char *name, int id);
static void sync_retained_patch_scope(wb_spec_parser *p, const wb_spec_patch_scope *scope);
static int parse_patch(wb_spec_parser *p, char *line, int line_no);

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

static int leading_indent_width(const char *s)
{
	int width = 0;
	
	if (!s)
		return 0;
	while (*s == ' ' || *s == '\t')
	{
		width += (*s == '\t') ? 4 : 1;
		s++;
	}
	return width;
}

static int is_unit_boundary_char(char c)
{
	return c == 0 || c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
		c == ',' || c == ')' || c == ']' || c == ':' || c == ';';
}

static void expand_dimension_units(char *line, size_t cap)
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
		
		if (!in_quotes &&
			((line[i] >= '0' && line[i] <= '9') ||
			 ((line[i] == '-' || line[i] == '+') && ((line[i + 1] >= '0' && line[i + 1] <= '9') || line[i + 1] == '.')) ||
			 (line[i] == '.' && (line[i + 1] >= '0' && line[i + 1] <= '9'))))
		{
			char *end = NULL;
			double value = strtod(line + i, &end);
			size_t consumed = end ? (size_t)(end - (line + i)) : 0;
			
			if (consumed > 0 && end && (*end == 'w' || *end == 'h' || *end == 'm') && is_unit_boundary_char(*(end + 1)))
			{
				double scale = 1.0;
				int written;
				
				if (*end == 'w')
					scale = WIDTH;
				else if (*end == 'h')
					scale = HEIGHT;
				else
					scale = binary_min(WIDTH, HEIGHT);
				written = snprintf(out + j, sizeof(out) - j, "%.3f", value * scale);
				if (written <= 0 || (size_t)written >= sizeof(out) - j)
					break;
				j += (size_t)written;
				i += consumed + 1;
				continue;
			}
		}
		
		out[j++] = line[i++];
	}
	
	out[j] = 0;
	snprintf(line, cap, "%s", out);
}

static void expand_relative_coords(char *line, size_t cap)
{
	char out[2048];
	size_t i = 0;
	size_t j = 0;
	int in_quotes = 0;
	float geo_scale = binary_min(WIDTH, HEIGHT);
	
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
				if (fabsf(rx) > 1.5f || fabsf(ry) > 1.5f)
				{
					out[j++] = line[i++];
					continue;
				}
				int written = snprintf(out + j, sizeof(out) - j, "(%.3f,%.3f)", rx * WIDTH, ry * HEIGHT);
				
				if (written <= 0 || (size_t)written >= sizeof(out) - j)
					break;
				j += (size_t)written;
				i += (size_t)consumed;
				continue;
			}
		}
		if (!in_quotes && line[i] == '{')
		{
			float gx = 0.0f;
			float gy = 0.0f;
			int consumed = 0;
			int matched = sscanf(line + i, "{%f,%f}%n", &gx, &gy, &consumed);
			
			if (matched != 2)
				matched = sscanf(line + i, "{%f, %f}%n", &gx, &gy, &consumed);
			if (matched == 2 && consumed > 0)
			{
				int written = snprintf(out + j, sizeof(out) - j, "(%.3f,%.3f)", gx * geo_scale, gy * geo_scale);
				
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

static int starts_with_word(const char *s, const char *word)
{
	size_t n;
	
	if (!s || !word)
		return 0;
	n = strlen(word);
	if (strncmp(s, word, n) != 0)
		return 0;
	return s[n] == 0 || s[n] == ' ' || s[n] == '\t';
}

static int current_layer_type(const wb_spec_parser *p)
{
	if (p && p->n_patch_scopes > 0)
		return p->patch_scopes[p->n_patch_scopes - 1].dimension == 3 ? WB_LAYER_3D : WB_LAYER_2D;
	if (!p || !p->scene)
		return WB_LAYER_2D;
	return WB_LAYER_2D;
}

/* The implicit root patch is a mathematical viewport.  Its vertical span is
 * [-1, 1], while its horizontal span follows the output aspect ratio.  The
 * renderer still consumes pixels, so flat root-patch objects are converted
 * once, here, at the parser/scene boundary. */
static float root_world_pixel_scale(const wb_scene *scene)
{
	float half_height = scene ? scene->root_viewport.half_height : 1.0f;
	if (half_height <= 0.0f)
		half_height = 1.0f;
	return (float)HEIGHT * 0.5f / half_height;
}

static wb_vec2 root_world_to_pixel(const wb_scene *scene, wb_vec2 point)
{
	wb_vec2 center = scene ? scene->root_viewport.center : vec2(0, 0);
	float scale = root_world_pixel_scale(scene);
	return vec2((float)WIDTH * 0.5f + (point.x - center.x) * scale,
		(float)HEIGHT * 0.5f - (point.y - center.y) * scale);
}

static float root_world_length_to_pixel(const wb_scene *scene, float length)
{
	return length * root_world_pixel_scale(scene);
}

static void root_worldify_stored_points(const wb_scene *scene, wb_scene_object *obj)
{
	wb_vec2 points[7];
	int n;

	if (!obj)
		return;
	n = (int)(obj->radius + 0.5f);
	if (n < 3 || n > 7)
		return;
	points[0] = obj->p0;
	points[1] = obj->p1;
	points[2] = vec2(obj->q0.x, obj->q0.y);
	points[3] = vec2(obj->q1.x, obj->q1.y);
	points[4] = vec2(obj->q2.x, obj->q2.y);
	points[5] = vec2(obj->x, obj->y);
	points[6] = vec2(obj->q2.z, obj->size);
	for (int i = 0; i < n; i++)
		points[i] = root_world_to_pixel(scene, points[i]);
	obj->p0 = points[0];
	obj->p1 = points[1];
	obj->q0.x = points[2].x;
	obj->q0.y = points[2].y;
	obj->q1.x = points[3].x;
	obj->q1.y = points[3].y;
	obj->q2.x = points[4].x;
	obj->q2.y = points[4].y;
	obj->x = points[5].x;
	obj->y = points[5].y;
	obj->q2.z = points[6].x;
	obj->size = points[6].y;
}

static void root_worldify_object(const wb_scene *scene, wb_scene_object *obj)
{
	if (!obj)
		return;

	switch (obj->type)
	{
	case WB_OBJECT_MATH:
	case WB_OBJECT_TEXT:
		{
			wb_vec2 p = root_world_to_pixel(scene, vec2(obj->x, obj->y));
			obj->x = p.x;
			obj->y = p.y;
			obj->size = root_world_length_to_pixel(scene, obj->size);
		}
		break;
	case WB_OBJECT_CURVE:
	case WB_OBJECT_TRIANGLE:
	case WB_OBJECT_SHADE_TRIANGLE:
		obj->p0 = root_world_to_pixel(scene, obj->p0);
		obj->p1 = root_world_to_pixel(scene, obj->p1);
		{
			wb_vec2 p = root_world_to_pixel(scene, vec2(obj->x, obj->y));
			obj->x = p.x;
			obj->y = p.y;
		}
		if (obj->type != WB_OBJECT_SHADE_TRIANGLE)
			obj->thickness = root_world_length_to_pixel(scene, obj->thickness);
		break;
	case WB_OBJECT_LINE:
	case WB_OBJECT_RAY:
	case WB_OBJECT_DOTTED_LINE:
	case WB_OBJECT_DASHED_LINE:
	case WB_OBJECT_ARROW:
		obj->p0 = root_world_to_pixel(scene, obj->p0);
		obj->p1 = root_world_to_pixel(scene, obj->p1);
		obj->thickness = root_world_length_to_pixel(scene, obj->thickness);
		if (obj->type == WB_OBJECT_DOTTED_LINE || obj->type == WB_OBJECT_DASHED_LINE || obj->type == WB_OBJECT_ARROW)
			obj->size = root_world_length_to_pixel(scene, obj->size);
		break;
	case WB_OBJECT_QUAD:
		obj->p0 = root_world_to_pixel(scene, obj->p0);
		obj->p1 = root_world_to_pixel(scene, obj->p1);
		{
			wb_vec2 p = root_world_to_pixel(scene, vec2(obj->q0.x, obj->q0.y));
			obj->q0.x = p.x; obj->q0.y = p.y;
			p = root_world_to_pixel(scene, vec2(obj->q1.x, obj->q1.y));
			obj->q1.x = p.x; obj->q1.y = p.y;
		}
		obj->thickness = root_world_length_to_pixel(scene, obj->thickness);
		break;
	case WB_OBJECT_POLYGON:
	case WB_OBJECT_SHADE_POLYGON:
	case WB_OBJECT_BLOB:
	case WB_OBJECT_SHADE_BLOB:
		root_worldify_stored_points(scene, obj);
		if (obj->type == WB_OBJECT_POLYGON || obj->type == WB_OBJECT_BLOB)
			obj->thickness = root_world_length_to_pixel(scene, obj->thickness);
		break;
	case WB_OBJECT_SHADE_DISC:
	case WB_OBJECT_POINT:
	case WB_OBJECT_OPEN_POINT:
	case WB_OBJECT_CIRCLE:
	case WB_OBJECT_ELLIPSE:
		{
			wb_vec2 p = root_world_to_pixel(scene, vec2(obj->x, obj->y));
			obj->x = p.x;
			obj->y = p.y;
			obj->radius = root_world_length_to_pixel(scene, obj->radius);
			obj->p0.x = root_world_length_to_pixel(scene, obj->p0.x);
			obj->p0.y = root_world_length_to_pixel(scene, obj->p0.y);
			obj->thickness = root_world_length_to_pixel(scene, obj->thickness);
		}
		break;
	default:
		break;
	}
}

static void root_worldify_action(const wb_scene *scene, wb_scene_action *action)
{
	float scale;

	if (!action)
		return;
	scale = root_world_pixel_scale(scene);
	switch (action->type)
	{
	case WB_ACTION_MOVE:
		action->from = root_world_to_pixel(scene, action->from);
		action->to = root_world_to_pixel(scene, action->to);
		break;
	case WB_ACTION_TRANSLATE:
		action->from = vec2(action->from.x * scale, -action->from.y * scale);
		action->to = vec2(action->to.x * scale, -action->to.y * scale);
		break;
	case WB_ACTION_TRANSFORM:
		action->q0 = vec3(root_world_to_pixel(scene, vec2(action->q0.x, action->q0.y)).x,
			root_world_to_pixel(scene, vec2(action->q0.x, action->q0.y)).y, 0);
		break;
	default:
		break;
	}
}

static float active_patch_length_scale(const wb_spec_parser *p)
{
	float scale = 1.0f;

	if (!p)
		return scale;
	for (int i = 0; i < p->n_patch_scopes; i++)
	{
		const wb_spec_patch_scope *scope = &p->patch_scopes[i];
		if (scope->dimension != 2)
			continue;
		scale *= (fabsf(scope->scale.x) + fabsf(scope->scale.y)) * 0.5f;
	}
	return scale;
}

static void scale_2d_object_lengths(wb_scene_object *obj, float scale)
{
	if (!obj || scale == 1.0f)
		return;

	switch (obj->type)
	{
	case WB_OBJECT_MATH:
	case WB_OBJECT_TEXT:
		obj->size *= scale;
		break;
	case WB_OBJECT_CURVE:
	case WB_OBJECT_LINE:
	case WB_OBJECT_RAY:
	case WB_OBJECT_TRIANGLE:
	case WB_OBJECT_QUAD:
	case WB_OBJECT_POLYGON:
	case WB_OBJECT_BLOB:
		obj->thickness *= scale;
		break;
	case WB_OBJECT_DOTTED_LINE:
	case WB_OBJECT_DASHED_LINE:
	case WB_OBJECT_ARROW:
		obj->thickness *= scale;
		obj->size *= scale;
		break;
	case WB_OBJECT_SHADE_DISC:
	case WB_OBJECT_POINT:
	case WB_OBJECT_OPEN_POINT:
	case WB_OBJECT_CIRCLE:
	case WB_OBJECT_ELLIPSE:
		obj->radius *= scale;
		obj->p0.x *= scale;
		obj->p0.y *= scale;
		obj->thickness *= scale;
		break;
	default:
		break;
	}
}

static wb_vec2 patch_local_to_parent(const wb_spec_patch_scope *scope, wb_vec2 p)
{
	float c;
	float s;
	
	if (!scope)
		return p;
	if (scope->coord_type == WB_PATCH_COORD_POLAR)
		p = vec2(p.x * cosf(p.y), p.x * sinf(p.y));
	p = vec2(p.x * scope->scale.x, p.y * scope->scale.y);
	c = cosf(scope->rotation);
	s = sinf(scope->rotation);
	p = vec2(c * p.x - s * p.y, s * p.x + c * p.y);
	return vec2_sum(p, scope->origin);
}

static wb_vec3 patch_local3d_to_parent(const wb_spec_patch_scope *scope, wb_vec3 p)
{
	float cy;
	float sy;
	float cx;
	float sx;
	float cz;
	float sz;
	float x;
	float y;
	float z;
	float radius;
	float theta;
	float phi;
	
	if (!scope)
		return p;
	if (scope->coord_type == WB_PATCH_COORD_CYLINDRICAL)
	{
		radius = p.x;
		theta = p.y;
		p = vec3(radius * cosf(theta), p.z, radius * sinf(theta));
	}
	else if (scope->coord_type == WB_PATCH_COORD_SPHERICAL)
	{
		radius = p.x;
		theta = p.y;
		phi = p.z;
		p = vec3(radius * cosf(phi) * cosf(theta), radius * sinf(phi), radius * cosf(phi) * sinf(theta));
	}
	p = vec3(p.x * scope->scale3.x, p.y * scope->scale3.y, p.z * scope->scale3.z);
	cy = cosf(scope->rotation3.x);
	sy = sinf(scope->rotation3.x);
	cx = cosf(scope->rotation3.y);
	sx = sinf(scope->rotation3.y);
	cz = cosf(scope->rotation3.z);
	sz = sinf(scope->rotation3.z);
	x = cy * p.x + sy * p.z;
	z = -sy * p.x + cy * p.z;
	p = vec3(x, p.y, z);
	y = cx * p.y - sx * p.z;
	z = sx * p.y + cx * p.z;
	p = vec3(p.x, y, z);
	x = cz * p.x - sz * p.y;
	y = sz * p.x + cz * p.y;
	p = vec3(x, y, p.z);
	return vec3_sum(p, scope->origin3);
}

static wb_vec2 apply_active_patch_to_point(const wb_spec_parser *p, wb_vec2 point)
{
	if (!p)
		return point;
	for (int i = 0; i < p->n_patch_scopes; i++)
		point = patch_local_to_parent(&p->patch_scopes[i], point);
	return point;
}

static wb_vec3 apply_active_patch_to_point3d(const wb_spec_parser *p, wb_vec3 point)
{
	if (!p)
		return point;
	for (int i = 0; i < p->n_patch_scopes; i++)
	{
		if (p->patch_scopes[i].dimension == 3)
			point = patch_local3d_to_parent(&p->patch_scopes[i], point);
	}
	return point;
}

static wb_vec2 apply_patch_scope_chain_to_point(const wb_spec_patch_scope *scopes, int n_scopes, wb_vec2 point)
{
	for (int i = 0; i < n_scopes; i++)
		point = patch_local_to_parent(&scopes[i], point);
	return point;
}

static wb_vec3 apply_patch_scope_chain_to_point3d(const wb_spec_patch_scope *scopes, int n_scopes, wb_vec3 point)
{
	for (int i = 0; i < n_scopes; i++)
	{
		if (scopes[i].dimension == 3)
			point = patch_local3d_to_parent(&scopes[i], point);
	}
	return point;
}

static int pending_block_is_patch_sensitive(int pending_type)
{
	switch (pending_type)
	{
	case WB_PENDING_BLOCK_MATH:
	case WB_PENDING_BLOCK_TEXT:
	case WB_PENDING_BLOCK_CURVE:
	case WB_PENDING_BLOCK_RAY:
	case WB_PENDING_BLOCK_DOTTED:
	case WB_PENDING_BLOCK_DASHED:
	case WB_PENDING_BLOCK_ARROW:
	case WB_PENDING_BLOCK_TRIANGLE:
	case WB_PENDING_BLOCK_SHADE_TRIANGLE:
	case WB_PENDING_BLOCK_ELLIPSE:
	case WB_PENDING_BLOCK_SHADE_DISC:
	case WB_PENDING_BLOCK_QUAD:
	case WB_PENDING_BLOCK_POLYGON:
	case WB_PENDING_BLOCK_SHADE_POLYGON:
	case WB_PENDING_BLOCK_BLOB:
	case WB_PENDING_BLOCK_SHADE_BLOB:
	case WB_PENDING_BLOCK_LINE:
	case WB_PENDING_BLOCK_POINT:
	case WB_PENDING_BLOCK_OPEN_POINT:
	case WB_PENDING_BLOCK_CIRCLE:
	case WB_PENDING_BLOCK_MOVE:
		return 1;
	default:
		return 0;
	}
}

static int line_should_apply_active_patch(const wb_spec_parser *p, const char *s)
{
	if (!p || p->n_patch_scopes <= 0 || !s || !*s)
		return 0;
	if (p->pending_block_type != WB_PENDING_BLOCK_NONE)
		return 0;
	if (starts_with_word(s, "scene") || starts_with_word(s, "video") ||
		starts_with_word(s, "background") || starts_with_word(s, "transition") ||
		starts_with_word(s, "defaults") || starts_with_word(s, "layer") ||
		starts_with_word(s, "camera") || starts_with_word(s, "cam") ||
		starts_with_word(s, "patch") || starts_with_word(s, "group") ||
		starts_with(s, "move_layer ") || starts_with(s, "move_patch ") || starts_with(s, "move_camera ") ||
		starts_with(s, "orbit_camera ") || starts_with(s, "fade_layer "))
		return 0;
	return 1;
}

static void apply_active_patch_to_line(wb_spec_parser *p, char *line, size_t cap)
{
	char out[2048];
	size_t i = 0;
	size_t j = 0;
	int in_quotes = 0;
	int touched = 0;
	
	if (!p || !line || cap == 0 || !line_should_apply_active_patch(p, trim_left(line)))
		return;
	
	while (line[i] && j + 1 < sizeof(out))
	{
		if (line[i] == '"')
		{
			out[j++] = line[i++];
			in_quotes = !in_quotes;
			continue;
		}
		if (!in_quotes && line[i] == '(')
		{
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			int consumed = 0;
			if ((sscanf(line + i, "(%f,%f,%f)%n", &x, &y, &z, &consumed) == 3 ||
				 sscanf(line + i, "(%f, %f, %f)%n", &x, &y, &z, &consumed) == 3) && consumed > 0)
			{
				if (current_layer_type(p) == WB_LAYER_3D)
				{
					wb_vec3 mapped = apply_active_patch_to_point3d(p, vec3(x, y, z));
					int written = snprintf(out + j, sizeof(out) - j, "(%.3f,%.3f,%.3f)", mapped.x, mapped.y, mapped.z);
					if (written <= 0 || (size_t)written >= sizeof(out) - j)
						break;
					touched = 1;
					j += (size_t)written;
				}
				else
				{
					size_t k;
					for (k = 0; k < (size_t)consumed && j + 1 < sizeof(out); k++)
						out[j++] = line[i + k];
				}
				i += (size_t)consumed;
				continue;
			}
			if ((sscanf(line + i, "(%f,%f)%n", &x, &y, &consumed) == 2 ||
				 sscanf(line + i, "(%f, %f)%n", &x, &y, &consumed) == 2) && consumed > 0)
			{
				if (current_layer_type(p) == WB_LAYER_2D)
				{
					wb_vec2 mapped = apply_active_patch_to_point(p, vec2(x, y));
					int written = snprintf(out + j, sizeof(out) - j, "(%.3f,%.3f)", mapped.x, mapped.y);
					if (written <= 0 || (size_t)written >= sizeof(out) - j)
						break;
					touched = 1;
					j += (size_t)written;
				}
				else
				{
					size_t k;
					for (k = 0; k < (size_t)consumed && j + 1 < sizeof(out); k++)
						out[j++] = line[i + k];
				}
				i += (size_t)consumed;
				continue;
			}
		}
		out[j++] = line[i++];
	}
	out[j] = 0;
	if (touched)
		snprintf(line, cap, "%s", out);
}

static int looks_like_time_range(const char *s)
{
	float t0 = 0.0f, t1 = 0.0f;
	
	if (!s || !*s)
		return 0;
	return sscanf(s, "%fs..%fs", &t0, &t1) == 2;
}

static int looks_like_duration(const char *s)
{
	float t = 0.0f;
	int consumed = 0;
	
	if (!s || !*s)
		return 0;
	return sscanf(s, "%fs%n", &t, &consumed) == 1 && consumed > 0 && s[consumed] == 0;
}

static int looks_like_vec2_literal(const char *s)
{
	float x = 0.0f, y = 0.0f;
	int consumed = 0;
	
	if (!s || !*s)
		return 0;
	return (sscanf(s, "(%f,%f)%n", &x, &y, &consumed) == 2 ||
		sscanf(s, "(%f, %f)%n", &x, &y, &consumed) == 2 ||
		sscanf(s, "[%f,%f]%n", &x, &y, &consumed) == 2 ||
		sscanf(s, "[%f, %f]%n", &x, &y, &consumed) == 2) && s[consumed] == 0;
}

static int looks_like_vec3_literal(const char *s)
{
	float x = 0.0f, y = 0.0f, z = 0.0f;
	int consumed = 0;
	
	if (!s || !*s)
		return 0;
	return (sscanf(s, "(%f,%f,%f)%n", &x, &y, &z, &consumed) == 3 ||
		sscanf(s, "(%f, %f, %f)%n", &x, &y, &z, &consumed) == 3) && s[consumed] == 0;
}

static int looks_like_vec2_arrow(const char *s)
{
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
	int consumed = 0;
	
	if (!s || !*s)
		return 0;
	return (sscanf(s, "(%f,%f) -> (%f,%f)%n", &x0, &y0, &x1, &y1, &consumed) == 4 ||
		sscanf(s, "(%f, %f) -> (%f, %f)%n", &x0, &y0, &x1, &y1, &consumed) == 4 ||
		sscanf(s, "[%f,%f] -> [%f,%f]%n", &x0, &y0, &x1, &y1, &consumed) == 4 ||
		sscanf(s, "[%f, %f] -> [%f, %f]%n", &x0, &y0, &x1, &y1, &consumed) == 4) && s[consumed] == 0;
}

static int looks_like_vec3_arrow(const char *s)
{
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	int consumed = 0;
	
	if (!s || !*s)
		return 0;
	return (sscanf(s, "(%f,%f,%f) -> (%f,%f,%f)%n", &x0, &y0, &z0, &x1, &y1, &z1, &consumed) == 6 ||
		sscanf(s, "(%f, %f, %f) -> (%f, %f, %f)%n", &x0, &y0, &z0, &x1, &y1, &z1, &consumed) == 6) && s[consumed] == 0;
}

static int count_vec2_literals(const char *s)
{
	int count = 0;
	const char *cursor = s;
	
	if (!s)
		return 0;
	while (cursor && *cursor)
	{
		float x = 0.0f, y = 0.0f;
		int consumed = 0;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		if (*cursor == 0)
			break;
		if (sscanf(cursor, "(%f,%f)%n", &x, &y, &consumed) == 2 ||
			sscanf(cursor, "(%f, %f)%n", &x, &y, &consumed) == 2 ||
			sscanf(cursor, "[%f,%f]%n", &x, &y, &consumed) == 2 ||
			sscanf(cursor, "[%f, %f]%n", &x, &y, &consumed) == 2)
		{
			count++;
			cursor += consumed;
			continue;
		}
		return 0;
	}
	return count;
}

static int count_vec3_literals(const char *s)
{
	int count = 0;
	const char *cursor = s;
	
	if (!s)
		return 0;
	while (cursor && *cursor)
	{
		float x = 0.0f, y = 0.0f, z = 0.0f;
		int consumed = 0;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		if (*cursor == 0)
			break;
		if (sscanf(cursor, "(%f,%f,%f)%n", &x, &y, &z, &consumed) == 3 ||
			sscanf(cursor, "(%f, %f, %f)%n", &x, &y, &z, &consumed) == 3)
		{
			count++;
			cursor += consumed;
			continue;
		}
		return 0;
	}
	return count;
}

static int looks_like_scalar_arrow(const char *s)
{
	float a0 = 0.0f, a1 = 0.0f;
	
	if (!s || !*s)
		return 0;
	return sscanf(s, "%f -> %f", &a0, &a1) == 2;
}

static int pending_block_type_for_line(const char *s)
{
	if (starts_with_word(s, "scene"))
		return WB_PENDING_BLOCK_SCENE;
	if (starts_with_word(s, "video"))
		return WB_PENDING_BLOCK_VIDEO;
	if (starts_with_word(s, "math"))
		return WB_PENDING_BLOCK_MATH;
	if (starts_with_word(s, "text"))
		return WB_PENDING_BLOCK_TEXT;
	if (starts_with_word(s, "curve"))
		return WB_PENDING_BLOCK_CURVE;
	if (strcmp(s, "camera") == 0 || strcmp(s, "cam") == 0)
		return WB_PENDING_BLOCK_CAMERA;
	if (starts_with_word(s, "layer"))
		return WB_PENDING_BLOCK_LAYER;
	if (starts_with_word(s, "patch"))
		return WB_PENDING_BLOCK_PATCH;
	if (starts_with_word(s, "defaults"))
		return WB_PENDING_BLOCK_DEFAULTS;
	if (starts_with_word(s, "background"))
		return WB_PENDING_BLOCK_BACKGROUND;
	if (starts_with_word(s, "transition"))
		return WB_PENDING_BLOCK_TRANSITION;
	if (starts_with_word(s, "ray"))
		return WB_PENDING_BLOCK_RAY;
	if (starts_with_word(s, "dotted_line"))
		return WB_PENDING_BLOCK_DOTTED;
	if (starts_with_word(s, "dashed_line") || starts_with_word(s, "dash"))
		return WB_PENDING_BLOCK_DASHED;
	if (starts_with_word(s, "arrow"))
		return WB_PENDING_BLOCK_ARROW;
	if (starts_with_word(s, "shade_triangle"))
		return WB_PENDING_BLOCK_SHADE_TRIANGLE;
	if (starts_with_word(s, "triangle") || starts_with_word(s, "tri"))
		return WB_PENDING_BLOCK_TRIANGLE;
	if (starts_with_word(s, "ellipse") || starts_with_word(s, "ell"))
		return WB_PENDING_BLOCK_ELLIPSE;
	if (starts_with_word(s, "shade_disc"))
		return WB_PENDING_BLOCK_SHADE_DISC;
	if (starts_with_word(s, "shade_polygon") || starts_with_word(s, "shade_poly"))
		return WB_PENDING_BLOCK_SHADE_POLYGON;
	if (starts_with_word(s, "shade_blob"))
		return WB_PENDING_BLOCK_SHADE_BLOB;
	if (starts_with_word(s, "blob"))
		return WB_PENDING_BLOCK_BLOB;
	if (starts_with_word(s, "quad"))
		return WB_PENDING_BLOCK_QUAD;
	if (starts_with_word(s, "polygon") || starts_with_word(s, "poly"))
		return WB_PENDING_BLOCK_POLYGON;
	if (starts_with_word(s, "line3d"))
		return WB_PENDING_BLOCK_LINE3D;
	if (starts_with_word(s, "curve3d"))
		return WB_PENDING_BLOCK_CURVE3D;
	if (starts_with_word(s, "wireframe3d") || starts_with_word(s, "wire3d") || starts_with_word(s, "polygon3d"))
		return WB_PENDING_BLOCK_WIRE3D;
	if (starts_with_word(s, "shade_polygon3d") || starts_with_word(s, "shade_poly3d"))
		return WB_PENDING_BLOCK_SHADE_POLY3D;
	if (starts_with_word(s, "surface3d"))
		return WB_PENDING_BLOCK_SURFACE3D;
	if (starts_with_word(s, "mesh3d"))
		return WB_PENDING_BLOCK_MESH3D;
	if (starts_with_word(s, "blob3d"))
		return WB_PENDING_BLOCK_BLOB3D;
	if (starts_with_word(s, "param3d") || starts_with_word(s, "parametric3d"))
		return WB_PENDING_BLOCK_PARAM3D;
	if (starts_with_word(s, "param_surface3d") || starts_with_word(s, "parametric_surface3d"))
		return WB_PENDING_BLOCK_PARAM_SURFACE3D;
	if (starts_with_word(s, "volume3d") || starts_with_word(s, "ellipsoid3d"))
		return WB_PENDING_BLOCK_VOLUME3D;
	if (starts_with_word(s, "point3d"))
		return WB_PENDING_BLOCK_POINT3D;
	if (starts_with_word(s, "open_point3d"))
		return WB_PENDING_BLOCK_OPEN_POINT3D;
	if (starts_with_word(s, "shade_triangle3d"))
		return WB_PENDING_BLOCK_SHADE_TRIANGLE3D;
	if (starts_with_word(s, "triangle3d"))
		return WB_PENDING_BLOCK_TRIANGLE3D;
	if (starts_with_word(s, "axes3d") || starts_with_word(s, "axes"))
		return WB_PENDING_BLOCK_AXES3D;
	if (starts_with_word(s, "tetrahedron3d") || starts_with_word(s, "tetra3d"))
		return WB_PENDING_BLOCK_TETRA3D;
	if (starts_with_word(s, "cube3d") || starts_with_word(s, "cube"))
		return WB_PENDING_BLOCK_CUBE3D;
	if (starts_with_word(s, "line") || starts_with_word(s, "seg"))
		return WB_PENDING_BLOCK_LINE;
	if (starts_with_word(s, "point") || starts_with_word(s, "pt"))
		return WB_PENDING_BLOCK_POINT;
	if (starts_with_word(s, "open_point") || starts_with_word(s, "opt"))
		return WB_PENDING_BLOCK_OPEN_POINT;
	if (starts_with_word(s, "circle") || starts_with_word(s, "circ"))
		return WB_PENDING_BLOCK_CIRCLE;
	if (starts_with_word(s, "draw"))
		return WB_PENDING_BLOCK_DRAW;
	if (starts_with_word(s, "fade"))
		return WB_PENDING_BLOCK_FADE;
	if (starts_with_word(s, "move_patch"))
		return WB_PENDING_BLOCK_MOVE_PATCH;
	if (starts_with_word(s, "turn_patch"))
		return WB_PENDING_BLOCK_TURN_PATCH;
	if (starts_with_word(s, "scale_patch"))
		return WB_PENDING_BLOCK_SCALE_PATCH;
	if (starts_with_word(s, "turn"))
		return WB_PENDING_BLOCK_TURN;
	if (starts_with_word(s, "scale"))
		return WB_PENDING_BLOCK_SCALE;
	if (starts_with_word(s, "move"))
		return WB_PENDING_BLOCK_MOVE;
	return WB_PENDING_BLOCK_NONE;
}

static int pending_block_accepts_property(int pending_type, const char *s)
{
	if (!s || !*s)
		return 0;
	if (pending_type == WB_PENDING_BLOCK_MATH)
	{
		return starts_with_word(s, "at") || starts_with_word(s, "@") || looks_like_vec2_literal(s) ||
			starts_with_word(s, "size") || starts_with_word(s, "s") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "jitter");
	}
	if (pending_type == WB_PENDING_BLOCK_TEXT)
	{
		return starts_with_word(s, "at") || starts_with_word(s, "@") || looks_like_vec2_literal(s) ||
			starts_with_word(s, "size") || starts_with_word(s, "s") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	}
	if (pending_type == WB_PENDING_BLOCK_CURVE)
		return starts_with_word(s, "through") || count_vec2_literals(s) == 3 ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_SCENE)
		return starts_with_word(s, "duration") || looks_like_duration(s);
	if (pending_type == WB_PENDING_BLOCK_VIDEO)
		return starts_with_word(s, "fps") ||
			starts_with_word(s, "jitter_fps") ||
			starts_with_word(s, "output");
	if (pending_type == WB_PENDING_BLOCK_CAMERA)
	{
		return starts_with_word(s, "distance") || starts_with_word(s, "d") ||
			starts_with_word(s, "scale") || starts_with_word(s, "s") ||
			starts_with_word(s, "yaw") || starts_with_word(s, "y") ||
			starts_with_word(s, "projection") || starts_with_word(s, "proj") ||
			starts_with_word(s, "center") || starts_with_word(s, "@") ||
			starts_with_word(s, "look_at") || starts_with_word(s, "target");
	}
	if (pending_type == WB_PENDING_BLOCK_LAYER)
	{
		return starts_with_word(s, "type") ||
			starts_with_word(s, "opacity") || starts_with_word(s, "o") ||
			starts_with_word(s, "blur") ||
			starts_with_word(s, "glow") ||
			starts_with_word(s, "glow_opacity") ||
			starts_with_word(s, "jitter");
	}
	if (pending_type == WB_PENDING_BLOCK_PATCH)
	{
		return starts_with_word(s, "at") || starts_with_word(s, "origin") || starts_with_word(s, "@") ||
			starts_with_word(s, "scale") || starts_with_word(s, "s") ||
			starts_with_word(s, "yaw") || starts_with_word(s, "pitch") || starts_with_word(s, "roll") ||
			starts_with_word(s, "rotate") || starts_with_word(s, "rotation") || starts_with_word(s, "angle") ||
			starts_with_word(s, "coords") || starts_with_word(s, "coord") || starts_with_word(s, "type");
	}
	if (pending_type == WB_PENDING_BLOCK_DEFAULTS)
	{
		return starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "opacity") || starts_with_word(s, "o") ||
			starts_with_word(s, "jitter");
	}
	if (pending_type == WB_PENDING_BLOCK_BACKGROUND)
	{
		return starts_with_word(s, "radial") ||
			starts_with_word(s, "paper") ||
			starts_with_word(s, "center") ||
			starts_with_word(s, "edge");
	}
	if (pending_type == WB_PENDING_BLOCK_TRANSITION)
	{
		return starts_with_word(s, "type") ||
			starts_with_word(s, "duration");
	}
	if (pending_type == WB_PENDING_BLOCK_RAY)
		return starts_with_word(s, "from") || starts_with_word(s, "through") ||
			looks_like_vec2_arrow(s) ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_DOTTED || pending_type == WB_PENDING_BLOCK_DASHED)
		return starts_with_word(s, "from") || starts_with_word(s, "to") || looks_like_vec2_arrow(s) ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "gap") || starts_with_word(s, "g") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_ARROW)
		return starts_with_word(s, "from") || starts_with_word(s, "to") || looks_like_vec2_arrow(s) ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "head") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_TRIANGLE || pending_type == WB_PENDING_BLOCK_SHADE_TRIANGLE)
		return starts_with_word(s, "points") || count_vec2_literals(s) == 3 ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_ELLIPSE)
		return starts_with_word(s, "center") || starts_with_word(s, "at") || starts_with_word(s, "@") || looks_like_vec2_literal(s) ||
			starts_with_word(s, "radii") || starts_with_word(s, "rx") || starts_with_word(s, "ry") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_SHADE_DISC)
		return starts_with_word(s, "center") || starts_with_word(s, "at") || starts_with_word(s, "@") || looks_like_vec2_literal(s) ||
			starts_with_word(s, "radius") || starts_with_word(s, "r") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_QUAD)
		return starts_with_word(s, "points") || count_vec2_literals(s) == 4 ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_POLYGON || pending_type == WB_PENDING_BLOCK_SHADE_POLYGON)
		return starts_with_word(s, "points") || count_vec2_literals(s) >= 3 ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_BLOB || pending_type == WB_PENDING_BLOCK_SHADE_BLOB)
		return starts_with_word(s, "points") || count_vec2_literals(s) >= 3 ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_LINE3D)
		return starts_with_word(s, "from") || starts_with_word(s, "to") || looks_like_vec3_arrow(s) ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_CURVE3D)
		return starts_with_word(s, "through") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_WIRE3D)
		return starts_with_word(s, "points") || count_vec3_literals(s) >= 3 ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_SHADE_POLY3D)
		return starts_with_word(s, "points") || count_vec3_literals(s) >= 3 ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_SURFACE3D)
		return starts_with_word(s, "points") || count_vec3_literals(s) == 4 ||
			starts_with_word(s, "u_steps") || starts_with_word(s, "u") ||
			starts_with_word(s, "v_steps") || starts_with_word(s, "v") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_MESH3D)
		return starts_with_word(s, "vertices") || starts_with_word(s, "verts") ||
			starts_with_word(s, "faces") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_BLOB3D)
		return starts_with_word(s, "center") || starts_with_word(s, "at") || looks_like_vec3_literal(s) ||
			starts_with_word(s, "radius") || starts_with_word(s, "r") ||
			starts_with_word(s, "radii") || starts_with_word(s, "rx") || starts_with_word(s, "ry") || starts_with_word(s, "rz") ||
			starts_with_word(s, "u_steps") || starts_with_word(s, "u") ||
			starts_with_word(s, "v_steps") || starts_with_word(s, "v") ||
			starts_with_word(s, "wobble") || starts_with_word(s, "w") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_PARAM3D)
		return starts_with_word(s, "family") || starts_with_word(s, "type") ||
			starts_with_word(s, "center") || starts_with_word(s, "at") || looks_like_vec3_literal(s) ||
			starts_with_word(s, "radius") || starts_with_word(s, "r") ||
			starts_with_word(s, "radii") || starts_with_word(s, "rx") || starts_with_word(s, "ry") ||
			starts_with_word(s, "height") || starts_with_word(s, "pitch") ||
			starts_with_word(s, "turns") || starts_with_word(s, "freq") ||
			starts_with_word(s, "phase") || starts_with_word(s, "steps") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_PARAM_SURFACE3D)
		return starts_with_word(s, "family") || starts_with_word(s, "type") ||
			starts_with_word(s, "center") || starts_with_word(s, "at") || looks_like_vec3_literal(s) ||
			starts_with_word(s, "radius") || starts_with_word(s, "r") ||
			starts_with_word(s, "radii") || starts_with_word(s, "rx") || starts_with_word(s, "ry") || starts_with_word(s, "rz") ||
			starts_with_word(s, "major") || starts_with_word(s, "minor") ||
			starts_with_word(s, "height") || starts_with_word(s, "amp") ||
			starts_with_word(s, "freq") || starts_with_word(s, "phase") ||
			starts_with_word(s, "u_steps") || starts_with_word(s, "u") ||
			starts_with_word(s, "v_steps") || starts_with_word(s, "v") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_VOLUME3D)
		return starts_with_word(s, "center") || starts_with_word(s, "at") || looks_like_vec3_literal(s) ||
			starts_with_word(s, "radius") || starts_with_word(s, "r") ||
			starts_with_word(s, "radii") || starts_with_word(s, "rx") || starts_with_word(s, "ry") || starts_with_word(s, "rz") ||
			starts_with_word(s, "shells") || starts_with_word(s, "density") ||
			starts_with_word(s, "u_steps") || starts_with_word(s, "u") ||
			starts_with_word(s, "v_steps") || starts_with_word(s, "v") ||
			starts_with_word(s, "wobble") || starts_with_word(s, "w") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_POINT3D || pending_type == WB_PENDING_BLOCK_OPEN_POINT3D)
		return starts_with_word(s, "at") || starts_with_word(s, "@") || looks_like_vec3_literal(s) ||
			starts_with_word(s, "radius") || starts_with_word(s, "r") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_TRIANGLE3D || pending_type == WB_PENDING_BLOCK_SHADE_TRIANGLE3D)
		return starts_with_word(s, "points") || count_vec3_literals(s) == 3 ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_AXES3D)
		return starts_with_word(s, "at") || starts_with_word(s, "origin") || looks_like_vec3_literal(s) ||
			starts_with_word(s, "length") || starts_with_word(s, "len") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_TETRA3D)
		return starts_with_word(s, "points") || count_vec3_literals(s) == 4 ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") || starts_with_word(s, "a") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_CUBE3D)
		return starts_with_word(s, "center") || starts_with_word(s, "at") || looks_like_vec3_literal(s) ||
			starts_with_word(s, "size") || starts_with_word(s, "s") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "opacity") || starts_with_word(s, "a") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_LINE)
		return starts_with_word(s, "from") || starts_with_word(s, "to") || looks_like_vec2_arrow(s) ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_POINT || pending_type == WB_PENDING_BLOCK_OPEN_POINT)
		return starts_with_word(s, "at") || starts_with_word(s, "@") || looks_like_vec2_literal(s) ||
			starts_with_word(s, "radius") || starts_with_word(s, "r") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_CIRCLE)
		return starts_with_word(s, "center") || starts_with_word(s, "at") || starts_with_word(s, "@") || looks_like_vec2_literal(s) ||
			starts_with_word(s, "radius") || starts_with_word(s, "r") ||
			starts_with_word(s, "thickness") || starts_with_word(s, "t") ||
			starts_with_word(s, "colour") || starts_with_word(s, "color") || starts_with_word(s, "c") ||
			starts_with_word(s, "jitter");
	if (pending_type == WB_PENDING_BLOCK_DRAW)
		return starts_with_word(s, "during") || looks_like_time_range(s);
	if (pending_type == WB_PENDING_BLOCK_FADE)
		return starts_with_word(s, "from") || starts_with_word(s, "to") || starts_with_word(s, "during") ||
			looks_like_time_range(s) || looks_like_scalar_arrow(s);
	if (pending_type == WB_PENDING_BLOCK_MOVE_PATCH)
		return starts_with_word(s, "from") || starts_with_word(s, "to") || starts_with_word(s, "during") ||
			looks_like_vec2_arrow(s) || looks_like_vec3_arrow(s) ||
			looks_like_time_range(s);
	if (pending_type == WB_PENDING_BLOCK_TURN_PATCH)
		return starts_with_word(s, "from") || starts_with_word(s, "to") || starts_with_word(s, "during") ||
			looks_like_time_range(s) || looks_like_scalar_arrow(s) || looks_like_vec3_arrow(s);
	if (pending_type == WB_PENDING_BLOCK_SCALE_PATCH)
		return starts_with_word(s, "from") || starts_with_word(s, "to") || starts_with_word(s, "during") ||
			looks_like_time_range(s) || looks_like_scalar_arrow(s) || looks_like_vec2_arrow(s) || looks_like_vec3_arrow(s);
	if (pending_type == WB_PENDING_BLOCK_TURN)
		return starts_with_word(s, "around") || starts_with_word(s, "from") || starts_with_word(s, "to") || starts_with_word(s, "during") ||
			looks_like_time_range(s) || looks_like_scalar_arrow(s) || looks_like_vec3_arrow(s);
	if (pending_type == WB_PENDING_BLOCK_SCALE)
		return starts_with_word(s, "around") || starts_with_word(s, "from") || starts_with_word(s, "to") || starts_with_word(s, "during") ||
			looks_like_time_range(s) || looks_like_scalar_arrow(s) || looks_like_vec2_arrow(s) || looks_like_vec3_arrow(s);
	if (pending_type == WB_PENDING_BLOCK_MOVE)
		return starts_with_word(s, "from") || starts_with_word(s, "to") || starts_with_word(s, "during") ||
			looks_like_vec2_arrow(s) || looks_like_vec3_arrow(s) ||
			looks_like_time_range(s);
	return 0;
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

static void clear_active_defaults(wb_spec_parser *p);

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
	p->n_groups = 0;
	p->n_layers = 0;
	p->n_group_scopes = 0;
	p->n_patch_scopes = 0;
	p->n_patch_defs = 0;
	clear_active_defaults(p);
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

static void clear_active_defaults(wb_spec_parser *p)
{
	if (!p)
		return;
	p->n_default_scopes = 0;
}

static const char *effective_default_colour_name(wb_spec_parser *p, const char *fallback)
{
	if (p && p->n_default_scopes > 0)
	{
		wb_spec_defaults_scope *scope = &p->default_scopes[p->n_default_scopes - 1];
		if (scope->colour_set && scope->colour_name[0])
			return scope->colour_name;
	}
	return fallback;
}

static float effective_default_thickness(wb_spec_parser *p, float fallback)
{
	if (p && p->n_default_scopes > 0)
	{
		wb_spec_defaults_scope *scope = &p->default_scopes[p->n_default_scopes - 1];
		if (scope->thickness_set)
			return scope->thickness;
	}
	return fallback;
}

static float effective_default_opacity(wb_spec_parser *p, float fallback)
{
	if (p && p->n_default_scopes > 0)
	{
		wb_spec_defaults_scope *scope = &p->default_scopes[p->n_default_scopes - 1];
		if (scope->opacity_set)
			return scope->opacity;
	}
	return fallback;
}

static void apply_default_object_jitter(wb_spec_parser *p, char *line, int object_id)
{
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	
	if (!p || !line || object_id <= 0)
		return;
	if (parse_jitter_token(line, &jitter_strength))
	{
		wb_scene_set_object_jitter(p->scene, object_id, jitter_strength);
		return;
	}
	if (p && p->n_default_scopes > 0)
	{
		wb_spec_defaults_scope *scope = &p->default_scopes[p->n_default_scopes - 1];
		if (scope->jitter_set)
			wb_scene_set_object_jitter(p->scene, object_id, scope->jitter_strength);
	}
}

static float effective_default_jitter(wb_spec_parser *p, float fallback)
{
	if (p && p->n_default_scopes > 0)
	{
		wb_spec_defaults_scope *scope = &p->default_scopes[p->n_default_scopes - 1];
		if (scope->jitter_set)
			return scope->jitter_strength;
	}
	return fallback;
}

static void remember_name(wb_spec_parser *p, const char *name, int id)
{
	if (!name || !*name || id <= 0 || p->n_names >= (int)(sizeof(p->names) / sizeof(p->names[0])))
		return;
	snprintf(p->names[p->n_names].name, sizeof(p->names[p->n_names].name), "%s", name);
	p->names[p->n_names].id = id;
	p->n_names++;
}

static const char *ensure_object_name(wb_spec_parser *p, char *name, size_t cap, const char *prefix)
{
	static int anonymous_object_counter = 1;
	
	if (!name || cap == 0)
		return "";
	if (name[0] != 0)
		return name;
	if (!prefix || !*prefix)
		prefix = "obj";
	snprintf(name, cap, "__%s_%d", prefix, anonymous_object_counter++);
	return name;
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

static wb_spec_group *find_group(wb_spec_parser *p, const char *name)
{
	if (!p || !name)
		return NULL;
	
	for (int i = 0; i < p->n_groups; i++)
	{
		if (strcmp(p->groups[i].name, name) == 0)
			return &p->groups[i];
	}
	return NULL;
}

static int scene_layer_type_by_id(const wb_scene *scene, int layer_id)
{
	if (!scene)
		return WB_LAYER_2D;
	(void)layer_id;
	return WB_LAYER_2D;
}

static int scene_object_is_3d(const wb_scene *scene, int object_id)
{
	if (!scene || object_id <= 0)
		return 0;
	for (int i = 0; i < scene->n_objects; i++)
	{
		if (scene->objects[i].id == object_id)
		{
			wb_scene_patch *patch = wb_scene_find_patch((wb_scene *)scene, scene->objects[i].patch_id);
			return patch && patch->dimension == WB_LAYER_3D;
		}
	}
	return 0;
}

static int group_is_3d(const wb_spec_parser *p, const wb_spec_group *group)
{
	if (!p || !group || group->n_ids <= 0)
		return 0;
	for (int i = 0; i < group->n_ids; i++)
	{
		if (scene_object_is_3d(p->scene, group->ids[i]))
			return 1;
	}
	return 0;
}

static void remember_group_member(wb_spec_parser *p, const char *group_name, int id)
{
	wb_spec_group *group;
	
	if (!p || !group_name || !*group_name || id <= 0)
		return;
	
	group = find_group(p, group_name);
	if (!group)
	{
		if (p->n_groups >= (int)(sizeof(p->groups) / sizeof(p->groups[0])))
			return;
		group = &p->groups[p->n_groups++];
		memset(group, 0, sizeof(*group));
		snprintf(group->name, sizeof(group->name), "%s", group_name);
	}
	
	for (int i = 0; i < group->n_ids; i++)
	{
		if (group->ids[i] == id)
			return;
	}
	if (group->n_ids < (int)(sizeof(group->ids) / sizeof(group->ids[0])))
		group->ids[group->n_ids++] = id;
}

static void remember_patch_def(wb_spec_parser *p, const char *name)
{
	wb_spec_patch_def *def = NULL;
	
	if (!p || !name || !*name)
		return;
	if (p->n_patch_scopes > 0 && strcmp(p->patch_scopes[p->n_patch_scopes - 1].name, name) == 0)
		sync_retained_patch_scope(p, &p->patch_scopes[p->n_patch_scopes - 1]);
	for (int i = 0; i < p->n_patch_defs; i++)
	{
		if (strcmp(p->patch_defs[i].name, name) == 0)
		{
			def = &p->patch_defs[i];
			break;
		}
	}
	if (!def)
	{
		if (p->n_patch_defs >= (int)(sizeof(p->patch_defs) / sizeof(p->patch_defs[0])))
			return;
		def = &p->patch_defs[p->n_patch_defs++];
		memset(def, 0, sizeof(*def));
		snprintf(def->name, sizeof(def->name), "%s", name);
	}
	def->n_scopes = p->n_patch_scopes;
	if (def->n_scopes > (int)(sizeof(def->scopes) / sizeof(def->scopes[0])))
		def->n_scopes = (int)(sizeof(def->scopes) / sizeof(def->scopes[0]));
	for (int i = 0; i < def->n_scopes; i++)
		def->scopes[i] = p->patch_scopes[i];
}

static wb_spec_patch_def *find_patch_def(wb_spec_parser *p, const char *name)
{
	if (!p || !name)
		return NULL;
	for (int i = 0; i < p->n_patch_defs; i++)
	{
		if (strcmp(p->patch_defs[i].name, name) == 0)
			return &p->patch_defs[i];
	}
	return NULL;
}

static void pop_finished_group_scopes(wb_spec_parser *p, int raw_indent)
{
	if (!p)
		return;
	while (p->n_group_scopes > 0 && raw_indent <= p->group_scopes[p->n_group_scopes - 1].indent)
		p->n_group_scopes--;
}

static void pop_finished_patch_scopes(wb_spec_parser *p, int raw_indent)
{

	if (!p)
		return;
	while (p->n_patch_scopes > 0 && raw_indent <= p->patch_scopes[p->n_patch_scopes - 1].indent)
	{
		p->n_patch_scopes--;
	}
	if (p->scene)
	{
		int patch_id = p->n_patch_scopes > 0 ? p->patch_scopes[p->n_patch_scopes - 1].patch_id : p->scene->root_patch_id;
		wb_scene_set_current_patch(p->scene, patch_id);
		(void)patch_id;
	}
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
		sscanf(line, "scene \"%127[^\"]\" %fs", title, &duration) == 2 ||
		sscanf(line, "scene duration %fs", &duration) == 1)
	{
		if (!start_new_scene(p, duration))
			return set_error(p, line_no, "failed to create scene");
		return 1;
	}
	if (strcmp(line, "scene") == 0 || sscanf(line, "scene \"%127[^\"]\"", title) == 1)
	{
		if (!start_new_scene(p, 0.0f))
			return set_error(p, line_no, "failed to create scene");
		return 1;
	}
	return set_error(p, line_no, "expected scene \"title\" duration Ns");
}

static int parse_video(wb_spec_parser *p, char *line, int line_no)
{
	char output_path[256];
	int fps = 0;
	int jitter_fps = 0;
	char *fps_prop = NULL;
	char *jitter_prop = NULL;
	char *output_prop = NULL;
	
	if (sscanf(line, "video%*[^\"]\"%255[^\"]\"", output_path) == 1)
	{
		if (!is_safe_output_path(output_path))
			return set_error(p, line_no, "video output path may only contain letters, digits, '/', '.', '_' and '-'");
		snprintf(p->output_path, sizeof(p->output_path), "%s", output_path);
		return 1;
	}
	if (strcmp(line, "video") == 0)
		return 1;
	
	fps_prop = strstr(line, " fps ");
	jitter_prop = strstr(line, " jitter_fps ");
	output_prop = strstr(line, " output ");
	if (fps_prop)
		sscanf(fps_prop, " fps %d", &fps);
	if (jitter_prop)
		sscanf(jitter_prop, " jitter_fps %d", &jitter_fps);
	if (output_prop && sscanf(output_prop, " output \"%255[^\"]\"", output_path) == 1)
	{
		if (!is_safe_output_path(output_path))
			return set_error(p, line_no, "video output path may only contain letters, digits, '/', '.', '_' and '-'");
		snprintf(p->output_path, sizeof(p->output_path), "%s", output_path);
	}
	if (fps_prop || jitter_prop || output_prop)
		return 1;
	
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
	{
		char *type_prop = strstr(line, " type ");
		char *duration_prop = strstr(line, " duration ");
		if (type_prop)
			sscanf(type_prop, " type %31s", type_name);
		if (duration_prop)
			sscanf(duration_prop, " duration %f", &duration);
		if (!type_prop || !duration_prop)
			return set_error(p, line_no, "expected transition fade|crossfade Ns");
	}
	
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
	char name[64] = "";
	char latex[512];
	char colour_name[64];
	float x = 0.0f, y = 0.0f, size = WB_DEFAULT_MATH_SIZE;
	float thickness = effective_default_thickness(p, WB_DEFAULT_MATH_THICKNESS);
	int n = 0;
	int id = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	if (sscanf(line, "math %63s \"%511[^\"]\" at (%f,%f) size %f colour %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 5 ||
		sscanf(line, "math %63s \"%511[^\"]\" at (%f, %f) size %f colour %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 5 ||
		sscanf(line, "math %63s \"%511[^\"]\" @ (%f,%f) s %f c %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 4 ||
		sscanf(line, "math %63s \"%511[^\"]\" @ (%f, %f) s %f c %63s%n", name, latex, &x, &y, &size, colour_name, &n) >= 4)
	{
		goto have_math;
	}
	if (sscanf(line, "math %63s \"%511[^\"]\" at (%f,%f)", name, latex, &x, &y) == 4 ||
		sscanf(line, "math %63s \"%511[^\"]\" at (%f, %f)", name, latex, &x, &y) == 4 ||
		sscanf(line, "math %63s \"%511[^\"]\" @ (%f,%f)", name, latex, &x, &y) == 4 ||
		sscanf(line, "math %63s \"%511[^\"]\" @ (%f, %f)", name, latex, &x, &y) == 4)
		goto have_math;
	if (sscanf(line, "math \"%511[^\"]\" at (%f,%f) size %f colour %63s%n", latex, &x, &y, &size, colour_name, &n) >= 4 ||
		sscanf(line, "math \"%511[^\"]\" at (%f, %f) size %f colour %63s%n", latex, &x, &y, &size, colour_name, &n) >= 4 ||
		sscanf(line, "math \"%511[^\"]\" @ (%f,%f) s %f c %63s%n", latex, &x, &y, &size, colour_name, &n) >= 3 ||
		sscanf(line, "math \"%511[^\"]\" @ (%f, %f) s %f c %63s%n", latex, &x, &y, &size, colour_name, &n) >= 3)
		goto have_math;
	if (sscanf(line, "math \"%511[^\"]\" at (%f,%f)", latex, &x, &y) == 3 ||
		sscanf(line, "math \"%511[^\"]\" at (%f, %f)", latex, &x, &y) == 3 ||
		sscanf(line, "math \"%511[^\"]\" @ (%f,%f)", latex, &x, &y) == 3 ||
		sscanf(line, "math \"%511[^\"]\" @ (%f, %f)", latex, &x, &y) == 3)
		goto have_math;
	
	return set_error(p, line_no, "expected math [name] \"$...$\" at (x,y) size N colour name");

have_math:
	if (sscanf(line + n, " thickness %f", &thickness) != 1)
	{
		char *t = strstr(line, " thickness ");
		if (t)
			sscanf(t, " thickness %f", &thickness);
	}
	id = wb_scene_add_math(p->scene, latex, x, y, size, parse_colour(colour_name));
	if (!id)
		return set_error(p, line_no, "failed to create math object");
	p->scene->objects[p->scene->n_objects - 1].thickness = thickness;
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_text(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char text[512];
	char colour_name[64];
	float x = 0.0f, y = 0.0f, size = WB_DEFAULT_MATH_SIZE;
	int id = 0;
	
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	if (sscanf(line, "text %63s \"%511[^\"]\" at (%f,%f) size %f colour %63s", name, text, &x, &y, &size, colour_name) >= 5 ||
		sscanf(line, "text %63s \"%511[^\"]\" at (%f, %f) size %f colour %63s", name, text, &x, &y, &size, colour_name) >= 5 ||
		sscanf(line, "text %63s \"%511[^\"]\" @ (%f,%f) s %f c %63s", name, text, &x, &y, &size, colour_name) >= 4 ||
		sscanf(line, "text %63s \"%511[^\"]\" @ (%f, %f) s %f c %63s", name, text, &x, &y, &size, colour_name) >= 4 ||
		sscanf(line, "text %63s \"%511[^\"]\" at (%f,%f)", name, text, &x, &y) == 4 ||
		sscanf(line, "text %63s \"%511[^\"]\" at (%f, %f)", name, text, &x, &y) == 4 ||
		sscanf(line, "text %63s \"%511[^\"]\" @ (%f,%f)", name, text, &x, &y) == 4 ||
		sscanf(line, "text %63s \"%511[^\"]\" @ (%f, %f)", name, text, &x, &y) == 4)
	{
	}
	else if (sscanf(line, "text \"%511[^\"]\" at (%f,%f) size %f colour %63s", text, &x, &y, &size, colour_name) >= 4 ||
		sscanf(line, "text \"%511[^\"]\" at (%f, %f) size %f colour %63s", text, &x, &y, &size, colour_name) >= 4 ||
		sscanf(line, "text \"%511[^\"]\" @ (%f,%f) s %f c %63s", text, &x, &y, &size, colour_name) >= 3 ||
		sscanf(line, "text \"%511[^\"]\" @ (%f, %f) s %f c %63s", text, &x, &y, &size, colour_name) >= 3 ||
		sscanf(line, "text \"%511[^\"]\" at (%f,%f)", text, &x, &y) == 3 ||
		sscanf(line, "text \"%511[^\"]\" at (%f, %f)", text, &x, &y) == 3 ||
		sscanf(line, "text \"%511[^\"]\" @ (%f,%f)", text, &x, &y) == 3 ||
		sscanf(line, "text \"%511[^\"]\" @ (%f, %f)", text, &x, &y) == 3)
	{
	}
	else
		return set_error(p, line_no, "expected text [name] \"...\" at (x,y) size N colour name");
	
	if (strstr(line, " size "))
		sscanf(strstr(line, " size "), " size %f", &size);
	else if (strstr(line, " s "))
		sscanf(strstr(line, " s "), " s %f", &size);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	id = wb_scene_add_text(p->scene, text, x, y, size, parse_colour(colour_name));
	if (!id)
		return set_error(p, line_no, "failed to create text object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_curve_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	int matched = 0;
	
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "curve %63s through (%f,%f) (%f,%f) (%f,%f) thickness %f colour %63s",
		name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "curve %63s through (%f, %f) (%f, %f) (%f, %f) thickness %f colour %63s",
			name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7 && sscanf(line, "curve %63s", name) == 1)
	{
		char *through = strstr(line, " through ");
		if (through &&
			(sscanf(through, " through (%f,%f) (%f,%f) (%f,%f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6 ||
			 sscanf(through, " through (%f, %f) (%f, %f) (%f, %f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6))
			matched = 7;
	}
	if (matched < 7 && (strcmp(line, "curve") == 0 || starts_with(line, "curve through ")))
	{
		char *through = strstr(line, " through ");
		if (through &&
			(sscanf(through, " through (%f,%f) (%f,%f) (%f,%f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6 ||
			 sscanf(through, " through (%f, %f) (%f, %f) (%f, %f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6))
			matched = 7;
	}
	if (matched < 7)
		return set_error(p, line_no, "expected curve [name] through (x,y) (x,y) (x,y) thickness N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_curve(p->scene, x0, y0, x1, y1, x2, y2, thickness, parse_colour(colour_name));
	if (!id)
		return set_error(p, line_no, "failed to create curve object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_background(wb_spec_parser *p, char *line, int line_no)
{
	char center[64] = "blue";
	char edge[64] = "blue";
	int paper = 0;

	if (strcmp(line, "background") == 0 || strcmp(line, "background radial") == 0)
	{
		wb_scene_set_radial_background(
			p->scene,
			WB_DEFAULT_BACKGROUND_CENTER_COLOUR,
			WB_DEFAULT_BACKGROUND_EDGE_COLOUR);
		return 1;
	}
	if (strcmp(line, "background paper") == 0)
	{
		wb_scene_set_paper_background(
			p->scene,
			WB_DEFAULT_BACKGROUND_CENTER_COLOUR,
			WB_DEFAULT_BACKGROUND_EDGE_COLOUR);
		return 1;
	}
	
	if (sscanf(line, "background radial center %63s edge %63s", center, edge) == 2)
	{
		wb_scene_set_radial_background(p->scene, parse_colour(center), parse_colour(edge));
		return 1;
	}
	if (sscanf(line, "background paper center %63s edge %63s", center, edge) == 2)
	{
		wb_scene_set_paper_background(p->scene, parse_colour(center), parse_colour(edge));
		return 1;
	}
	if (starts_with(line, "background radial"))
	{
		char *center_prop = strstr(line, " center ");
		char *edge_prop = strstr(line, " edge ");
		if (center_prop)
			sscanf(center_prop, " center %63s", center);
		if (edge_prop)
			sscanf(edge_prop, " edge %63s", edge);
		if (!center_prop && !edge_prop)
		{
			wb_scene_set_radial_background(
				p->scene,
				WB_DEFAULT_BACKGROUND_CENTER_COLOUR,
				WB_DEFAULT_BACKGROUND_EDGE_COLOUR);
			return 1;
		}
		if (center_prop && edge_prop)
		{
			wb_scene_set_radial_background(p->scene, parse_colour(center), parse_colour(edge));
			return 1;
		}
	}
	if (starts_with(line, "background paper"))
	{
		char *center_prop = strstr(line, " center ");
		char *edge_prop = strstr(line, " edge ");
		paper = 1;
		if (center_prop)
			sscanf(center_prop, " center %63s", center);
		if (edge_prop)
			sscanf(edge_prop, " edge %63s", edge);
		if (!center_prop && !edge_prop)
		{
			wb_scene_set_paper_background(
				p->scene,
				WB_DEFAULT_BACKGROUND_CENTER_COLOUR,
				WB_DEFAULT_BACKGROUND_EDGE_COLOUR);
			return 1;
		}
		if (center_prop && edge_prop)
		{
			wb_scene_set_paper_background(p->scene, parse_colour(center), parse_colour(edge));
			return 1;
		}
	}
	
	return set_error(p, line_no, paper ? "expected background paper [center colour edge colour]" : "expected background [radial|paper] [center colour edge colour]");
}

static int parse_layer(wb_spec_parser *p, char *line, int line_no)
{
	(void)line;
	return set_error(p, line_no, "layers have been replaced by patches; use patch for 2d or space for 3d");
	/* Legacy implementation retained below temporarily while its runtime
	 * helpers are removed in the next Manifold pass. */
	#if 0
	char name[64] = "";
	char type_name[32] = "2d";
	char opacity_word[32];
	float opacity = WB_DEFAULT_LAYER_OPACITY;
	float blur_radius = WB_DEFAULT_LAYER_BLUR_RADIUS;
	float glow_radius = WB_DEFAULT_LAYER_GLOW_RADIUS;
	float glow_opacity = WB_DEFAULT_LAYER_GLOW_OPACITY;
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
		matched = sscanf(line, "layer %63s", name);
	if (matched < 1)
	{
		if (strcmp(line, "layer") == 0)
			matched = 0;
		else if (starts_with(line, "layer type ") || starts_with(line, "layer opacity ") ||
			starts_with(line, "layer o ") || starts_with(line, "layer blur ") ||
			starts_with(line, "layer glow ") || starts_with(line, "layer glow_opacity ") ||
			starts_with(line, "layer jitter "))
			matched = 0;
		else
			return set_error(p, line_no, "expected layer [name] [2d|3d] [opacity N]");
	}
	if (matched == 1 &&
		(strcmp(name, "2d") == 0 || strcmp(name, "3d") == 0 ||
		 strcmp(name, "type") == 0 || strcmp(name, "opacity") == 0 ||
		 strcmp(name, "o") == 0 || strcmp(name, "blur") == 0 ||
		 strcmp(name, "glow") == 0 || strcmp(name, "glow_opacity") == 0 ||
		 strcmp(name, "jitter") == 0))
	{
		snprintf(type_name, sizeof(type_name), "%s", name);
		name[0] = 0;
		matched = 0;
	}
	if (matched >= 2 &&
		(strcmp(type_name, "opacity") == 0 || strcmp(type_name, "o") == 0 ||
		 strcmp(type_name, "blur") == 0 || strcmp(type_name, "glow") == 0 ||
		 strcmp(type_name, "glow_opacity") == 0 || strcmp(type_name, "jitter") == 0))
	{
		matched = 1;
		snprintf(type_name, sizeof(type_name), "2d");
	}
	
	if (strcmp(type_name, "3d") == 0)
		type = WB_LAYER_3D;
	else if (strcmp(type_name, "2d") == 0)
		type = WB_LAYER_2D;
	else if (strcmp(type_name, "type") == 0)
		type = WB_LAYER_2D;
	else if (matched == 0)
		type = WB_LAYER_2D;
	else if (matched == 1)
		type = WB_LAYER_2D;
	else if (sscanf(line, "layer %63s opacity %f", name, &opacity) == 2)
		type = WB_LAYER_2D;
	else if (sscanf(line, "layer %63s %31s", name, opacity_word) == 2 && strcmp(opacity_word, "opacity") == 0)
		return set_error(p, line_no, "expected opacity value after layer opacity");
	else
		return set_error(p, line_no, "layer type must be 2d or 3d");
	
	if (strstr(line, " type 3d"))
		type = WB_LAYER_3D;
	else if (strstr(line, " type 2d"))
		type = WB_LAYER_2D;
	else if (starts_with(line, "layer 3d"))
		type = WB_LAYER_3D;
	else if (starts_with(line, "layer 2d"))
		type = WB_LAYER_2D;
	
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	ensure_object_name(p, name, sizeof(name), "layer");
	id = wb_scene_add_layer(p->scene, name, type, opacity);
	if (!id)
		return set_error(p, line_no, "failed to create layer");
	
	char *blur = strstr(line, " blur ");
	if (blur && sscanf(blur, " blur %f", &blur_radius) == 1)
		wb_scene_set_layer_blur(p->scene, id, blur_radius);
	if (strstr(line, " glow "))
		sscanf(strstr(line, " glow "), " glow %f", &glow_radius);
	if (strstr(line, " glow_opacity "))
		sscanf(strstr(line, " glow_opacity "), " glow_opacity %f", &glow_opacity);
	if (glow_radius > 0.0f || strstr(line, " glow_opacity "))
		wb_scene_set_layer_glow(p->scene, id, glow_radius, glow_opacity);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	else if (strstr(line, " o "))
		sscanf(strstr(line, " o "), " o %f", &opacity);
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	p->scene->render_contexts[p->scene->n_render_contexts - 1].opacity = opacity;
	p->scene->render_contexts[p->scene->n_render_contexts - 1].render_opacity = opacity;
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_layer_jitter(p->scene, id, jitter_strength);
	
	remember_layer(p, name, id);
	return 1;
	#endif
}

/* `space` is the progressive-disclosure spelling for a camera-backed 3D
 * patch.  The compositor still uses the existing surface implementation while
 * retained patch traversal is completed. */
static int parse_space(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char expanded[256];

	if (sscanf(line, "space %63s", name) != 1)
		return set_error(p, line_no, "expected space name");
	snprintf(expanded, sizeof(expanded), "patch %s 3d", name);
	return parse_patch(p, expanded, line_no);
}

static void sync_retained_patch_scope(wb_spec_parser *p, const wb_spec_patch_scope *scope)
{
	if (!p || !p->scene || !scope || scope->patch_id <= 0)
		return;
	wb_scene_set_patch_transform(p->scene, scope->patch_id,
		scope->origin, scope->scale, scope->rotation,
		scope->origin3, scope->scale3, scope->rotation3);
}

static int parse_patch(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char coord_name[32] = "cartesian";
	float ox = 0.0f, oy = 0.0f;
	float oz = 0.0f;
	float sx = 1.0f, sy = 1.0f;
	float sz = 1.0f;
	float rotation = 0.0f;
	float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
	float opacity = 1.0f;
	float blur_radius = 0.0f;
	float glow_radius = 0.0f;
	float glow_opacity = WB_DEFAULT_LAYER_GLOW_OPACITY;
	float jitter_strength = WB_DEFAULT_LAYER_JITTER_STRENGTH;
	int jitter_explicit = 0;
	int have_origin = 0;
	int have_scale = 0;
	int layer_type;
	wb_spec_patch_scope scope;
	
	if (!p || !p->scene)
		return set_error(p, line_no, "patch must appear inside a scene");
	layer_type = strstr(line, " 3d") ? WB_LAYER_3D : current_layer_type(p);
	if (layer_type != WB_LAYER_2D && layer_type != WB_LAYER_3D)
		return set_error(p, line_no, "patch must appear inside a 2d or 3d layer");
	if (p->n_patch_scopes >= (int)(sizeof(p->patch_scopes) / sizeof(p->patch_scopes[0])))
		return set_error(p, line_no, "too many nested patches");
	if (sscanf(line, "patch %63s", name) != 1 && strcmp(line, "patch") != 0)
		return set_error(p, line_no, "expected patch [name]");
	if (strstr(line, " at "))
	{
		if (layer_type == WB_LAYER_3D)
		{
			if (sscanf(strstr(line, " at "), " at (%f,%f,%f)", &ox, &oy, &oz) == 3 ||
				sscanf(strstr(line, " at "), " at (%f, %f, %f)", &ox, &oy, &oz) == 3)
				have_origin = 1;
		}
		else if (sscanf(strstr(line, " at "), " at (%f,%f)", &ox, &oy) == 2 ||
			sscanf(strstr(line, " at "), " at (%f, %f)", &ox, &oy) == 2)
			have_origin = 1;
	}
	else if (strstr(line, " origin "))
	{
		if (layer_type == WB_LAYER_3D)
		{
			if (sscanf(strstr(line, " origin "), " origin (%f,%f,%f)", &ox, &oy, &oz) == 3 ||
				sscanf(strstr(line, " origin "), " origin (%f, %f, %f)", &ox, &oy, &oz) == 3)
				have_origin = 1;
		}
		else if (sscanf(strstr(line, " origin "), " origin (%f,%f)", &ox, &oy) == 2 ||
			sscanf(strstr(line, " origin "), " origin (%f, %f)", &ox, &oy) == 2)
			have_origin = 1;
	}
	else if (strstr(line, " @ "))
	{
		if (layer_type == WB_LAYER_3D)
		{
			if (sscanf(strstr(line, " @ "), " @ (%f,%f,%f)", &ox, &oy, &oz) == 3 ||
				sscanf(strstr(line, " @ "), " @ (%f, %f, %f)", &ox, &oy, &oz) == 3)
				have_origin = 1;
		}
		else if (sscanf(strstr(line, " @ "), " @ (%f,%f)", &ox, &oy) == 2 ||
			sscanf(strstr(line, " @ "), " @ (%f, %f)", &ox, &oy) == 2)
			have_origin = 1;
	}
	if (strstr(line, " scale "))
	{
		if (layer_type == WB_LAYER_3D &&
			(sscanf(strstr(line, " scale "), " scale (%f,%f,%f)", &sx, &sy, &sz) == 3 ||
			 sscanf(strstr(line, " scale "), " scale (%f, %f, %f)", &sx, &sy, &sz) == 3))
			have_scale = 1;
		else if (sscanf(strstr(line, " scale "), " scale (%f,%f)", &sx, &sy) == 2 ||
			sscanf(strstr(line, " scale "), " scale (%f, %f)", &sx, &sy) == 2)
			have_scale = 1;
		else if (sscanf(strstr(line, " scale "), " scale %f", &sx) == 1)
		{
			sy = sx;
			sz = sx;
			have_scale = 1;
		}
	}
	else if (strstr(line, " s "))
	{
		if (layer_type == WB_LAYER_3D &&
			(sscanf(strstr(line, " s "), " s (%f,%f,%f)", &sx, &sy, &sz) == 3 ||
			 sscanf(strstr(line, " s "), " s (%f, %f, %f)", &sx, &sy, &sz) == 3))
			have_scale = 1;
		else if (sscanf(strstr(line, " s "), " s (%f,%f)", &sx, &sy) == 2 ||
			sscanf(strstr(line, " s "), " s (%f, %f)", &sx, &sy) == 2)
			have_scale = 1;
		else if (sscanf(strstr(line, " s "), " s %f", &sx) == 1)
		{
			sy = sx;
			sz = sx;
			have_scale = 1;
		}
	}
	if (strstr(line, " yaw "))
		sscanf(strstr(line, " yaw "), " yaw %f", &yaw);
	if (strstr(line, " pitch "))
		sscanf(strstr(line, " pitch "), " pitch %f", &pitch);
	if (strstr(line, " roll "))
		sscanf(strstr(line, " roll "), " roll %f", &roll);
	if (layer_type == WB_LAYER_2D && strstr(line, " yaw "))
		rotation = yaw;
	else if (strstr(line, " rotate "))
	{
		if (layer_type == WB_LAYER_3D &&
			(sscanf(strstr(line, " rotate "), " rotate (%f,%f,%f)", &yaw, &pitch, &roll) == 3 ||
			 sscanf(strstr(line, " rotate "), " rotate (%f, %f, %f)", &yaw, &pitch, &roll) == 3))
		{
		}
		else
			sscanf(strstr(line, " rotate "), " rotate %f", &rotation);
	}
	else if (strstr(line, " rotation "))
	{
		if (layer_type == WB_LAYER_3D &&
			(sscanf(strstr(line, " rotation "), " rotation (%f,%f,%f)", &yaw, &pitch, &roll) == 3 ||
			 sscanf(strstr(line, " rotation "), " rotation (%f, %f, %f)", &yaw, &pitch, &roll) == 3))
		{
		}
		else
			sscanf(strstr(line, " rotation "), " rotation %f", &rotation);
	}
	else if (strstr(line, " angle "))
		sscanf(strstr(line, " angle "), " angle %f", &rotation);
	if (layer_type == WB_LAYER_3D && (rotation != 0.0f || strstr(line, " angle ") || strstr(line, " rotate ") || strstr(line, " rotation ")))
		yaw = rotation;
	if (strstr(line, " coords "))
		sscanf(strstr(line, " coords "), " coords %31s", coord_name);
	else if (strstr(line, " coord "))
		sscanf(strstr(line, " coord "), " coord %31s", coord_name);
	else if (strstr(line, " type "))
		sscanf(strstr(line, " type "), " type %31s", coord_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	else if (strstr(line, " o "))
		sscanf(strstr(line, " o "), " o %f", &opacity);
	if (strstr(line, " blur "))
		sscanf(strstr(line, " blur "), " blur %f", &blur_radius);
	if (strstr(line, " glow "))
		sscanf(strstr(line, " glow "), " glow %f", &glow_radius);
	if (strstr(line, " glow_opacity "))
		sscanf(strstr(line, " glow_opacity "), " glow_opacity %f", &glow_opacity);
	jitter_explicit = parse_jitter_token(line, &jitter_strength);
	
	memset(&scope, 0, sizeof(scope));
	snprintf(scope.name, sizeof(scope.name), "%s", name);
	scope.indent = p->current_line_indent;
	scope.dimension = layer_type == WB_LAYER_3D ? 3 : 2;
	scope.coord_type = (strcmp(coord_name, "polar") == 0) ? WB_PATCH_COORD_POLAR : WB_PATCH_COORD_CARTESIAN;
	scope.origin = have_origin ? vec2(ox, oy) : vec2(0, 0);
	scope.scale = have_scale ? vec2(sx, sy) : vec2(1, 1);
	scope.origin3 = have_origin ? vec3(ox, oy, oz) : vec3(0, 0, 0);
	scope.scale3 = have_scale ? vec3(sx, sy, sz) : vec3(1, 1, 1);
	scope.rotation = rotation;
	scope.rotation3 = vec3(yaw, pitch, roll);
	scope.root_manifold = scope.dimension == 2 && p->n_patch_scopes == 0;
	if (scope.root_manifold)
	{
		scope.origin = root_world_to_pixel(p->scene, have_origin ? vec2(ox, oy) : vec2(0, 0));
		scope.scale = vec2((have_scale ? sx : 1.0f) * root_world_pixel_scale(p->scene),
			-(have_scale ? sy : 1.0f) * root_world_pixel_scale(p->scene));
		scope.rotation = -rotation;
	}
	if (scope.dimension == 3)
	{
		scope.rotation = 0.0f;
		if (strcmp(coord_name, "cylindrical") == 0)
			scope.coord_type = WB_PATCH_COORD_CYLINDRICAL;
		else if (strcmp(coord_name, "spherical") == 0)
			scope.coord_type = WB_PATCH_COORD_SPHERICAL;
		else
			scope.coord_type = WB_PATCH_COORD_CARTESIAN;
	}
	scope.patch_id = wb_scene_add_patch(p->scene, scope.name,
		p->scene->current_patch_id, scope.dimension, scope.coord_type);
	if (!scope.patch_id)
		return set_error(p, line_no, "failed to create patch");
	{
		wb_scene_patch *patch = wb_scene_find_patch(p->scene, scope.patch_id);
		if (patch)
		{
			patch->opacity = binary_max(WB_MIN_OPACITY, binary_min(WB_MAX_OPACITY, opacity));
			patch->render_opacity = patch->opacity;
			patch->blur_radius = binary_max(0.0f, binary_min(WB_MAX_LAYER_BLUR_RADIUS, blur_radius));
			patch->glow_radius = binary_max(0.0f, binary_min(WB_MAX_LAYER_BLUR_RADIUS, glow_radius));
			patch->glow_opacity = binary_max(WB_MIN_OPACITY, binary_min(WB_MAX_OPACITY, glow_opacity));
			patch->jitter_strength = binary_max(WB_MIN_JITTER_STRENGTH, jitter_strength);
			patch->jitter_explicit = jitter_explicit;
			patch->render_jitter_strength = patch->jitter_strength;
		}
	}
	sync_retained_patch_scope(p, &scope);
	p->patch_scopes[p->n_patch_scopes++] = scope;
	wb_scene_set_current_patch(p->scene, scope.patch_id);
	if (name[0])
		remember_patch_def(p, name);
	
	if (name[0])
	{
		if (p->n_group_scopes >= (int)(sizeof(p->group_scopes) / sizeof(p->group_scopes[0])))
			return set_error(p, line_no, "too many nested groups");
		snprintf(p->group_scopes[p->n_group_scopes].name, sizeof(p->group_scopes[p->n_group_scopes].name), "%s", name);
		p->group_scopes[p->n_group_scopes].indent = p->current_line_indent;
		p->n_group_scopes++;
	}
	return 1;
}

static int parse_patch_property(wb_spec_parser *p, const char *line, int line_no)
{
	wb_spec_patch_scope *scope;
	float ox = 0.0f, oy = 0.0f;
	float oz = 0.0f;
	float sx = 1.0f, sy = 1.0f;
	float sz = 1.0f;
	float rotation = 0.0f;
	float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
	char coord_name[32] = "";
	int matched = 0;
	
	if (!p || p->n_patch_scopes <= 0 || !line)
		return set_error(p, line_no, "patch property without active patch");
	scope = &p->patch_scopes[p->n_patch_scopes - 1];
	
	if (starts_with_word(line, "at") || starts_with_word(line, "origin") || starts_with_word(line, "@"))
	{
		if (scope->dimension == 3)
			matched =
				sscanf(line, "at (%f,%f,%f)", &ox, &oy, &oz) == 3 ||
				sscanf(line, "at (%f, %f, %f)", &ox, &oy, &oz) == 3 ||
				sscanf(line, "origin (%f,%f,%f)", &ox, &oy, &oz) == 3 ||
				sscanf(line, "origin (%f, %f, %f)", &ox, &oy, &oz) == 3 ||
				sscanf(line, "@ (%f,%f,%f)", &ox, &oy, &oz) == 3 ||
				sscanf(line, "@ (%f, %f, %f)", &ox, &oy, &oz) == 3;
		else
			matched =
				sscanf(line, "at (%f,%f)", &ox, &oy) == 2 ||
				sscanf(line, "at (%f, %f)", &ox, &oy) == 2 ||
				sscanf(line, "origin (%f,%f)", &ox, &oy) == 2 ||
				sscanf(line, "origin (%f, %f)", &ox, &oy) == 2 ||
				sscanf(line, "@ (%f,%f)", &ox, &oy) == 2 ||
				sscanf(line, "@ (%f, %f)", &ox, &oy) == 2;
		if (!matched)
			return set_error(p, line_no, scope->dimension == 3 ? "expected patch at/origin (@) (x,y,z)" : "expected patch at/origin (@) (x,y)");
		scope->origin = scope->root_manifold ? root_world_to_pixel(p->scene, vec2(ox, oy)) : vec2(ox, oy);
		scope->origin3 = vec3(ox, oy, oz);
		if (scope->name[0])
			remember_patch_def(p, scope->name);
		return 1;
	}
	if (starts_with_word(line, "scale") || starts_with_word(line, "s"))
	{
		if (scope->dimension == 3 &&
			(sscanf(line, "scale (%f,%f,%f)", &sx, &sy, &sz) == 3 ||
			 sscanf(line, "scale (%f, %f, %f)", &sx, &sy, &sz) == 3 ||
			 sscanf(line, "s (%f,%f,%f)", &sx, &sy, &sz) == 3 ||
			 sscanf(line, "s (%f, %f, %f)", &sx, &sy, &sz) == 3))
		{
			scope->scale3 = vec3(sx, sy, sz);
			scope->scale = scope->root_manifold ? vec2(sx * root_world_pixel_scale(p->scene), -sy * root_world_pixel_scale(p->scene)) : vec2(sx, sy);
			if (scope->name[0])
				remember_patch_def(p, scope->name);
			return 1;
		}
		if (sscanf(line, "scale (%f,%f)", &sx, &sy) == 2 ||
			sscanf(line, "scale (%f, %f)", &sx, &sy) == 2 ||
			sscanf(line, "s (%f,%f)", &sx, &sy) == 2 ||
			sscanf(line, "s (%f, %f)", &sx, &sy) == 2)
		{
			scope->scale = scope->root_manifold ? vec2(sx * root_world_pixel_scale(p->scene), -sy * root_world_pixel_scale(p->scene)) : vec2(sx, sy);
			if (scope->name[0])
				remember_patch_def(p, scope->name);
			return 1;
		}
		if (sscanf(line, "scale %f", &sx) == 1 || sscanf(line, "s %f", &sx) == 1)
		{
			scope->scale = scope->root_manifold ? vec2(sx * root_world_pixel_scale(p->scene), -sx * root_world_pixel_scale(p->scene)) : vec2(sx, sx);
			scope->scale3 = vec3(sx, sx, sx);
			if (scope->name[0])
				remember_patch_def(p, scope->name);
			return 1;
		}
		return set_error(p, line_no, scope->dimension == 3 ? "expected patch scale N or scale (x,y,z)" : "expected patch scale N or scale (x,y)");
	}
	if (starts_with_word(line, "pitch"))
	{
		if (scope->dimension != 3)
			return set_error(p, line_no, "pitch is only valid for 3d patches");
		if (sscanf(line, "pitch %f", &pitch) == 1)
		{
			scope->rotation3.y = pitch;
			if (scope->name[0])
				remember_patch_def(p, scope->name);
			return 1;
		}
		return set_error(p, line_no, "expected patch pitch N");
	}
	if (starts_with_word(line, "roll"))
	{
		if (scope->dimension != 3)
			return set_error(p, line_no, "roll is only valid for 3d patches");
		if (sscanf(line, "roll %f", &roll) == 1)
		{
			scope->rotation3.z = roll;
			if (scope->name[0])
				remember_patch_def(p, scope->name);
			return 1;
		}
		return set_error(p, line_no, "expected patch roll N");
	}
	if (starts_with_word(line, "yaw") || starts_with_word(line, "rotate") || starts_with_word(line, "rotation") || starts_with_word(line, "angle"))
	{
		if (scope->dimension == 3)
		{
			if (sscanf(line, "yaw %f", &yaw) == 1 ||
				sscanf(line, "rotate %f", &yaw) == 1 ||
				sscanf(line, "rotation %f", &yaw) == 1 ||
				sscanf(line, "angle %f", &yaw) == 1)
			{
				scope->rotation3.x = yaw;
				if (scope->name[0])
					remember_patch_def(p, scope->name);
				return 1;
			}
			if (sscanf(line, "rotate (%f,%f,%f)", &yaw, &pitch, &roll) == 3 ||
				sscanf(line, "rotate (%f, %f, %f)", &yaw, &pitch, &roll) == 3 ||
				sscanf(line, "rotation (%f,%f,%f)", &yaw, &pitch, &roll) == 3 ||
				sscanf(line, "rotation (%f, %f, %f)", &yaw, &pitch, &roll) == 3)
			{
				scope->rotation3 = vec3(yaw, pitch, roll);
				if (scope->name[0])
					remember_patch_def(p, scope->name);
				return 1;
			}
			return set_error(p, line_no, "expected patch yaw N, pitch N, roll N, or rotate (yaw,pitch,roll)");
		}
		if (sscanf(line, "yaw %f", &rotation) == 1 ||
			sscanf(line, "rotate %f", &rotation) == 1 ||
			sscanf(line, "rotation %f", &rotation) == 1 ||
			sscanf(line, "angle %f", &rotation) == 1)
		{
			scope->rotation = scope->root_manifold ? -rotation : rotation;
			if (scope->name[0])
				remember_patch_def(p, scope->name);
			return 1;
		}
		return set_error(p, line_no, "expected patch rotate/rotation/angle N");
	}
	if (starts_with_word(line, "coords") || starts_with_word(line, "coord") || starts_with_word(line, "type"))
	{
		if (sscanf(line, "coords %31s", coord_name) != 1 &&
			sscanf(line, "coord %31s", coord_name) != 1 &&
			sscanf(line, "type %31s", coord_name) != 1)
			return set_error(p, line_no, scope->dimension == 3 ? "expected patch coords cartesian|cylindrical|spherical" : "expected patch coords cartesian|polar");
		if (scope->dimension == 3)
		{
			if (strcmp(coord_name, "cartesian") == 0)
				scope->coord_type = WB_PATCH_COORD_CARTESIAN;
			else if (strcmp(coord_name, "cylindrical") == 0)
				scope->coord_type = WB_PATCH_COORD_CYLINDRICAL;
			else if (strcmp(coord_name, "spherical") == 0)
				scope->coord_type = WB_PATCH_COORD_SPHERICAL;
			else
				return set_error(p, line_no, "expected patch coords cartesian|cylindrical|spherical");
		}
		else
			scope->coord_type = (strcmp(coord_name, "polar") == 0) ? WB_PATCH_COORD_POLAR : WB_PATCH_COORD_CARTESIAN;
		if (scope->name[0])
			remember_patch_def(p, scope->name);
		return 1;
	}
	return set_error(p, line_no, "unknown patch property");
}

static int parse_camera(wb_spec_parser *p, char *line, int line_no)
{
	float distance = WB_DEFAULT_LAYER_CAMERA_DISTANCE;
	float scale = WB_DEFAULT_LAYER_CAMERA_SCALE;
	float yaw = WB_DEFAULT_LAYER_CAMERA_YAW;
	float cx = WB_DEFAULT_LAYER_CAMERA_CENTER_X;
	float cy = WB_DEFAULT_LAYER_CAMERA_CENTER_Y;
	float tx = 0.0f, ty = 0.0f, tz = 0.0f;
	char projection_name[32] = "perspective";
	int projection = WB_CAMERA_PROJECTION_PERSPECTIVE;
	int target_explicit = 0;
	int matched = 0;
	int saw_any = 0;
	
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
		if (strstr(line, " look_at ") &&
			(sscanf(strstr(line, " look_at "), " look_at (%f,%f,%f)", &tx, &ty, &tz) != 3 &&
			 sscanf(strstr(line, " look_at "), " look_at (%f, %f, %f)", &tx, &ty, &tz) != 3))
			return set_error(p, line_no, "expected look_at (x,y,z)");
		wb_scene_set_patch_camera(p->scene, p->scene->current_patch_id, distance, scale, matched == 5 ? yaw : 0.0f, projection, vec2(cx, cy), strstr(line, " look_at ") || strstr(line, " target "), vec3(tx, ty, tz));
		return 1;
	}
	if (strcmp(line, "camera") == 0 || strcmp(line, "cam") == 0)
	{
		wb_scene_set_patch_camera(p->scene, p->scene->current_patch_id, distance, scale, yaw, projection, vec2(cx, cy), 0, vec3(0, 0, 0));
		return 1;
	}
	
	if (strstr(line, " distance "))
		saw_any = sscanf(strstr(line, " distance "), " distance %f", &distance) == 1 || saw_any;
	else if (strstr(line, " d "))
		saw_any = sscanf(strstr(line, " d "), " d %f", &distance) == 1 || saw_any;
	if (strstr(line, " scale "))
		saw_any = sscanf(strstr(line, " scale "), " scale %f", &scale) == 1 || saw_any;
	else if (strstr(line, " s "))
		saw_any = sscanf(strstr(line, " s "), " s %f", &scale) == 1 || saw_any;
	if (strstr(line, " yaw "))
		saw_any = sscanf(strstr(line, " yaw "), " yaw %f", &yaw) == 1 || saw_any;
	else if (strstr(line, " y "))
		saw_any = sscanf(strstr(line, " y "), " y %f", &yaw) == 1 || saw_any;
	if (strstr(line, " projection "))
		saw_any = sscanf(strstr(line, " projection "), " projection %31s", projection_name) == 1 || saw_any;
	else if (strstr(line, " proj "))
		saw_any = sscanf(strstr(line, " proj "), " proj %31s", projection_name) == 1 || saw_any;
	if (strstr(line, " center "))
		saw_any = (sscanf(strstr(line, " center "), " center (%f,%f)", &cx, &cy) == 2 ||
			sscanf(strstr(line, " center "), " center (%f, %f)", &cx, &cy) == 2) || saw_any;
	else if (strstr(line, " @ "))
		saw_any = (sscanf(strstr(line, " @ "), " @ (%f,%f)", &cx, &cy) == 2 ||
			sscanf(strstr(line, " @ "), " @ (%f, %f)", &cx, &cy) == 2) || saw_any;
	if (strstr(line, " look_at "))
	{
		if (sscanf(strstr(line, " look_at "), " look_at (%f,%f,%f)", &tx, &ty, &tz) == 3 ||
			sscanf(strstr(line, " look_at "), " look_at (%f, %f, %f)", &tx, &ty, &tz) == 3)
		{
			target_explicit = 1;
			saw_any = 1;
		}
		else if (strstr(line, " look_at origin ") || strcmp(line, "camera look_at origin") == 0 || strcmp(line, "cam look_at origin") == 0)
		{
			target_explicit = 0;
			saw_any = 1;
		}
	}
	else if (strstr(line, " target "))
	{
		if (sscanf(strstr(line, " target "), " target (%f,%f,%f)", &tx, &ty, &tz) == 3 ||
			sscanf(strstr(line, " target "), " target (%f, %f, %f)", &tx, &ty, &tz) == 3)
		{
			target_explicit = 1;
			saw_any = 1;
		}
		else if (strstr(line, " target origin ") || strcmp(line, "camera target origin") == 0 || strcmp(line, "cam target origin") == 0)
		{
			target_explicit = 0;
			saw_any = 1;
		}
	}
	if (strcmp(projection_name, "orthographic") == 0 || strcmp(projection_name, "ortho") == 0)
		projection = WB_CAMERA_PROJECTION_ORTHOGRAPHIC;
	else
		projection = WB_CAMERA_PROJECTION_PERSPECTIVE;
	if (saw_any)
	{
		wb_scene_set_patch_camera(p->scene, p->scene->current_patch_id, distance, scale, yaw, projection, vec2(cx, cy), target_explicit, vec3(tx, ty, tz));
		return 1;
	}
	
	return set_error(p, line_no, "expected camera distance D scale S [yaw A] [projection perspective|orthographic] [look_at (x,y,z)] center (x,y)");
}

static int parse_defaults(wb_spec_parser *p, char *line, int line_no)
{
	char colour_name[64] = "";
	float thickness = 0.0f;
	float opacity = 0.0f;
	float jitter = 0.0f;
	int saw_any = 0;
	wb_spec_defaults_scope scope;
	
	if (!p)
		return 0;
	if (strcmp(line, "defaults clear") == 0 || strcmp(line, "defaults off") == 0)
	{
		clear_active_defaults(p);
		return 1;
	}
	if (strstr(line, " colour "))
		saw_any = sscanf(strstr(line, " colour "), " colour %63s", colour_name) == 1 || saw_any;
	else if (strstr(line, " color "))
		saw_any = sscanf(strstr(line, " color "), " color %63s", colour_name) == 1 || saw_any;
	else if (strstr(line, " c "))
		saw_any = sscanf(strstr(line, " c "), " c %63s", colour_name) == 1 || saw_any;
	if (strstr(line, " thickness "))
		saw_any = sscanf(strstr(line, " thickness "), " thickness %f", &thickness) == 1 || saw_any;
	else if (strstr(line, " t "))
		saw_any = sscanf(strstr(line, " t "), " t %f", &thickness) == 1 || saw_any;
	if (strstr(line, " opacity "))
		saw_any = sscanf(strstr(line, " opacity "), " opacity %f", &opacity) == 1 || saw_any;
	else if (strstr(line, " o "))
		saw_any = sscanf(strstr(line, " o "), " o %f", &opacity) == 1 || saw_any;
	if (parse_jitter_token(line, &jitter))
		saw_any = 1;
	if (!saw_any)
		return set_error(p, line_no, "expected defaults with colour/thickness/opacity/jitter properties");

	if (p->n_default_scopes >= (int)(sizeof(p->default_scopes) / sizeof(p->default_scopes[0])))
		return set_error(p, line_no, "too many nested defaults scopes");
	memset(&scope, 0, sizeof(scope));
	if (p->n_default_scopes > 0)
		scope = p->default_scopes[p->n_default_scopes - 1];
	scope.indent = p->current_line_indent;
	if (colour_name[0])
	{
		snprintf(scope.colour_name, sizeof(scope.colour_name), "%s", colour_name);
		scope.colour_set = 1;
	}
	if (thickness > 0.0f)
	{
		scope.thickness = thickness;
		scope.thickness_set = 1;
	}
	if (opacity || strstr(line, " opacity ") || strstr(line, " o "))
	{
		if (opacity < WB_MIN_OPACITY)
			opacity = WB_MIN_OPACITY;
		if (opacity > WB_MAX_OPACITY)
			opacity = WB_MAX_OPACITY;
		scope.opacity = opacity;
		scope.opacity_set = 1;
	}
	if (parse_jitter_token(line, &jitter))
	{
		if (jitter < WB_MIN_JITTER_STRENGTH)
			jitter = WB_MIN_JITTER_STRENGTH;
		scope.jitter_strength = jitter;
		scope.jitter_set = 1;
	}
	p->default_scopes[p->n_default_scopes++] = scope;
	return 1;
}

static int parse_move(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, t0 = 0.0f, t1 = 0.0f;
	float z1 = 0.0f, z2 = 0.0f;
	int is_3d = 0;
	wb_spec_group *group = NULL;
	int id = 0;
	
	if (sscanf(line, "move %63s from (%f,%f,%f) to (%f,%f,%f) during %fs..%fs", name, &x1, &y1, &z1, &x2, &y2, &z2, &t0, &t1) == 9 ||
		sscanf(line, "move %63s from (%f, %f, %f) to (%f, %f, %f) during %fs..%fs", name, &x1, &y1, &z1, &x2, &y2, &z2, &t0, &t1) == 9 ||
		sscanf(line, "move %63s (%f,%f,%f) -> (%f,%f,%f) %fs..%fs", name, &x1, &y1, &z1, &x2, &y2, &z2, &t0, &t1) == 9 ||
		sscanf(line, "move %63s (%f, %f, %f) -> (%f, %f, %f) %fs..%fs", name, &x1, &y1, &z1, &x2, &y2, &z2, &t0, &t1) == 9)
	{
		group = find_group(p, name);
		id = find_name(p, name);
		if (!id && !group)
			return set_error(p, line_no, "move references unknown object");
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
				wb_scene_translate3d(p->scene, group->ids[i], t0, t1, x1, y1, z1, x2, y2, z2);
		}
		else
			wb_scene_translate3d(p->scene, id, t0, t1, x1, y1, z1, x2, y2, z2);
		return 1;
	}
	if (sscanf(line, "move %63s from (%f,%f) to (%f,%f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move %63s from (%f, %f) to (%f, %f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move %63s (%f,%f) -> (%f,%f) %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7 ||
		sscanf(line, "move %63s (%f, %f) -> (%f, %f) %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) == 7)
	{
		group = find_group(p, name);
		id = find_name(p, name);
		if (!id && !group)
			return set_error(p, line_no, "move references unknown object");
		is_3d = (group && group->n_ids > 0) ? group_is_3d(p, group) : scene_object_is_3d(p->scene, id);
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
			{
				if (is_3d)
					wb_scene_translate3d(p->scene, group->ids[i], t0, t1, x1, y1, 0.0f, x2, y2, 0.0f);
				else
					wb_scene_move(p->scene, group->ids[i], t0, t1, x1, y1, x2, y2);
			}
		}
		else
		{
			if (is_3d)
				wb_scene_translate3d(p->scene, id, t0, t1, x1, y1, 0.0f, x2, y2, 0.0f);
			else
				wb_scene_move(p->scene, id, t0, t1, x1, y1, x2, y2);
		}
		return 1;
	}
	if ((sscanf(line, "move %63s", name) == 1) &&
		((sscanf(strstr(line, " from ") ? strstr(line, " from ") : "", " from (%f,%f,%f)", &x1, &y1, &z1) == 3 ||
		  sscanf(strstr(line, " from ") ? strstr(line, " from ") : "", " from (%f, %f, %f)", &x1, &y1, &z1) == 3) &&
		 (sscanf(strstr(line, " to ") ? strstr(line, " to ") : "", " to (%f,%f,%f)", &x2, &y2, &z2) == 3 ||
		  sscanf(strstr(line, " to ") ? strstr(line, " to ") : "", " to (%f, %f, %f)", &x2, &y2, &z2) == 3) &&
		 (sscanf(strstr(line, " during ") ? strstr(line, " during ") : "", " during %fs..%fs", &t0, &t1) == 2)))
	{
		group = find_group(p, name);
		id = find_name(p, name);
		if (!id && !group)
			return set_error(p, line_no, "move references unknown object");
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
				wb_scene_translate3d(p->scene, group->ids[i], t0, t1, x1, y1, z1, x2, y2, z2);
		}
		else
			wb_scene_translate3d(p->scene, id, t0, t1, x1, y1, z1, x2, y2, z2);
		return 1;
	}
	if ((sscanf(line, "move %63s", name) == 1) &&
		((sscanf(strstr(line, " from ") ? strstr(line, " from ") : "", " from (%f,%f)", &x1, &y1) == 2 ||
		  sscanf(strstr(line, " from ") ? strstr(line, " from ") : "", " from (%f, %f)", &x1, &y1) == 2) &&
		 (sscanf(strstr(line, " to ") ? strstr(line, " to ") : "", " to (%f,%f)", &x2, &y2) == 2 ||
		  sscanf(strstr(line, " to ") ? strstr(line, " to ") : "", " to (%f, %f)", &x2, &y2) == 2) &&
		 (sscanf(strstr(line, " during ") ? strstr(line, " during ") : "", " during %fs..%fs", &t0, &t1) == 2)))
	{
		group = find_group(p, name);
		id = find_name(p, name);
		if (!id && !group)
			return set_error(p, line_no, "move references unknown object");
		is_3d = (group && group->n_ids > 0) ? group_is_3d(p, group) : scene_object_is_3d(p->scene, id);
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
			{
				if (is_3d)
					wb_scene_translate3d(p->scene, group->ids[i], t0, t1, x1, y1, 0.0f, x2, y2, 0.0f);
				else
					wb_scene_move(p->scene, group->ids[i], t0, t1, x1, y1, x2, y2);
			}
		}
		else
		{
			if (is_3d)
				wb_scene_translate3d(p->scene, id, t0, t1, x1, y1, 0.0f, x2, y2, 0.0f);
			else
				wb_scene_move(p->scene, id, t0, t1, x1, y1, x2, y2);
		}
		return 1;
	}
	
	return set_error(p, line_no, "expected move name from (x,y) to (x,y) or from (x,y,z) to (x,y,z) during Ts..Ts");
}

static int parse_move_layer(wb_spec_parser *p, char *line, int line_no)
{
	(void)line;
	return set_error(p, line_no, "move_layer has been replaced by move_patch");
	#if 0
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
	#endif
}

static int parse_move_patch(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, t0 = 0.0f, t1 = 0.0f;
	float z1 = 0.0f, z2 = 0.0f;
	wb_spec_patch_def *patch;
	wb_spec_group *group;
	wb_vec2 origin_world;
	wb_vec2 from_world;
	wb_vec2 to_world;
	wb_vec2 delta_from;
	wb_vec2 delta_to;
	wb_vec3 origin_world3;
	wb_vec3 from_world3;
	wb_vec3 to_world3;
	wb_vec3 delta_from3;
	wb_vec3 delta_to3;
	int dimension = 2;
	
	if (sscanf(line, "move_patch %63s", name) != 1)
		return set_error(p, line_no, "expected move_patch name ...");
	patch = find_patch_def(p, name);
	group = find_group(p, name);
	if (!patch || !group || group->n_ids <= 0)
		return set_error(p, line_no, "move_patch references unknown patch");
	if (patch->n_scopes > 0)
		dimension = patch->scopes[patch->n_scopes - 1].dimension;
	
	if (dimension == 3)
	{
		if (sscanf(line, "move_patch %63s from (%f,%f,%f) to (%f,%f,%f) during %fs..%fs", name, &x1, &y1, &z1, &x2, &y2, &z2, &t0, &t1) != 9 &&
			sscanf(line, "move_patch %63s from (%f, %f, %f) to (%f, %f, %f) during %fs..%fs", name, &x1, &y1, &z1, &x2, &y2, &z2, &t0, &t1) != 9 &&
			sscanf(line, "move_patch %63s (%f,%f,%f) -> (%f,%f,%f) %fs..%fs", name, &x1, &y1, &z1, &x2, &y2, &z2, &t0, &t1) != 9 &&
			sscanf(line, "move_patch %63s (%f, %f, %f) -> (%f, %f, %f) %fs..%fs", name, &x1, &y1, &z1, &x2, &y2, &z2, &t0, &t1) != 9)
			return set_error(p, line_no, "expected move_patch name from (x,y,z) to (x,y,z) during Ts..Ts");
		
		origin_world3 = apply_patch_scope_chain_to_point3d(patch->scopes, patch->n_scopes, vec3(0, 0, 0));
		from_world3 = apply_patch_scope_chain_to_point3d(patch->scopes, patch->n_scopes, vec3(x1, y1, z1));
		to_world3 = apply_patch_scope_chain_to_point3d(patch->scopes, patch->n_scopes, vec3(x2, y2, z2));
		delta_from3 = vec3_diff(from_world3, origin_world3);
		delta_to3 = vec3_diff(to_world3, origin_world3);
		wb_scene_translate_patch3d(p->scene,
			patch->scopes[patch->n_scopes - 1].patch_id, t0, t1,
			delta_from3, delta_to3);
		return 1;
	}
	
	if (sscanf(line, "move_patch %63s from (%f,%f) to (%f,%f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) != 7 &&
		sscanf(line, "move_patch %63s from (%f, %f) to (%f, %f) during %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) != 7 &&
		sscanf(line, "move_patch %63s (%f,%f) -> (%f,%f) %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) != 7 &&
		sscanf(line, "move_patch %63s (%f, %f) -> (%f, %f) %fs..%fs", name, &x1, &y1, &x2, &y2, &t0, &t1) != 7)
		return set_error(p, line_no, "expected move_patch name from (x,y) to (x,y) during Ts..Ts");
	
	origin_world = apply_patch_scope_chain_to_point(patch->scopes, patch->n_scopes, vec2(0, 0));
	from_world = apply_patch_scope_chain_to_point(patch->scopes, patch->n_scopes, vec2(x1, y1));
	to_world = apply_patch_scope_chain_to_point(patch->scopes, patch->n_scopes, vec2(x2, y2));
	delta_from = vec2_diff(from_world, origin_world);
	delta_to = vec2_diff(to_world, origin_world);
	wb_scene_translate_patch(p->scene,
		patch->scopes[patch->n_scopes - 1].patch_id, t0, t1,
		delta_from, delta_to);
	return 1;
}

static int parse_turn(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float a0 = 0.0f, a1 = 0.0f, t0 = 0.0f, t1 = 0.0f;
	float y0 = 0.0f, p0 = 0.0f, r0 = 0.0f, y1 = 0.0f, p1 = 0.0f, r1 = 0.0f;
	float px = 0.0f, py = 0.0f, pz = 0.0f;
	int vector_mode = 0;
	int is_3d = 0;
	wb_spec_group *group = NULL;
	int id = 0;

	if (sscanf(line, "turn %63s", name) != 1)
		return set_error(p, line_no, "expected turn name ...");
	group = find_group(p, name);
	id = find_name(p, name);
	if (!id && !group)
		return set_error(p, line_no, "turn references unknown object");
	is_3d = (group && group->n_ids > 0) ? group_is_3d(p, group) : scene_object_is_3d(p->scene, id);

	if (is_3d)
	{
		if ((sscanf(line, "turn %63s around (%f,%f,%f) from (%f,%f,%f) to (%f,%f,%f) during %fs..%fs", name, &px, &py, &pz, &y0, &p0, &r0, &y1, &p1, &r1, &t0, &t1) == 12) ||
			(sscanf(line, "turn %63s around (%f, %f, %f) from (%f, %f, %f) to (%f, %f, %f) during %fs..%fs", name, &px, &py, &pz, &y0, &p0, &r0, &y1, &p1, &r1, &t0, &t1) == 12) ||
			(sscanf(line, "turn %63s around (%f,%f,%f) (%f,%f,%f) -> (%f,%f,%f) %fs..%fs", name, &px, &py, &pz, &y0, &p0, &r0, &y1, &p1, &r1, &t0, &t1) == 12) ||
			(sscanf(line, "turn %63s around (%f, %f, %f) (%f, %f, %f) -> (%f, %f, %f) %fs..%fs", name, &px, &py, &pz, &y0, &p0, &r0, &y1, &p1, &r1, &t0, &t1) == 12))
			vector_mode = 1;
		else if (sscanf(line, "turn %63s around (%f,%f,%f) from %f to %f during %fs..%fs", name, &px, &py, &pz, &a0, &a1, &t0, &t1) != 8 &&
			sscanf(line, "turn %63s around (%f, %f, %f) from %f to %f during %fs..%fs", name, &px, &py, &pz, &a0, &a1, &t0, &t1) != 8 &&
			sscanf(line, "turn %63s around (%f,%f,%f) %f -> %f %fs..%fs", name, &px, &py, &pz, &a0, &a1, &t0, &t1) != 8 &&
			sscanf(line, "turn %63s around (%f, %f, %f) %f -> %f %fs..%fs", name, &px, &py, &pz, &a0, &a1, &t0, &t1) != 8)
			return set_error(p, line_no, "expected turn name around (x,y,z) from A to A during Ts..Ts");
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
				wb_scene_transform3d(p->scene, group->ids[i], t0, t1, px, py, pz, 1.0f, 1.0f, 1.0f, vector_mode ? y0 : a0, vector_mode ? p0 : 0.0f, vector_mode ? r0 : 0.0f, 1.0f, 1.0f, 1.0f, vector_mode ? y1 : a1, vector_mode ? p1 : 0.0f, vector_mode ? r1 : 0.0f);
		}
		else
			wb_scene_transform3d(p->scene, id, t0, t1, px, py, pz, 1.0f, 1.0f, 1.0f, vector_mode ? y0 : a0, vector_mode ? p0 : 0.0f, vector_mode ? r0 : 0.0f, 1.0f, 1.0f, 1.0f, vector_mode ? y1 : a1, vector_mode ? p1 : 0.0f, vector_mode ? r1 : 0.0f);
		return 1;
	}

	if (sscanf(line, "turn %63s around (%f,%f) from %f to %f during %fs..%fs", name, &px, &py, &a0, &a1, &t0, &t1) != 7 &&
		sscanf(line, "turn %63s around (%f, %f) from %f to %f during %fs..%fs", name, &px, &py, &a0, &a1, &t0, &t1) != 7 &&
		sscanf(line, "turn %63s around (%f,%f) %f -> %f %fs..%fs", name, &px, &py, &a0, &a1, &t0, &t1) != 7 &&
		sscanf(line, "turn %63s around (%f, %f) %f -> %f %fs..%fs", name, &px, &py, &a0, &a1, &t0, &t1) != 7)
		return set_error(p, line_no, "expected turn name around (x,y) from A to A during Ts..Ts");
	if (group && group->n_ids > 0)
	{
		for (int i = 0; i < group->n_ids; i++)
			wb_scene_transform(p->scene, group->ids[i], t0, t1, px, py, 1.0f, 1.0f, a0, 1.0f, 1.0f, a1);
	}
	else
		wb_scene_transform(p->scene, id, t0, t1, px, py, 1.0f, 1.0f, a0, 1.0f, 1.0f, a1);
	return 1;
}

static int parse_scale(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float sx0 = 1.0f, sy0 = 1.0f, sz0 = 1.0f, sx1 = 1.0f, sy1 = 1.0f, sz1 = 1.0f;
	float px = 0.0f, py = 0.0f, pz = 0.0f, t0 = 0.0f, t1 = 0.0f;
	int matched = 0;
	int is_3d = 0;
	wb_spec_group *group = NULL;
	int id = 0;

	if (sscanf(line, "scale %63s", name) != 1)
		return set_error(p, line_no, "expected scale name ...");
	group = find_group(p, name);
	id = find_name(p, name);
	if (!id && !group)
		return set_error(p, line_no, "scale references unknown object");
	is_3d = (group && group->n_ids > 0) ? group_is_3d(p, group) : scene_object_is_3d(p->scene, id);

	if (is_3d)
	{
		matched = sscanf(line, "scale %63s around (%f,%f,%f) from (%f,%f,%f) to (%f,%f,%f) during %fs..%fs", name, &px, &py, &pz, &sx0, &sy0, &sz0, &sx1, &sy1, &sz1, &t0, &t1);
		if (matched != 12)
			matched = sscanf(line, "scale %63s around (%f, %f, %f) from (%f, %f, %f) to (%f, %f, %f) during %fs..%fs", name, &px, &py, &pz, &sx0, &sy0, &sz0, &sx1, &sy1, &sz1, &t0, &t1);
		if (matched != 12)
			matched = sscanf(line, "scale %63s around (%f,%f,%f) (%f,%f,%f) -> (%f,%f,%f) %fs..%fs", name, &px, &py, &pz, &sx0, &sy0, &sz0, &sx1, &sy1, &sz1, &t0, &t1);
		if (matched != 12)
			matched = sscanf(line, "scale %63s around (%f, %f, %f) (%f, %f, %f) -> (%f, %f, %f) %fs..%fs", name, &px, &py, &pz, &sx0, &sy0, &sz0, &sx1, &sy1, &sz1, &t0, &t1);
		if (matched != 12 && matched != 8)
			matched = sscanf(line, "scale %63s around (%f,%f,%f) from %f to %f during %fs..%fs", name, &px, &py, &pz, &sx0, &sx1, &t0, &t1);
		if (matched != 12 && matched != 8)
			matched = sscanf(line, "scale %63s around (%f, %f, %f) from %f to %f during %fs..%fs", name, &px, &py, &pz, &sx0, &sx1, &t0, &t1);
		if (matched != 12 && matched != 8)
			matched = sscanf(line, "scale %63s around (%f,%f,%f) %f -> %f %fs..%fs", name, &px, &py, &pz, &sx0, &sx1, &t0, &t1);
		if (matched != 12 && matched != 8)
			matched = sscanf(line, "scale %63s around (%f, %f, %f) %f -> %f %fs..%fs", name, &px, &py, &pz, &sx0, &sx1, &t0, &t1);
		if (matched == 8)
		{
			sy0 = sx0;
			sz0 = sx0;
			sy1 = sx1;
			sz1 = sx1;
		}
		else if (matched != 12)
			return set_error(p, line_no, "expected scale name around (x,y,z) from (sx,sy,sz) to (sx,sy,sz) during Ts..Ts");
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
				wb_scene_transform3d(p->scene, group->ids[i], t0, t1, px, py, pz, sx0, sy0, sz0, 0.0f, 0.0f, 0.0f, sx1, sy1, sz1, 0.0f, 0.0f, 0.0f);
		}
		else
			wb_scene_transform3d(p->scene, id, t0, t1, px, py, pz, sx0, sy0, sz0, 0.0f, 0.0f, 0.0f, sx1, sy1, sz1, 0.0f, 0.0f, 0.0f);
		return 1;
	}

	matched = sscanf(line, "scale %63s around (%f,%f) from (%f,%f) to (%f,%f) during %fs..%fs", name, &px, &py, &sx0, &sy0, &sx1, &sy1, &t0, &t1);
	if (matched != 9)
		matched = sscanf(line, "scale %63s around (%f, %f) from (%f, %f) to (%f, %f) during %fs..%fs", name, &px, &py, &sx0, &sy0, &sx1, &sy1, &t0, &t1);
	if (matched != 9 && matched != 7)
		matched = sscanf(line, "scale %63s around (%f,%f) from %f to %f during %fs..%fs", name, &px, &py, &sx0, &sx1, &t0, &t1);
	if (matched != 9 && matched != 7)
		matched = sscanf(line, "scale %63s around (%f, %f) from %f to %f during %fs..%fs", name, &px, &py, &sx0, &sx1, &t0, &t1);
	if (matched != 9 && matched != 7)
		matched = sscanf(line, "scale %63s around (%f,%f) (%f,%f) -> (%f,%f) %fs..%fs", name, &px, &py, &sx0, &sy0, &sx1, &sy1, &t0, &t1);
	if (matched != 9 && matched != 7)
		matched = sscanf(line, "scale %63s around (%f, %f) (%f, %f) -> (%f, %f) %fs..%fs", name, &px, &py, &sx0, &sy0, &sx1, &sy1, &t0, &t1);
	if (matched != 9 && matched != 7)
		matched = sscanf(line, "scale %63s around (%f,%f) %f -> %f %fs..%fs", name, &px, &py, &sx0, &sx1, &t0, &t1);
	if (matched != 9 && matched != 7)
		matched = sscanf(line, "scale %63s around (%f, %f) %f -> %f %fs..%fs", name, &px, &py, &sx0, &sx1, &t0, &t1);
	if (matched == 7)
	{
		sy0 = sx0;
		sy1 = sx1;
	}
	else if (matched != 9)
		return set_error(p, line_no, "expected scale name around (x,y) from (sx,sy) to (sx,sy) during Ts..Ts");
	if (group && group->n_ids > 0)
	{
		for (int i = 0; i < group->n_ids; i++)
			wb_scene_transform(p->scene, group->ids[i], t0, t1, px, py, sx0, sy0, 0.0f, sx1, sy1, 0.0f);
	}
	else
		wb_scene_transform(p->scene, id, t0, t1, px, py, sx0, sy0, 0.0f, sx1, sy1, 0.0f);
	return 1;
}

static int parse_turn_patch(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float a0 = 0.0f, a1 = 0.0f, t0 = 0.0f, t1 = 0.0f;
	float y0 = 0.0f, p0 = 0.0f, r0 = 0.0f, y1 = 0.0f, p1 = 0.0f, r1 = 0.0f;
	wb_spec_patch_def *patch;
	wb_spec_group *group;
	wb_vec2 pivot;
	wb_vec3 pivot3;
	int dimension = 2;
	int vector_mode = 0;
	
	if (sscanf(line, "turn_patch %63s from (%f,%f,%f) to (%f,%f,%f) during %fs..%fs", name, &y0, &p0, &r0, &y1, &p1, &r1, &t0, &t1) == 9 ||
		sscanf(line, "turn_patch %63s from (%f, %f, %f) to (%f, %f, %f) during %fs..%fs", name, &y0, &p0, &r0, &y1, &p1, &r1, &t0, &t1) == 9 ||
		sscanf(line, "turn_patch %63s (%f,%f,%f) -> (%f,%f,%f) %fs..%fs", name, &y0, &p0, &r0, &y1, &p1, &r1, &t0, &t1) == 9 ||
		sscanf(line, "turn_patch %63s (%f, %f, %f) -> (%f, %f, %f) %fs..%fs", name, &y0, &p0, &r0, &y1, &p1, &r1, &t0, &t1) == 9)
		vector_mode = 1;
	else if (sscanf(line, "turn_patch %63s from %f to %f during %fs..%fs", name, &a0, &a1, &t0, &t1) != 5 &&
		sscanf(line, "turn_patch %63s %f -> %f %fs..%fs", name, &a0, &a1, &t0, &t1) != 5)
		return set_error(p, line_no, "expected turn_patch name from A to A during Ts..Ts");
	patch = find_patch_def(p, name);
	group = find_group(p, name);
	if (!patch || !group || group->n_ids <= 0)
		return set_error(p, line_no, "turn_patch references unknown patch");
	if (patch->n_scopes > 0)
		dimension = patch->scopes[patch->n_scopes - 1].dimension;
	if (dimension == 3)
	{
		pivot3 = apply_patch_scope_chain_to_point3d(patch->scopes, patch->n_scopes, vec3(0, 0, 0));
		wb_scene_transform_patch3d(p->scene, patch->scopes[patch->n_scopes - 1].patch_id,
			t0, t1, pivot3, vec3(1, 1, 1), vec3(vector_mode ? y0 : a0, vector_mode ? p0 : 0.0f, vector_mode ? r0 : 0.0f),
			vec3(1, 1, 1), vec3(vector_mode ? y1 : a1, vector_mode ? p1 : 0.0f, vector_mode ? r1 : 0.0f));
		return 1;
	}
	pivot = apply_patch_scope_chain_to_point(patch->scopes, patch->n_scopes, vec2(0, 0));
	wb_scene_transform_patch(p->scene, patch->scopes[patch->n_scopes - 1].patch_id,
		t0, t1, pivot, vec2(1, 1), a0, vec2(1, 1), a1);
	return 1;
}

static int parse_scale_patch(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float sx0 = 1.0f, sy0 = 1.0f, sx1 = 1.0f, sy1 = 1.0f, t0 = 0.0f, t1 = 0.0f;
	float sz0 = 1.0f, sz1 = 1.0f;
	wb_spec_patch_def *patch;
	wb_spec_group *group;
	wb_vec2 pivot;
	wb_vec3 pivot3;
	int matched = 0;
	int dimension = 2;
	
	patch = find_patch_def(p, name);
	if (!patch && sscanf(line, "scale_patch %63s", name) == 1)
		patch = find_patch_def(p, name);
	group = find_group(p, name);
	if (!patch || !group || group->n_ids <= 0)
		return set_error(p, line_no, "scale_patch references unknown patch");
	if (patch->n_scopes > 0)
		dimension = patch->scopes[patch->n_scopes - 1].dimension;
	if (dimension == 3)
	{
		matched = sscanf(line, "scale_patch %63s from (%f,%f,%f) to (%f,%f,%f) during %fs..%fs", name, &sx0, &sy0, &sz0, &sx1, &sy1, &sz1, &t0, &t1);
		if (matched != 9)
			matched = sscanf(line, "scale_patch %63s from (%f, %f, %f) to (%f, %f, %f) during %fs..%fs", name, &sx0, &sy0, &sz0, &sx1, &sy1, &sz1, &t0, &t1);
		if (matched != 9)
			matched = sscanf(line, "scale_patch %63s (%f,%f,%f) -> (%f,%f,%f) %fs..%fs", name, &sx0, &sy0, &sz0, &sx1, &sy1, &sz1, &t0, &t1);
		if (matched != 9)
			matched = sscanf(line, "scale_patch %63s (%f, %f, %f) -> (%f, %f, %f) %fs..%fs", name, &sx0, &sy0, &sz0, &sx1, &sy1, &sz1, &t0, &t1);
		if (matched != 9 && matched != 5)
			matched = sscanf(line, "scale_patch %63s from %f to %f during %fs..%fs", name, &sx0, &sx1, &t0, &t1);
		if (matched != 9 && matched != 5)
			matched = sscanf(line, "scale_patch %63s %f -> %f %fs..%fs", name, &sx0, &sx1, &t0, &t1);
		if (matched == 5)
		{
			sy0 = sx0;
			sz0 = sx0;
			sy1 = sx1;
			sz1 = sx1;
		}
		else if (matched != 9)
			return set_error(p, line_no, "expected scale_patch name from (sx,sy,sz) to (sx,sy,sz) during Ts..Ts");
		pivot3 = apply_patch_scope_chain_to_point3d(patch->scopes, patch->n_scopes, vec3(0, 0, 0));
		wb_scene_transform_patch3d(p->scene, patch->scopes[patch->n_scopes - 1].patch_id,
			t0, t1, pivot3, vec3(sx0, sy0, sz0), vec3(0, 0, 0), vec3(sx1, sy1, sz1), vec3(0, 0, 0));
		return 1;
	}
	matched = sscanf(line, "scale_patch %63s from (%f,%f) to (%f,%f) during %fs..%fs", name, &sx0, &sy0, &sx1, &sy1, &t0, &t1);
	if (matched != 7)
		matched = sscanf(line, "scale_patch %63s from (%f, %f) to (%f, %f) during %fs..%fs", name, &sx0, &sy0, &sx1, &sy1, &t0, &t1);
	if (matched != 7 && matched != 5)
		matched = sscanf(line, "scale_patch %63s from %f to %f during %fs..%fs", name, &sx0, &sx1, &t0, &t1);
	if (matched != 7 && matched != 5)
		matched = sscanf(line, "scale_patch %63s %f -> %f %fs..%fs", name, &sx0, &sx1, &t0, &t1);
	if (matched == 5)
	{
		sy0 = sx0;
		sy1 = sx1;
	}
	else if (matched != 7)
		return set_error(p, line_no, "expected scale_patch name from (sx,sy) to (sx,sy) during Ts..Ts");
	pivot = apply_patch_scope_chain_to_point(patch->scopes, patch->n_scopes, vec2(0, 0));
	wb_scene_transform_patch(p->scene, patch->scopes[patch->n_scopes - 1].patch_id,
		t0, t1, pivot, vec2(sx0, sy0), 0.0f, vec2(sx1, sy1), 0.0f);
	return 1;
}

static int parse_move_camera(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float d1 = WB_DEFAULT_LAYER_CAMERA_DISTANCE, s1 = WB_DEFAULT_LAYER_CAMERA_SCALE, y1 = WB_DEFAULT_LAYER_CAMERA_YAW, cx1 = WB_DEFAULT_LAYER_CAMERA_CENTER_X, cy1 = WB_DEFAULT_LAYER_CAMERA_CENTER_Y;
	float d2 = WB_DEFAULT_LAYER_CAMERA_DISTANCE, s2 = WB_DEFAULT_LAYER_CAMERA_SCALE, y2 = WB_DEFAULT_LAYER_CAMERA_YAW, cx2 = WB_DEFAULT_LAYER_CAMERA_CENTER_X, cy2 = WB_DEFAULT_LAYER_CAMERA_CENTER_Y;
	float tx1 = 0.0f, ty1 = 0.0f, tz1 = 0.0f, tx2 = 0.0f, ty2 = 0.0f, tz2 = 0.0f;
	int target1_explicit = 0, target2_explicit = 0;
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
		wb_spec_patch_def *patch_def = find_patch_def(p, name);
		if (!patch_def)
			return set_error(p, line_no, "move_camera references unknown space");
		if (strstr(line, " look_at "))
		{
			if (sscanf(strstr(line, " look_at "), " look_at (%f,%f,%f) -> (%f,%f,%f)", &tx1, &ty1, &tz1, &tx2, &ty2, &tz2) == 6 ||
				sscanf(strstr(line, " look_at "), " look_at (%f, %f, %f) -> (%f, %f, %f)", &tx1, &ty1, &tz1, &tx2, &ty2, &tz2) == 6)
				target1_explicit = target2_explicit = 1;
		}
		else if (strstr(line, " target "))
		{
			if (sscanf(strstr(line, " target "), " target (%f,%f,%f) -> (%f,%f,%f)", &tx1, &ty1, &tz1, &tx2, &ty2, &tz2) == 6 ||
				sscanf(strstr(line, " target "), " target (%f, %f, %f) -> (%f, %f, %f)", &tx1, &ty1, &tz1, &tx2, &ty2, &tz2) == 6)
				target1_explicit = target2_explicit = 1;
		}
		{
			if (patch_def && patch_def->n_scopes > 0)
				wb_scene_move_patch_camera(p->scene, patch_def->scopes[patch_def->n_scopes - 1].patch_id, t0, t1, d1, s1, y1, vec2(cx1, cy1), target1_explicit, vec3(tx1, ty1, tz1), d2, s2, y2, vec2(cx2, cy2), target2_explicit, vec3(tx2, ty2, tz2));
		}
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
		wb_spec_patch_def *patch_def = find_patch_def(p, name);
		if (!patch_def)
			return set_error(p, line_no, "orbit_camera references unknown space");
		{
			if (patch_def && patch_def->n_scopes > 0)
				wb_scene_orbit_patch_camera(p->scene, patch_def->scopes[patch_def->n_scopes - 1].patch_id, t0, t1, y0, y1);
		}
		return 1;
	}
	
	return set_error(p, line_no, "expected orbit_camera layer from A to A during Ts..Ts");
}

static int parse_fade_layer(wb_spec_parser *p, char *line, int line_no)
{
	(void)line;
	return set_error(p, line_no, "fade_layer has been replaced by fade patch_name");
	#if 0
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
	#endif
}

static int parse_fade(wb_spec_parser *p, char *line, int line_no)
{
	char name[64];
	float a0 = WB_DEFAULT_LAYER_OPACITY, a1 = WB_DEFAULT_LAYER_OPACITY, t0 = 0.0f, t1 = 0.0f;
	
	if (sscanf(line, "fade %63s from %f to %f during %fs..%fs", name, &a0, &a1, &t0, &t1) == 5 ||
		sscanf(line, "fade %63s %f -> %f %fs..%fs", name, &a0, &a1, &t0, &t1) == 5)
	{
		wb_spec_patch_def *patch = find_patch_def(p, name);
		wb_spec_group *group = find_group(p, name);
		int id = find_name(p, name);
		if (patch && patch->n_scopes > 0)
		{
			wb_scene_fade_patch(p->scene, patch->scopes[patch->n_scopes - 1].patch_id, t0, t1, a0, a1);
			return 1;
		}
		if (!id && !group)
			return set_error(p, line_no, "fade references unknown object");
		if (a0 < WB_MIN_OPACITY)
			a0 = WB_MIN_OPACITY;
		if (a0 > WB_MAX_OPACITY)
			a0 = WB_MAX_OPACITY;
		if (a1 < WB_MIN_OPACITY)
			a1 = WB_MIN_OPACITY;
		if (a1 > WB_MAX_OPACITY)
			a1 = WB_MAX_OPACITY;
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
				wb_scene_fade_object(p->scene, group->ids[i], t0, t1, a0, a1);
		}
		else
			wb_scene_fade_object(p->scene, id, t0, t1, a0, a1);
		return 1;
	}
	if ((sscanf(line, "fade %63s", name) == 1) &&
		(sscanf(strstr(line, " from ") ? strstr(line, " from ") : "", " from %f", &a0) == 1) &&
		(sscanf(strstr(line, " to ") ? strstr(line, " to ") : "", " to %f", &a1) == 1) &&
		(sscanf(strstr(line, " during ") ? strstr(line, " during ") : "", " during %fs..%fs", &t0, &t1) == 2))
	{
		wb_spec_patch_def *patch = find_patch_def(p, name);
		wb_spec_group *group = find_group(p, name);
		int id = find_name(p, name);
		if (patch && patch->n_scopes > 0)
		{
			wb_scene_fade_patch(p->scene, patch->scopes[patch->n_scopes - 1].patch_id, t0, t1, a0, a1);
			return 1;
		}
		if (!id && !group)
			return set_error(p, line_no, "fade references unknown object");
		if (a0 < WB_MIN_OPACITY) a0 = WB_MIN_OPACITY;
		if (a0 > WB_MAX_OPACITY) a0 = WB_MAX_OPACITY;
		if (a1 < WB_MIN_OPACITY) a1 = WB_MIN_OPACITY;
		if (a1 > WB_MAX_OPACITY) a1 = WB_MAX_OPACITY;
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
				wb_scene_fade_object(p->scene, group->ids[i], t0, t1, a0, a1);
		}
		else
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
		wb_spec_group *group = find_group(p, name);
		int id = find_name(p, name);
		if (!id && !group)
			return set_error(p, line_no, "draw references unknown object");
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
				wb_scene_draw_in(p->scene, group->ids[i], t0, t1);
		}
		else
			wb_scene_draw_in(p->scene, id, t0, t1);
		return 1;
	}
	if (sscanf(line, "draw %63s", name) == 1 &&
		sscanf(strstr(line, " during ") ? strstr(line, " during ") : "", " during %fs..%fs", &t0, &t1) == 2)
	{
		wb_spec_group *group = find_group(p, name);
		int id = find_name(p, name);
		if (!id && !group)
			return set_error(p, line_no, "draw references unknown object");
		if (group && group->n_ids > 0)
		{
			for (int i = 0; i < group->n_ids; i++)
				wb_scene_draw_in(p->scene, group->ids[i], t0, t1);
		}
		else
			wb_scene_draw_in(p->scene, id, t0, t1);
		return 1;
	}
	
	return set_error(p, line_no, "expected draw name during Ts..Ts");
}

static int parse_line_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "line %63s from (%f,%f) to (%f,%f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "line %63s from (%f, %f) to (%f, %f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "seg %63s (%f,%f) -> (%f,%f) t %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "seg %63s (%f, %f) -> (%f, %f) t %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5 && sscanf(line, "line %63s", name) == 1)
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(to, " to (%f,%f)", &x1, &y1) == 2 || sscanf(to, " to (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5 && sscanf(line, "seg %63s", name) == 1)
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(to, " to (%f,%f)", &x1, &y1) == 2 || sscanf(to, " to (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5 && (strcmp(line, "line") == 0 || strcmp(line, "seg") == 0 ||
		starts_with(line, "line from ") || starts_with(line, "seg from ")))
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(to, " to (%f,%f)", &x1, &y1) == 2 || sscanf(to, " to (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5)
		return set_error(p, line_no, "expected line/seg [name] from (x,y) to (x,y) thickness N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_line(p->scene, x0, y0, x1, y1, thickness, parse_colour(matched >= 7 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create line object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_ray_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "ray %63s from (%f,%f) through (%f,%f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ray %63s from (%f, %f) through (%f, %f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ray %63s (%f,%f) -> (%f,%f) t %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ray %63s (%f, %f) -> (%f, %f) t %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, colour_name);
	if (matched < 5 && sscanf(line, "ray %63s", name) == 1)
	{
		char *from = strstr(line, " from ");
		char *through = strstr(line, " through ");
		if (from && through &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(through, " through (%f,%f)", &x1, &y1) == 2 || sscanf(through, " through (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5 && (strcmp(line, "ray") == 0 || starts_with(line, "ray from ")))
	{
		char *from = strstr(line, " from ");
		char *through = strstr(line, " through ");
		if (from && through &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(through, " through (%f,%f)", &x1, &y1) == 2 || sscanf(through, " through (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5)
		return set_error(p, line_no, "expected ray [name] from (x,y) through (x,y) thickness N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_ray(p->scene, x0, y0, x1, y1, matched >= 6 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), parse_colour(matched >= 7 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create ray object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_dotted_line_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), gap = WB_DEFAULT_DOTTED_GAP;
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "dotted_line %63s from (%f,%f) to (%f,%f) thickness %f gap %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		matched = sscanf(line, "dotted_line %63s from (%f, %f) to (%f, %f) thickness %f gap %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5 && sscanf(line, "dotted_line %63s", name) == 1)
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(to, " to (%f,%f)", &x1, &y1) == 2 || sscanf(to, " to (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5 && (strcmp(line, "dotted_line") == 0 || starts_with(line, "dotted_line from ")))
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(to, " to (%f,%f)", &x1, &y1) == 2 || sscanf(to, " to (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5)
		return set_error(p, line_no, "expected dotted_line [name] from (x,y) to (x,y) thickness N gap N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	if (strstr(line, " gap "))
		sscanf(strstr(line, " gap "), " gap %f", &gap);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	
	int id = wb_scene_add_dotted_line(p->scene, x0, y0, x1, y1, matched >= 6 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), matched >= 7 ? gap : WB_DEFAULT_DOTTED_GAP, parse_colour(matched >= 8 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create dotted_line object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_dashed_line_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), gap = WB_DEFAULT_DASHED_GAP;
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "dashed_line %63s from (%f,%f) to (%f,%f) thickness %f gap %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		matched = sscanf(line, "dashed_line %63s from (%f, %f) to (%f, %f) thickness %f gap %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		matched = sscanf(line, "dash %63s (%f,%f) -> (%f,%f) t %f g %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5)
		matched = sscanf(line, "dash %63s (%f, %f) -> (%f, %f) t %f g %f c %63s", name, &x0, &y0, &x1, &y1, &thickness, &gap, colour_name);
	if (matched < 5 && (sscanf(line, "dashed_line %63s", name) == 1 || sscanf(line, "dash %63s", name) == 1))
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(to, " to (%f,%f)", &x1, &y1) == 2 || sscanf(to, " to (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5 && (strcmp(line, "dashed_line") == 0 || strcmp(line, "dash") == 0 ||
		starts_with(line, "dashed_line from ") || starts_with(line, "dash from ")))
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(to, " to (%f,%f)", &x1, &y1) == 2 || sscanf(to, " to (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5)
		return set_error(p, line_no, "expected dashed_line/dash [name] from (x,y) to (x,y) thickness N gap N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " gap "))
		sscanf(strstr(line, " gap "), " gap %f", &gap);
	else if (strstr(line, " g "))
		sscanf(strstr(line, " g "), " g %f", &gap);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_dashed_line(p->scene, x0, y0, x1, y1, matched >= 6 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), matched >= 7 ? gap : WB_DEFAULT_DASHED_GAP, parse_colour(matched >= 8 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create dashed_line object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_arrow_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), head_size = WB_DEFAULT_ARROW_HEAD_SIZE;
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "arrow %63s from (%f,%f) to (%f,%f) thickness %f head %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &head_size, colour_name);
	if (matched < 5)
		matched = sscanf(line, "arrow %63s from (%f, %f) to (%f, %f) thickness %f head %f colour %63s", name, &x0, &y0, &x1, &y1, &thickness, &head_size, colour_name);
	if (matched < 5 && sscanf(line, "arrow %63s", name) == 1)
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(to, " to (%f,%f)", &x1, &y1) == 2 || sscanf(to, " to (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5 && (strcmp(line, "arrow") == 0 || starts_with(line, "arrow from ")))
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f)", &x0, &y0) == 2 || sscanf(from, " from (%f, %f)", &x0, &y0) == 2) &&
			(sscanf(to, " to (%f,%f)", &x1, &y1) == 2 || sscanf(to, " to (%f, %f)", &x1, &y1) == 2))
			matched = 5;
	}
	if (matched < 5)
		return set_error(p, line_no, "expected arrow [name] from (x,y) to (x,y) thickness N head N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	if (strstr(line, " head "))
		sscanf(strstr(line, " head "), " head %f", &head_size);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	
	int id = wb_scene_add_arrow(p->scene, x0, y0, x1, y1, matched >= 6 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), matched >= 7 ? head_size : WB_DEFAULT_ARROW_HEAD_SIZE, parse_colour(matched >= 8 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create arrow object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_triangle_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "triangle %63s points (%f,%f) (%f,%f) (%f,%f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "triangle %63s points (%f, %f) (%f, %f) (%f, %f) thickness %f colour %63s", name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "tri %63s (%f,%f) (%f,%f) (%f,%f) t %f c %63s", name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "tri %63s (%f, %f) (%f, %f) (%f, %f) t %f c %63s", name, &x0, &y0, &x1, &y1, &x2, &y2, &thickness, colour_name);
	if (matched < 7 && (sscanf(line, "triangle %63s", name) == 1 || sscanf(line, "tri %63s", name) == 1))
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f) (%f,%f) (%f,%f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6 ||
			 sscanf(points, " points (%f, %f) (%f, %f) (%f, %f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6))
			matched = 7;
	}
	if (matched < 7 && (strcmp(line, "triangle") == 0 || strcmp(line, "tri") == 0 ||
		starts_with(line, "triangle points ") || starts_with(line, "tri points ")))
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f) (%f,%f) (%f,%f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6 ||
			 sscanf(points, " points (%f, %f) (%f, %f) (%f, %f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6))
			matched = 7;
	}
	if (matched < 7)
		return set_error(p, line_no, "expected triangle/tri [name] points (x,y) (x,y) (x,y) thickness N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_triangle(p->scene, x0, y0, x1, y1, x2, y2, matched >= 8 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), parse_colour(matched >= 9 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create triangle object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_triangle_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, opacity = effective_default_opacity(p, WB_DEFAULT_SHADE_OPACITY);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "shade_triangle %63s points (%f,%f) (%f,%f) (%f,%f) colour %63s opacity %f", name, &x0, &y0, &x1, &y1, &x2, &y2, colour_name, &opacity);
	if (matched < 7)
		matched = sscanf(line, "shade_triangle %63s points (%f, %f) (%f, %f) (%f, %f) colour %63s opacity %f", name, &x0, &y0, &x1, &y1, &x2, &y2, colour_name, &opacity);
	if (matched < 7 && sscanf(line, "shade_triangle %63s", name) == 1)
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f) (%f,%f) (%f,%f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6 ||
			 sscanf(points, " points (%f, %f) (%f, %f) (%f, %f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6))
			matched = 7;
	}
	if (matched < 7 && (strcmp(line, "shade_triangle") == 0 || starts_with(line, "shade_triangle points ")))
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f) (%f,%f) (%f,%f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6 ||
			 sscanf(points, " points (%f, %f) (%f, %f) (%f, %f)", &x0, &y0, &x1, &y1, &x2, &y2) == 6))
			matched = 7;
	}
	if (matched < 7)
		return set_error(p, line_no, "expected shade_triangle [name] points (x,y) (x,y) (x,y) colour name opacity A");
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	
	if (opacity < 0.0f)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	int id = wb_scene_add_shade_triangle(p->scene, x0, y0, x1, y1, x2, y2, parse_colour(matched >= 8 ? colour_name : effective_default_colour_name(p, "blue")), matched >= 9 ? opacity : effective_default_opacity(p, WB_DEFAULT_SHADE_OPACITY));
	if (!id)
		return set_error(p, line_no, "failed to create shade_triangle object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_quad_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, x3 = 0.0f, y3 = 0.0f, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));

	matched = sscanf(line, "quad %63s points (%f,%f) (%f,%f) (%f,%f) (%f,%f) thickness %f colour %63s",
		name, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3, &thickness, colour_name);
	if (matched < 9)
		matched = sscanf(line, "quad %63s points (%f, %f) (%f, %f) (%f, %f) (%f, %f) thickness %f colour %63s",
			name, &x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3, &thickness, colour_name);
	if (matched < 9 && sscanf(line, "quad %63s", name) == 1)
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f) (%f,%f) (%f,%f) (%f,%f)",
				&x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3) == 8 ||
			 sscanf(points, " points (%f, %f) (%f, %f) (%f, %f) (%f, %f)",
				&x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3) == 8))
			matched = 9;
	}
	if (matched < 9 && (strcmp(line, "quad") == 0 || starts_with(line, "quad points ")))
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f) (%f,%f) (%f,%f) (%f,%f)",
				&x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3) == 8 ||
			 sscanf(points, " points (%f, %f) (%f, %f) (%f, %f) (%f, %f)",
				&x0, &y0, &x1, &y1, &x2, &y2, &x3, &y3) == 8))
			matched = 9;
	}
	if (matched < 9)
		return set_error(p, line_no, "expected quad [name] points (x,y) (x,y) (x,y) (x,y) thickness N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);

	int id = wb_scene_add_quad(p->scene, x0, y0, x1, y1, x2, y2, x3, y3, matched >= 10 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), parse_colour(matched >= 11 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create quad object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_polygon_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	wb_vec2 points[7];
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	char *cursor;
	char *tok;
	int n_points = 0;
	int id = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	if (sscanf(line, "polygon %63s", name) != 1 && sscanf(line, "poly %63s", name) != 1)
	{
		if (strcmp(line, "polygon") != 0 && strcmp(line, "poly") != 0 &&
			!starts_with(line, "polygon points ") && !starts_with(line, "poly points "))
			return set_error(p, line_no, "expected polygon/poly [name] ...");
	}
	
	cursor = strstr(line, " points ");
	if (!cursor)
		cursor = strchr(line, ' ');
	if (!cursor)
		return set_error(p, line_no, "expected polygon/poly [name] points ...");
	if (starts_with(line, "polygon "))
		cursor = strstr(line, " points ");
	else
		cursor = strchr(cursor + 1, ' ');
	if (!cursor)
	{
		if (starts_with(line, "poly points "))
			cursor = strstr(line, " points ");
	}
	if (!cursor)
		return set_error(p, line_no, "expected polygon/poly [name] points ...");
	
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
		tok = strstr(line, " color ");
		if (tok)
			sscanf(tok, " color %63s", colour_name);
		else
		{
			tok = strstr(line, " c ");
			if (tok)
				sscanf(tok, " c %63s", colour_name);
		}
	}
	
	id = wb_scene_add_polygon(p->scene, points, n_points, thickness, parse_colour(colour_name));
	if (!id)
		return set_error(p, line_no, "failed to create polygon object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_polygon_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	wb_vec2 points[7];
	float opacity = effective_default_opacity(p, WB_DEFAULT_SHADE_OPACITY);
	char *cursor;
	char *tok;
	int n_points = 0;
	int id = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	if (sscanf(line, "shade_polygon %63s", name) != 1 && sscanf(line, "shade_poly %63s", name) != 1)
	{
		if (strcmp(line, "shade_polygon") != 0 && strcmp(line, "shade_poly") != 0 &&
			!starts_with(line, "shade_polygon points ") && !starts_with(line, "shade_poly points "))
			return set_error(p, line_no, "expected shade_polygon/shade_poly [name] ...");
	}
	
	cursor = strstr(line, " points ");
	if (!cursor)
		cursor = strchr(line, ' ');
	if (!cursor)
		return set_error(p, line_no, "expected shade_polygon/shade_poly [name] points ...");
	if (starts_with(line, "shade_polygon "))
		cursor = strstr(line, " points ");
	else
		cursor = strchr(cursor + 1, ' ');
	if (!cursor)
	{
		if (starts_with(line, "shade_poly points "))
			cursor = strstr(line, " points ");
	}
	if (!cursor)
		return set_error(p, line_no, "expected shade_polygon/shade_poly [name] points ...");
	
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
		tok = strstr(line, " color ");
		if (tok)
			sscanf(tok, " color %63s", colour_name);
		else
		{
			tok = strstr(line, " c ");
			if (tok)
				sscanf(tok, " c %63s", colour_name);
		}
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
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_blob_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	wb_vec2 points[7];
	int n_points = 0;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	if (sscanf(line, "blob %63s", name) != 1)
	{
		if (strcmp(line, "blob") != 0 && !starts_with(line, "blob points "))
			return set_error(p, line_no, "expected blob [name] ...");
		name[0] = 0;
	}
	if (strcmp(name, "points") == 0 || strcmp(name, "colour") == 0 || strcmp(name, "color") == 0 ||
		strcmp(name, "thickness") == 0 || strcmp(name, "t") == 0 || strcmp(name, "jitter") == 0)
		name[0] = 0;
	
	char *points_prop = strstr(line, " points ");
	if (!points_prop)
		return set_error(p, line_no, "expected blob [name] points ...");
	points_prop += 8;
	if (starts_with(line, "blob "))
	{
		char *after_name = strchr(line + 5, ' ');
		if (after_name && starts_with(after_name + 1, "points "))
			points_prop = after_name + 8;
	}
	while (n_points < 7)
	{
		int consumed = 0;
		float x = 0.0f, y = 0.0f;
		while (*points_prop == ' ' || *points_prop == '\t')
			points_prop++;
		if (sscanf(points_prop, "(%f,%f)%n", &x, &y, &consumed) == 2 ||
			sscanf(points_prop, "(%f, %f)%n", &x, &y, &consumed) == 2)
		{
			points[n_points++] = vec2(x, y);
			points_prop += consumed;
			continue;
		}
		break;
	}
	if (n_points < 3)
		return set_error(p, line_no, "expected blob with 3 to 7 control points");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_blob(p->scene, points, n_points, thickness, parse_colour(colour_name));
	if (!id)
		return set_error(p, line_no, "failed to create blob object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_blob_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	wb_vec2 points[7];
	int n_points = 0;
	float opacity = effective_default_opacity(p, WB_DEFAULT_SHADE_OPACITY);
	
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	if (sscanf(line, "shade_blob %63s", name) != 1)
	{
		if (strcmp(line, "shade_blob") != 0 && !starts_with(line, "shade_blob points "))
			return set_error(p, line_no, "expected shade_blob [name] ...");
		name[0] = 0;
	}
	if (strcmp(name, "points") == 0 || strcmp(name, "colour") == 0 || strcmp(name, "color") == 0 ||
		strcmp(name, "opacity") == 0 || strcmp(name, "jitter") == 0)
		name[0] = 0;
	
	char *points_prop = strstr(line, " points ");
	if (!points_prop)
		return set_error(p, line_no, "expected shade_blob [name] points ...");
	points_prop += 8;
	if (starts_with(line, "shade_blob "))
	{
		char *after_name = strchr(line + 11, ' ');
		if (after_name && starts_with(after_name + 1, "points "))
			points_prop = after_name + 8;
	}
	while (n_points < 7)
	{
		int consumed = 0;
		float x = 0.0f, y = 0.0f;
		while (*points_prop == ' ' || *points_prop == '\t')
			points_prop++;
		if (sscanf(points_prop, "(%f,%f)%n", &x, &y, &consumed) == 2 ||
			sscanf(points_prop, "(%f, %f)%n", &x, &y, &consumed) == 2)
		{
			points[n_points++] = vec2(x, y);
			points_prop += consumed;
			continue;
		}
		break;
	}
	if (n_points < 3)
		return set_error(p, line_no, "expected shade_blob with 3 to 7 control points");
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	
	int id = wb_scene_add_shade_blob(p->scene, points, n_points, parse_colour(colour_name), opacity);
	if (!id)
		return set_error(p, line_no, "failed to create shade_blob object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_line3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "line3d %63s from (%f,%f,%f) to (%f,%f,%f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &thickness, colour_name);
	if (matched < 7)
		matched = sscanf(line, "line3d %63s from (%f, %f, %f) to (%f, %f, %f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &thickness, colour_name);
	if (matched < 7 && sscanf(line, "line3d %63s", name) == 1)
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f,%f)", &x0, &y0, &z0) == 3 || sscanf(from, " from (%f, %f, %f)", &x0, &y0, &z0) == 3) &&
			(sscanf(to, " to (%f,%f,%f)", &x1, &y1, &z1) == 3 || sscanf(to, " to (%f, %f, %f)", &x1, &y1, &z1) == 3))
			matched = 7;
	}
	if (matched < 7 && (strcmp(line, "line3d") == 0 || starts_with(line, "line3d from ")))
	{
		char *from = strstr(line, " from ");
		char *to = strstr(line, " to ");
		if (from && to &&
			(sscanf(from, " from (%f,%f,%f)", &x0, &y0, &z0) == 3 || sscanf(from, " from (%f, %f, %f)", &x0, &y0, &z0) == 3) &&
			(sscanf(to, " to (%f,%f,%f)", &x1, &y1, &z1) == 3 || sscanf(to, " to (%f, %f, %f)", &x1, &y1, &z1) == 3))
			matched = 7;
	}
	if (matched < 7)
		return set_error(p, line_no, "expected line3d [name] from (x,y,z) to (x,y,z) thickness N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_line3d(p->scene, x0, y0, z0, x1, y1, z1, matched >= 8 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), parse_colour(matched >= 9 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create line3d object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_curve3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64] = "blue";
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, x2 = 0.0f, y2 = 0.0f, z2 = 0.0f, thickness = WB_DEFAULT_LINE_THICKNESS;
	float jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	int matched = 0;
	
	matched = sscanf(line, "curve3d %63s through (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10)
		matched = sscanf(line, "curve3d %63s through (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10 && sscanf(line, "curve3d %63s", name) == 1)
	{
		char *through = strstr(line, " through ");
		if (through &&
			(sscanf(through, " through (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9 ||
			 sscanf(through, " through (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9))
			matched = 10;
	}
	if (matched < 10 && (strcmp(line, "curve3d") == 0 || starts_with(line, "curve3d through ")))
	{
		char *through = strstr(line, " through ");
		if (through &&
			(sscanf(through, " through (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9 ||
			 sscanf(through, " through (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9))
			matched = 10;
	}
	if (matched < 10)
		return set_error(p, line_no, "expected curve3d [name] through (x,y,z) (x,y,z) (x,y,z) thickness N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_curve3d(p->scene, x0, y0, z0, x1, y1, z1, x2, y2, z2, matched >= 11 ? thickness : WB_DEFAULT_LINE_THICKNESS, parse_colour(matched >= 12 ? colour_name : "blue"));
	if (!id)
		return set_error(p, line_no, "failed to create curve3d object");
	if (parse_jitter_token(line, &jitter_strength))
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	remember_name(p, name, id);
	return 1;
}

static int parse_wire3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	wb_vec3 points[16];
	int n_points = 0;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	char *points_prop = NULL;
	
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	if (sscanf(line, "wire3d %63s", name) != 1 &&
		sscanf(line, "wireframe3d %63s", name) != 1 &&
		sscanf(line, "polygon3d %63s", name) != 1)
	{
		if (strcmp(line, "wire3d") != 0 && strcmp(line, "wireframe3d") != 0 &&
			strcmp(line, "polygon3d") != 0 &&
			!starts_with(line, "wire3d points ") &&
			!starts_with(line, "wireframe3d points ") &&
			!starts_with(line, "polygon3d points "))
			return set_error(p, line_no, "expected wire3d/wireframe3d/polygon3d [name] ...");
		name[0] = 0;
	}
	if (strcmp(name, "points") == 0 || strcmp(name, "colour") == 0 || strcmp(name, "color") == 0 ||
		strcmp(name, "thickness") == 0 || strcmp(name, "t") == 0 || strcmp(name, "jitter") == 0)
		name[0] = 0;
	
	points_prop = strstr(line, " points ");
	if (!points_prop)
		return set_error(p, line_no, "expected wire3d/wireframe3d/polygon3d [name] points ...");
	points_prop += 8;
	while (n_points < 16)
	{
		int consumed = 0;
		float x = 0.0f, y = 0.0f, z = 0.0f;
		while (*points_prop == ' ' || *points_prop == '\t')
			points_prop++;
		if (sscanf(points_prop, "(%f,%f,%f)%n", &x, &y, &z, &consumed) == 3 ||
			sscanf(points_prop, "(%f, %f, %f)%n", &x, &y, &z, &consumed) == 3)
		{
			points[n_points++] = vec3(x, y, z);
			points_prop += consumed;
			continue;
		}
		break;
	}
	if (n_points < 3)
		return set_error(p, line_no, "expected wire3d with at least 3 points");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_wire3d(p->scene, points, n_points, thickness, parse_colour(colour_name));
	if (!id)
		return set_error(p, line_no, "failed to create wire3d object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_poly3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	wb_vec3 points[16];
	int n_points = 0;
	float opacity = effective_default_opacity(p, WB_DEFAULT_SHADE_OPACITY);
	char *points_prop = NULL;
	
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	if (sscanf(line, "shade_poly3d %63s", name) != 1 &&
		sscanf(line, "shade_polygon3d %63s", name) != 1)
	{
		if (strcmp(line, "shade_poly3d") != 0 && strcmp(line, "shade_polygon3d") != 0 &&
			!starts_with(line, "shade_poly3d points ") &&
			!starts_with(line, "shade_polygon3d points "))
			return set_error(p, line_no, "expected shade_poly3d/shade_polygon3d [name] ...");
		name[0] = 0;
	}
	if (strcmp(name, "points") == 0 || strcmp(name, "colour") == 0 || strcmp(name, "color") == 0 ||
		strcmp(name, "opacity") == 0 || strcmp(name, "jitter") == 0)
		name[0] = 0;
	
	points_prop = strstr(line, " points ");
	if (!points_prop)
		return set_error(p, line_no, "expected shade_poly3d/shade_polygon3d [name] points ...");
	points_prop += 8;
	while (n_points < 16)
	{
		int consumed = 0;
		float x = 0.0f, y = 0.0f, z = 0.0f;
		while (*points_prop == ' ' || *points_prop == '\t')
			points_prop++;
		if (sscanf(points_prop, "(%f,%f,%f)%n", &x, &y, &z, &consumed) == 3 ||
			sscanf(points_prop, "(%f, %f, %f)%n", &x, &y, &z, &consumed) == 3)
		{
			points[n_points++] = vec3(x, y, z);
			points_prop += consumed;
			continue;
		}
		break;
	}
	if (n_points < 3)
		return set_error(p, line_no, "expected shade_poly3d with at least 3 points");
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	int id = wb_scene_add_shade_poly3d(p->scene, points, n_points, parse_colour(colour_name), opacity);
	if (!id)
		return set_error(p, line_no, "failed to create shade_poly3d object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static wb_vec3 bilerp3(wb_vec3 p00, wb_vec3 p10, wb_vec3 p11, wb_vec3 p01, float u, float v)
{
	float a00 = (1.0f - u) * (1.0f - v);
	float a10 = u * (1.0f - v);
	float a11 = u * v;
	float a01 = (1.0f - u) * v;
	return vec3(
		p00.x * a00 + p10.x * a10 + p11.x * a11 + p01.x * a01,
		p00.y * a00 + p10.y * a10 + p11.y * a11 + p01.y * a01,
		p00.z * a00 + p10.z * a10 + p11.z * a11 + p01.z * a01);
}

static int parse_mesh3d_vertices(const char *s, wb_vec3 *vertices, int max_vertices)
{
	int n = 0;
	const char *cursor = s;
	
	if (!s || !vertices || max_vertices <= 0)
		return 0;
	while (cursor && *cursor)
	{
		float x = 0.0f, y = 0.0f, z = 0.0f;
		int consumed = 0;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		if (*cursor == 0)
			break;
		if (starts_with_word(cursor, "faces") || starts_with_word(cursor, "colour") ||
			starts_with_word(cursor, "color") || starts_with_word(cursor, "thickness") ||
			starts_with_word(cursor, "opacity") || starts_with_word(cursor, "jitter"))
			break;
		if ((sscanf(cursor, "(%f,%f,%f)%n", &x, &y, &z, &consumed) == 3 ||
			sscanf(cursor, "(%f, %f, %f)%n", &x, &y, &z, &consumed) == 3) && consumed > 0)
		{
			if (n >= max_vertices)
				return -1;
			vertices[n++] = vec3(x, y, z);
			cursor += consumed;
			continue;
		}
		return 0;
	}
	return n;
}

static int parse_mesh3d_faces(const char *s, int faces[][4], int *face_sizes, int max_faces)
{
	int n = 0;
	const char *cursor = s;
	
	if (!s || !faces || !face_sizes || max_faces <= 0)
		return 0;
	while (cursor && *cursor)
	{
		int a = -1, b = -1, c = -1, d = -1;
		int consumed = 0;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		if (*cursor == 0)
			break;
		if (starts_with_word(cursor, "colour") || starts_with_word(cursor, "color") ||
			starts_with_word(cursor, "thickness") || starts_with_word(cursor, "opacity") ||
			starts_with_word(cursor, "jitter"))
			break;
		if (*cursor != '[')
			return 0;
		cursor++;
		consumed = 0;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		a = (int)strtol(cursor, (char **)&cursor, 10);
		consumed++;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		if (*cursor != ',')
			return 0;
		cursor++;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		b = (int)strtol(cursor, (char **)&cursor, 10);
		consumed++;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		if (*cursor != ',')
			return 0;
		cursor++;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		c = (int)strtol(cursor, (char **)&cursor, 10);
		consumed++;
		while (*cursor == ' ' || *cursor == '\t')
			cursor++;
		d = -1;
		if (*cursor == ',')
		{
			cursor++;
			while (*cursor == ' ' || *cursor == '\t')
				cursor++;
			d = (int)strtol(cursor, (char **)&cursor, 10);
			consumed++;
			while (*cursor == ' ' || *cursor == '\t')
				cursor++;
		}
		if (*cursor != ']')
			return 0;
		cursor++;
		if (n >= max_faces)
			return -1;
		faces[n][0] = a;
		faces[n][1] = b;
		faces[n][2] = c;
		faces[n][3] = d;
		face_sizes[n++] = consumed;
	}
	return n;
}

static int add_mesh3d_objects(wb_spec_parser *p, const char *name,
	const wb_vec3 *vertices, int n_vertices,
	const int faces[][4], const int *face_sizes, int n_faces,
	float thickness, uint32_t colour, float opacity, float jitter_strength)
{
	int id = 0;
	int edges[256][2];
	int n_edges = 0;
	
	if (!p || !name || !*name || !vertices || n_vertices < 3 || !faces || !face_sizes || n_faces < 1)
		return 0;
	
	if (opacity > 0.0f)
	{
		for (int i = 0; i < n_faces; i++)
		{
			wb_vec3 poly[4];
			for (int j = 0; j < face_sizes[i]; j++)
				poly[j] = vertices[faces[i][j]];
			id = wb_scene_add_shade_poly3d(p->scene, poly, face_sizes[i], colour, opacity);
			if (!id)
				return 0;
			remember_group_member(p, name, id);
			wb_scene_set_object_jitter(p->scene, id, jitter_strength);
		}
	}
	
	for (int i = 0; i < n_faces; i++)
	{
		for (int j = 0; j < face_sizes[i]; j++)
		{
			int a = faces[i][j];
			int b = faces[i][(j + 1) % face_sizes[i]];
			int e0 = a < b ? a : b;
			int e1 = a < b ? b : a;
			int seen = 0;
			for (int k = 0; k < n_edges; k++)
			{
				if (edges[k][0] == e0 && edges[k][1] == e1)
				{
					seen = 1;
					break;
				}
			}
			if (!seen)
			{
				if (n_edges >= (int)(sizeof(edges) / sizeof(edges[0])))
					return 0;
				edges[n_edges][0] = e0;
				edges[n_edges][1] = e1;
				n_edges++;
			}
		}
	}
	
	for (int i = 0; i < n_edges; i++)
	{
		id = wb_scene_add_line3d(
			p->scene,
			vertices[edges[i][0]].x, vertices[edges[i][0]].y, vertices[edges[i][0]].z,
			vertices[edges[i][1]].x, vertices[edges[i][1]].y, vertices[edges[i][1]].z,
			thickness, colour);
		if (!id)
			return 0;
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static int add_surface3d_objects(wb_spec_parser *p, const char *name,
	wb_vec3 p00, wb_vec3 p10, wb_vec3 p11, wb_vec3 p01,
	int u_steps, int v_steps, float thickness, uint32_t colour, float opacity, float jitter_strength)
{
	int id = 0;
	char part_name[64];
	wb_vec3 row[32];
	wb_vec3 col[32];
	
	if (!p || !name || !*name)
		return 0;
	if (u_steps < 1)
		u_steps = 1;
	if (v_steps < 1)
		v_steps = 1;
	if (u_steps > 16)
		u_steps = 16;
	if (v_steps > 16)
		v_steps = 16;
	
	if (opacity > 0.0f)
	{
		for (int j = 0; j < v_steps; j++)
		{
			for (int i = 0; i < u_steps; i++)
			{
				float u0 = (float)i / (float)u_steps;
				float u1 = (float)(i + 1) / (float)u_steps;
				float v0 = (float)j / (float)v_steps;
				float v1 = (float)(j + 1) / (float)v_steps;
				wb_vec3 quad[4] = {
					bilerp3(p00, p10, p11, p01, u0, v0),
					bilerp3(p00, p10, p11, p01, u1, v0),
					bilerp3(p00, p10, p11, p01, u1, v1),
					bilerp3(p00, p10, p11, p01, u0, v1),
				};
				id = wb_scene_add_shade_poly3d(p->scene, quad, 4, colour, opacity);
				if (!id)
					return 0;
				remember_group_member(p, name, id);
			}
		}
	}
	
	for (int j = 0; j <= v_steps; j++)
	{
		float v = (float)j / (float)v_steps;
		for (int i = 0; i <= u_steps; i++)
		{
			float u = (float)i / (float)u_steps;
			row[i] = bilerp3(p00, p10, p11, p01, u, v);
		}
		id = wb_scene_add_wire3d(p->scene, row, u_steps + 1, thickness, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_row_%d", name, j);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	for (int i = 0; i <= u_steps; i++)
	{
		float u = (float)i / (float)u_steps;
		for (int j = 0; j <= v_steps; j++)
		{
			float v = (float)j / (float)v_steps;
			col[j] = bilerp3(p00, p10, p11, p01, u, v);
		}
		id = wb_scene_add_wire3d(p->scene, col, v_steps + 1, thickness, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_col_%d", name, i);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static wb_vec3 sample_blob3d_point(wb_vec3 center, wb_vec3 radii, float u, float v, float wobble)
{
	float cu = cosf(u);
	float su = sinf(u);
	float cv = cosf(v);
	float sv = sinf(v);
	float bump = 1.0f + wobble * (0.18f * sinf(3.0f * u + 0.7f) * cosf(2.0f * v - 0.4f) +
		0.12f * sinf(5.0f * v + 1.3f) +
		0.07f * cosf(4.0f * u - 3.0f * v + 0.5f));
	return vec3(
		center.x + radii.x * bump * cv * cu,
		center.y + radii.y * bump * sv,
		center.z + radii.z * bump * cv * su);
}

static int add_blob3d_objects(wb_spec_parser *p, const char *name,
	wb_vec3 center, wb_vec3 radii, int u_steps, int v_steps, float wobble,
	float thickness, uint32_t colour, float opacity, float jitter_strength)
{
	int id = 0;
	char part_name[64];
	wb_vec3 row[48];
	wb_vec3 col[32];
	
	if (!p || !name || !*name)
		return 0;
	if (u_steps < 3)
		u_steps = 3;
	if (v_steps < 2)
		v_steps = 2;
	if (u_steps > 24)
		u_steps = 24;
	if (v_steps > 12)
		v_steps = 12;
	if (wobble < 0.0f)
		wobble = 0.0f;
	if (wobble > 0.45f)
		wobble = 0.45f;
	
	if (opacity > 0.0f)
	{
		for (int j = 0; j < v_steps; j++)
		{
			float v0 = -0.5f * PI + (float)j * PI / (float)v_steps;
			float v1 = -0.5f * PI + (float)(j + 1) * PI / (float)v_steps;
			for (int i = 0; i < u_steps; i++)
			{
				float u0 = (float)i * TAU / (float)u_steps;
				float u1 = (float)(i + 1) * TAU / (float)u_steps;
				wb_vec3 quad[4] = {
					sample_blob3d_point(center, radii, u0, v0, wobble),
					sample_blob3d_point(center, radii, u1, v0, wobble),
					sample_blob3d_point(center, radii, u1, v1, wobble),
					sample_blob3d_point(center, radii, u0, v1, wobble),
				};
				id = wb_scene_add_shade_poly3d(p->scene, quad, 4, colour, opacity);
				if (!id)
					return 0;
				remember_group_member(p, name, id);
				wb_scene_set_object_jitter(p->scene, id, jitter_strength);
			}
		}
	}
	
	for (int j = 0; j <= v_steps; j++)
	{
		float v = -0.5f * PI + (float)j * PI / (float)v_steps;
		for (int i = 0; i <= u_steps; i++)
		{
			float u = (float)i * TAU / (float)u_steps;
			row[i] = sample_blob3d_point(center, radii, u, v, wobble);
		}
		id = wb_scene_add_wire3d(p->scene, row, u_steps + 1, thickness, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_row_%d", name, j);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	for (int i = 0; i < u_steps; i++)
	{
		float u = (float)i * TAU / (float)u_steps;
		for (int j = 0; j <= v_steps; j++)
		{
			float v = -0.5f * PI + (float)j * PI / (float)v_steps;
			col[j] = sample_blob3d_point(center, radii, u, v, wobble);
		}
		id = wb_scene_add_wire3d(p->scene, col, v_steps + 1, thickness, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_col_%d", name, i);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static int add_volume3d_objects(wb_spec_parser *p, const char *name,
	wb_vec3 center, wb_vec3 radii, int shells, int u_steps, int v_steps, float wobble,
	float thickness, uint32_t colour, float opacity, float jitter_strength)
{
	char shell_name[64];
	
	if (!p || !name || !*name)
		return 0;
	if (shells < 2)
		shells = 2;
	if (shells > 8)
		shells = 8;
	for (int i = 0; i < shells; i++)
	{
		float a = (float)(i + 1) / (float)shells;
		float local_opacity = opacity / (float)shells;
		float local_wobble = wobble * (0.6f + 0.4f * a);
		wb_vec3 local_radii = vec3_scale(a, radii);
		int objects_before = p->scene->n_objects;
		snprintf(shell_name, sizeof(shell_name), "%s_shell_%d", name, i);
		if (!add_blob3d_objects(p, shell_name, center, local_radii, u_steps, v_steps, local_wobble,
			thickness, colour, local_opacity, jitter_strength))
			return 0;
		for (int j = objects_before; j < p->scene->n_objects; j++)
			remember_group_member(p, name, p->scene->objects[j].id);
	}
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static int parse_mesh3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	char verts_text[1024];
	char faces_text[1024];
	wb_vec3 vertices[32];
	int faces[64][4];
	int face_sizes[64];
	int n_vertices = 0;
	int n_faces = 0;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	float opacity = effective_default_opacity(p, 0.10f);
	float jitter_strength = effective_default_jitter(p, WB_DEFAULT_OBJECT_JITTER_STRENGTH);
	char *verts_prop = NULL;
	char *faces_prop = NULL;
	char *stop = NULL;
	
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	if (sscanf(line, "mesh3d %63s", name) != 1)
	{
		if (strcmp(line, "mesh3d") != 0 && !starts_with(line, "mesh3d vertices "))
			return set_error(p, line_no, "expected mesh3d [name] ...");
	}
	verts_prop = strstr(line, " vertices ");
	if (!verts_prop)
		verts_prop = strstr(line, " verts ");
	faces_prop = strstr(line, " faces ");
	if (!verts_prop || !faces_prop)
		return set_error(p, line_no, "expected mesh3d [name] vertices (...) (...) ... faces [i,j,k] ...");
	memset(verts_text, 0, sizeof(verts_text));
	memset(faces_text, 0, sizeof(faces_text));
	if (starts_with(verts_prop, " vertices "))
		verts_prop += 10;
	else
		verts_prop += 7;
	stop = faces_prop;
	while (stop > verts_prop && (*(stop - 1) == ' ' || *(stop - 1) == '\t'))
		stop--;
	snprintf(verts_text, binary_min((int)sizeof(verts_text), (int)(stop - verts_prop) + 1), "%s", verts_prop);
	faces_prop += 7;
	stop = strstr(faces_prop, " thickness ");
	if (!stop) stop = strstr(faces_prop, " colour ");
	if (!stop) stop = strstr(faces_prop, " color ");
	if (!stop) stop = strstr(faces_prop, " c ");
	if (!stop) stop = strstr(faces_prop, " opacity ");
	if (!stop) stop = strstr(faces_prop, " jitter ");
	if (!stop) stop = faces_prop + strlen(faces_prop);
	while (stop > faces_prop && (*(stop - 1) == ' ' || *(stop - 1) == '\t'))
		stop--;
	snprintf(faces_text, binary_min((int)sizeof(faces_text), (int)(stop - faces_prop) + 1), "%s", faces_prop);
	n_vertices = parse_mesh3d_vertices(verts_text, vertices, 32);
	if (n_vertices < 3)
		return set_error(p, line_no, "expected mesh3d with at least 3 vertices");
	n_faces = parse_mesh3d_faces(faces_text, faces, face_sizes, 64);
	if (n_faces < 1)
		return set_error(p, line_no, "expected mesh3d with at least 1 face");
	for (int i = 0; i < n_faces; i++)
	{
		for (int j = 0; j < face_sizes[i]; j++)
		{
			if (faces[i][j] < 0 || faces[i][j] >= n_vertices)
				return set_error(p, line_no, "mesh3d face index out of range");
		}
	}
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	ensure_object_name(p, name, sizeof(name), "mesh3d");
	if (!add_mesh3d_objects(p, name, vertices, n_vertices, faces, face_sizes, n_faces,
		thickness, parse_colour(colour_name), opacity, jitter_strength))
		return set_error(p, line_no, "failed to create mesh3d object");
	return 1;
}

static wb_vec3 sample_param3d_point(const char *family, wb_vec3 center, float rx, float ry, float height, float turns, float phase, float freq, float t)
{
	if (strcmp(family, "circle") == 0)
	{
		float a = phase + TAU * t;
		return vec3(center.x + rx * cosf(a), center.y + ry * sinf(a), center.z);
	}
	if (strcmp(family, "lissajous") == 0)
	{
		float a = phase + TAU * t;
		return vec3(
			center.x + rx * sinf(freq * a),
			center.y + ry * sinf((freq + 1.0f) * a + 0.7f),
			center.z + height * cosf((freq + 2.0f) * a + 0.3f));
	}
	/* default helix */
	{
		float a = phase + TAU * turns * t;
		return vec3(
			center.x + rx * cosf(a),
			center.y + ry * sinf(a),
			center.z + height * (t - 0.5f));
	}
}

static int add_param3d_objects(wb_spec_parser *p, const char *name, const char *family,
	wb_vec3 center, float rx, float ry, float height, float turns, float phase, float freq, int steps,
	float thickness, uint32_t colour, float jitter_strength)
{
	int id = 0;
	char part_name[64];
	wb_vec3 samples[65];
	
	if (!p || !name || !*name || !family)
		return 0;
	if (steps < 4)
		steps = 4;
	if (steps > 64)
		steps = 64;
	for (int i = 0; i <= steps; i++)
		samples[i] = sample_param3d_point(family, center, rx, ry, height, turns, phase, freq, (float)i / (float)steps);
	if (steps >= 2)
	{
		wb_vec3 first_mid = vec3_scale(0.5f, vec3_sum(samples[0], samples[1]));
		id = wb_scene_add_curve3d(p->scene,
			samples[0].x, samples[0].y, samples[0].z,
			first_mid.x, first_mid.y, first_mid.z,
			samples[1].x, samples[1].y, samples[1].z,
			thickness, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_seg_%d", name, 0);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	for (int i = 1; i < steps; i++)
	{
		wb_vec3 m0 = vec3_scale(0.5f, vec3_sum(samples[i - 1], samples[i]));
		wb_vec3 m1 = vec3_scale(0.5f, vec3_sum(samples[i], samples[i + 1]));
		id = wb_scene_add_curve3d(p->scene,
			m0.x, m0.y, m0.z,
			samples[i].x, samples[i].y, samples[i].z,
			m1.x, m1.y, m1.z,
			thickness, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_seg_%d", name, i);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	if (steps >= 2)
	{
		wb_vec3 last_mid = vec3_scale(0.5f, vec3_sum(samples[steps - 1], samples[steps]));
		id = wb_scene_add_curve3d(p->scene,
			samples[steps - 1].x, samples[steps - 1].y, samples[steps - 1].z,
			last_mid.x, last_mid.y, last_mid.z,
			samples[steps].x, samples[steps].y, samples[steps].z,
			thickness, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_seg_%d", name, steps);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static int parse_param3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char family[32] = "helix";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float rx = 1.0f, ry = 1.0f;
	float radius = -1.0f;
	float height = 1.5f;
	float turns = 2.0f;
	float phase = 0.0f;
	float freq = 2.0f;
	int steps = 20;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	float jitter_strength = effective_default_jitter(p, WB_DEFAULT_OBJECT_JITTER_STRENGTH);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "param3d %63s at (%f,%f,%f)", name, &x, &y, &z);
	if (matched < 4)
		matched = sscanf(line, "param3d %63s at (%f, %f, %f)", name, &x, &y, &z);
	if (matched < 4)
		matched = sscanf(line, "parametric3d %63s at (%f,%f,%f)", name, &x, &y, &z);
	if (matched < 4)
		matched = sscanf(line, "parametric3d %63s at (%f, %f, %f)", name, &x, &y, &z);
	if (matched < 4 && (sscanf(line, "param3d %63s", name) == 1 || sscanf(line, "parametric3d %63s", name) == 1))
	{
		char *at = strstr(line, " at ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 ||
			 sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4 && (strcmp(line, "param3d") == 0 || strcmp(line, "parametric3d") == 0 || starts_with(line, "param3d at ") || starts_with(line, "parametric3d at ")))
	{
		char *at = strstr(line, " at ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 ||
			 sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected param3d/parametric3d [name] at (x,y,z)");
	if (strstr(line, " family "))
		sscanf(strstr(line, " family "), " family %31s", family);
	else if (strstr(line, " type "))
		sscanf(strstr(line, " type "), " type %31s", family);
	if (strstr(line, " radius "))
		sscanf(strstr(line, " radius "), " radius %f", &radius);
	else if (strstr(line, " r "))
		sscanf(strstr(line, " r "), " r %f", &radius);
	if (radius > 0.0f)
		rx = ry = radius;
	if (strstr(line, " radii "))
	{
		if (sscanf(strstr(line, " radii "), " radii (%f,%f)", &rx, &ry) != 2)
			sscanf(strstr(line, " radii "), " radii (%f, %f)", &rx, &ry);
	}
	if (strstr(line, " rx "))
		sscanf(strstr(line, " rx "), " rx %f", &rx);
	if (strstr(line, " ry "))
		sscanf(strstr(line, " ry "), " ry %f", &ry);
	if (strstr(line, " height "))
		sscanf(strstr(line, " height "), " height %f", &height);
	else if (strstr(line, " pitch "))
		sscanf(strstr(line, " pitch "), " pitch %f", &height);
	if (strstr(line, " turns "))
		sscanf(strstr(line, " turns "), " turns %f", &turns);
	if (strstr(line, " phase "))
		sscanf(strstr(line, " phase "), " phase %f", &phase);
	if (strstr(line, " freq "))
		sscanf(strstr(line, " freq "), " freq %f", &freq);
	if (strstr(line, " steps "))
		sscanf(strstr(line, " steps "), " steps %d", &steps);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (rx <= 0.0f) rx = 1.0f;
	if (ry <= 0.0f) ry = rx;
	ensure_object_name(p, name, sizeof(name), "param3d");
	if (!add_param3d_objects(p, name, family, vec3(x, y, z), rx, ry, height, turns, phase, freq, steps,
		thickness, parse_colour(colour_name), jitter_strength))
		return set_error(p, line_no, "failed to create param3d object");
	return 1;
}

static wb_vec3 sample_param_surface3d_point(const char *family, wb_vec3 center,
	float rx, float ry, float rz, float major, float minor, float amp, float freq, float phase,
	float u, float v)
{
	if (strcmp(family, "torus") == 0)
	{
		float a = TAU * u + phase;
		float b = TAU * v;
		float ring = major + minor * cosf(b);
		return vec3(
			center.x + ring * cosf(a),
			center.y + rz * minor * sinf(b),
			center.z + ring * sinf(a));
	}
	if (strcmp(family, "saddle") == 0)
	{
		float x = (u - 0.5f) * 2.0f * rx;
		float z = (v - 0.5f) * 2.0f * ry;
		return vec3(center.x + x, center.y + amp * (x * x - z * z), center.z + z);
	}
	/* default wave sheet */
	{
		float x = (u - 0.5f) * 2.0f * rx;
		float z = (v - 0.5f) * 2.0f * ry;
		return vec3(
			center.x + x,
			center.y + amp * sinf(freq * PI * u + phase) * cosf(freq * PI * v - 0.4f),
			center.z + z);
	}
}

static int add_param_surface3d_objects(wb_spec_parser *p, const char *name, const char *family,
	wb_vec3 center, float rx, float ry, float rz, float major, float minor, float amp, float freq, float phase,
	int u_steps, int v_steps, float thickness, uint32_t colour, float opacity, float jitter_strength)
{
	int id = 0;
	char part_name[64];
	wb_vec3 row[33];
	wb_vec3 col[33];
	int wrap_u = strcmp(family, "torus") == 0;
	
	if (!p || !name || !*name || !family)
		return 0;
	if (u_steps < 2)
		u_steps = 2;
	if (v_steps < 2)
		v_steps = 2;
	if (u_steps > 32)
		u_steps = 32;
	if (v_steps > 32)
		v_steps = 32;
	
	if (opacity > 0.0f)
	{
		for (int j = 0; j < v_steps; j++)
		{
			for (int i = 0; i < u_steps; i++)
			{
				float u0 = (float)i / (float)u_steps;
				float u1 = (float)(i + 1) / (float)u_steps;
				float v0 = (float)j / (float)v_steps;
				float v1 = (float)(j + 1) / (float)v_steps;
				wb_vec3 quad[4] = {
					sample_param_surface3d_point(family, center, rx, ry, rz, major, minor, amp, freq, phase, u0, v0),
					sample_param_surface3d_point(family, center, rx, ry, rz, major, minor, amp, freq, phase, wrap_u && i + 1 == u_steps ? 0.0f : u1, v0),
					sample_param_surface3d_point(family, center, rx, ry, rz, major, minor, amp, freq, phase, wrap_u && i + 1 == u_steps ? 0.0f : u1, v1),
					sample_param_surface3d_point(family, center, rx, ry, rz, major, minor, amp, freq, phase, u0, v1),
				};
				id = wb_scene_add_shade_poly3d(p->scene, quad, 4, colour, opacity);
				if (!id)
					return 0;
				remember_group_member(p, name, id);
				wb_scene_set_object_jitter(p->scene, id, jitter_strength);
			}
		}
	}
	
	for (int j = 0; j <= v_steps; j++)
	{
		float v = (float)j / (float)v_steps;
		for (int i = 0; i <= u_steps; i++)
		{
			float u = wrap_u && i == u_steps ? 0.0f : (float)i / (float)u_steps;
			row[i] = sample_param_surface3d_point(family, center, rx, ry, rz, major, minor, amp, freq, phase, u, v);
		}
		id = wb_scene_add_wire3d(p->scene, row, u_steps + 1, thickness, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_row_%d", name, j);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	for (int i = 0; i <= u_steps; i++)
	{
		float u = wrap_u && i == u_steps ? 0.0f : (float)i / (float)u_steps;
		for (int j = 0; j <= v_steps; j++)
		{
			float v = (float)j / (float)v_steps;
			col[j] = sample_param_surface3d_point(family, center, rx, ry, rz, major, minor, amp, freq, phase, u, v);
		}
		id = wb_scene_add_wire3d(p->scene, col, v_steps + 1, thickness, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_col_%d", name, i);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static int parse_param_surface3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char family[32] = "wave";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float rx = 1.0f, ry = 1.0f, rz = 1.0f;
	float radius = -1.0f;
	float major = 1.2f, minor = 0.35f;
	float amp = 0.35f, freq = 2.0f, phase = 0.0f;
	int u_steps = 10, v_steps = 8;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	float opacity = effective_default_opacity(p, 0.10f);
	float jitter_strength = effective_default_jitter(p, WB_DEFAULT_OBJECT_JITTER_STRENGTH);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "param_surface3d %63s at (%f,%f,%f)", name, &x, &y, &z);
	if (matched < 4)
		matched = sscanf(line, "param_surface3d %63s at (%f, %f, %f)", name, &x, &y, &z);
	if (matched < 4)
		matched = sscanf(line, "parametric_surface3d %63s at (%f,%f,%f)", name, &x, &y, &z);
	if (matched < 4)
		matched = sscanf(line, "parametric_surface3d %63s at (%f, %f, %f)", name, &x, &y, &z);
	if (matched < 4 && (sscanf(line, "param_surface3d %63s", name) == 1 || sscanf(line, "parametric_surface3d %63s", name) == 1))
	{
		char *at = strstr(line, " at ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 ||
			 sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4 && (strcmp(line, "param_surface3d") == 0 || strcmp(line, "parametric_surface3d") == 0 || starts_with(line, "param_surface3d at ") || starts_with(line, "parametric_surface3d at ")))
	{
		char *at = strstr(line, " at ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 ||
			 sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected param_surface3d/parametric_surface3d [name] at (x,y,z)");
	if (strstr(line, " family "))
		sscanf(strstr(line, " family "), " family %31s", family);
	else if (strstr(line, " type "))
		sscanf(strstr(line, " type "), " type %31s", family);
	if (strstr(line, " radius "))
		sscanf(strstr(line, " radius "), " radius %f", &radius);
	else if (strstr(line, " r "))
		sscanf(strstr(line, " r "), " r %f", &radius);
	if (radius > 0.0f)
		rx = ry = rz = radius;
	if (strstr(line, " radii "))
	{
		if (sscanf(strstr(line, " radii "), " radii (%f,%f,%f)", &rx, &ry, &rz) != 3)
			sscanf(strstr(line, " radii "), " radii (%f, %f, %f)", &rx, &ry, &rz);
	}
	if (strstr(line, " rx "))
		sscanf(strstr(line, " rx "), " rx %f", &rx);
	if (strstr(line, " ry "))
		sscanf(strstr(line, " ry "), " ry %f", &ry);
	if (strstr(line, " rz "))
		sscanf(strstr(line, " rz "), " rz %f", &rz);
	if (strstr(line, " major "))
		sscanf(strstr(line, " major "), " major %f", &major);
	if (strstr(line, " minor "))
		sscanf(strstr(line, " minor "), " minor %f", &minor);
	if (strstr(line, " height "))
		sscanf(strstr(line, " height "), " height %f", &amp);
	else if (strstr(line, " amp "))
		sscanf(strstr(line, " amp "), " amp %f", &amp);
	if (strstr(line, " freq "))
		sscanf(strstr(line, " freq "), " freq %f", &freq);
	if (strstr(line, " phase "))
		sscanf(strstr(line, " phase "), " phase %f", &phase);
	if (strstr(line, " u_steps "))
		sscanf(strstr(line, " u_steps "), " u_steps %d", &u_steps);
	else if (strstr(line, " u "))
		sscanf(strstr(line, " u "), " u %d", &u_steps);
	if (strstr(line, " v_steps "))
		sscanf(strstr(line, " v_steps "), " v_steps %d", &v_steps);
	else if (strstr(line, " v "))
		sscanf(strstr(line, " v "), " v %d", &v_steps);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	if (rx <= 0.0f) rx = 1.0f;
	if (ry <= 0.0f) ry = rx;
	if (rz <= 0.0f) rz = rx;
	ensure_object_name(p, name, sizeof(name), "param_surface3d");
	if (!add_param_surface3d_objects(p, name, family, vec3(x, y, z), rx, ry, rz, major, minor, amp, freq, phase, u_steps, v_steps,
		thickness, parse_colour(colour_name), opacity, jitter_strength))
		return set_error(p, line_no, "failed to create param_surface3d object");
	return 1;
}

static int parse_volume3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float rx = 1.0f, ry = 0.8f, rz = 0.9f;
	float radius = -1.0f;
	int shells = 4;
	int u_steps = 10;
	int v_steps = 6;
	float wobble = 0.12f;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	float opacity = effective_default_opacity(p, 0.16f);
	float jitter_strength = effective_default_jitter(p, WB_DEFAULT_OBJECT_JITTER_STRENGTH);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "volume3d %63s at (%f,%f,%f) radius %f", name, &x, &y, &z, &radius);
	if (matched < 5)
		matched = sscanf(line, "volume3d %63s at (%f, %f, %f) radius %f", name, &x, &y, &z, &radius);
	if (matched < 5)
		matched = sscanf(line, "ellipsoid3d %63s at (%f,%f,%f) radius %f", name, &x, &y, &z, &radius);
	if (matched < 5)
		matched = sscanf(line, "ellipsoid3d %63s at (%f, %f, %f) radius %f", name, &x, &y, &z, &radius);
	if (matched < 4)
		matched = sscanf(line, "volume3d %63s at (%f,%f,%f)", name, &x, &y, &z);
	if (matched < 4)
		matched = sscanf(line, "volume3d %63s at (%f, %f, %f)", name, &x, &y, &z);
	if (matched < 4 && (sscanf(line, "volume3d %63s", name) == 1 || sscanf(line, "ellipsoid3d %63s", name) == 1))
	{
		char *at = strstr(line, " at ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 ||
			 sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected volume3d/ellipsoid3d [name] at (x,y,z) [radius R]");
	if (strstr(line, " radius "))
		sscanf(strstr(line, " radius "), " radius %f", &radius);
	else if (strstr(line, " r "))
		sscanf(strstr(line, " r "), " r %f", &radius);
	if (radius > 0.0f)
		rx = ry = rz = radius;
	if (strstr(line, " radii "))
	{
		if (sscanf(strstr(line, " radii "), " radii (%f,%f,%f)", &rx, &ry, &rz) != 3)
			sscanf(strstr(line, " radii "), " radii (%f, %f, %f)", &rx, &ry, &rz);
	}
	if (strstr(line, " rx "))
		sscanf(strstr(line, " rx "), " rx %f", &rx);
	if (strstr(line, " ry "))
		sscanf(strstr(line, " ry "), " ry %f", &ry);
	if (strstr(line, " rz "))
		sscanf(strstr(line, " rz "), " rz %f", &rz);
	if (strstr(line, " shells "))
		sscanf(strstr(line, " shells "), " shells %d", &shells);
	else if (strstr(line, " density "))
		sscanf(strstr(line, " density "), " density %d", &shells);
	if (strstr(line, " u_steps "))
		sscanf(strstr(line, " u_steps "), " u_steps %d", &u_steps);
	else if (strstr(line, " u "))
		sscanf(strstr(line, " u "), " u %d", &u_steps);
	if (strstr(line, " v_steps "))
		sscanf(strstr(line, " v_steps "), " v_steps %d", &v_steps);
	else if (strstr(line, " v "))
		sscanf(strstr(line, " v "), " v %d", &v_steps);
	if (strstr(line, " wobble "))
		sscanf(strstr(line, " wobble "), " wobble %f", &wobble);
	else if (strstr(line, " w "))
		sscanf(strstr(line, " w "), " w %f", &wobble);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	if (rx <= 0.0f) rx = 1.0f;
	if (ry <= 0.0f) ry = rx;
	if (rz <= 0.0f) rz = rx;
	ensure_object_name(p, name, sizeof(name), "volume3d");
	if (!add_volume3d_objects(p, name, vec3(x, y, z), vec3(rx, ry, rz), shells, u_steps, v_steps, wobble,
		thickness, parse_colour(colour_name), opacity, jitter_strength))
		return set_error(p, line_no, "failed to create volume3d object");
	return 1;
}

static int parse_blob3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float rx = 1.0f, ry = 0.8f, rz = 0.9f;
	float radius = -1.0f;
	int u_steps = 10;
	int v_steps = 6;
	float wobble = 0.18f;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	float opacity = effective_default_opacity(p, 0.10f);
	float jitter_strength = effective_default_jitter(p, WB_DEFAULT_OBJECT_JITTER_STRENGTH);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "blob3d %63s at (%f,%f,%f) radius %f", name, &x, &y, &z, &radius);
	if (matched < 5)
		matched = sscanf(line, "blob3d %63s at (%f, %f, %f) radius %f", name, &x, &y, &z, &radius);
	if (matched < 4)
		matched = sscanf(line, "blob3d %63s at (%f,%f,%f)", name, &x, &y, &z);
	if (matched < 4)
		matched = sscanf(line, "blob3d %63s at (%f, %f, %f)", name, &x, &y, &z);
	if (matched < 4 && sscanf(line, "blob3d %63s", name) == 1)
	{
		char *at = strstr(line, " at ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 ||
			 sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4 && (strcmp(line, "blob3d") == 0 || starts_with(line, "blob3d at ")))
	{
		char *at = strstr(line, " at ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 ||
			 sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected blob3d [name] at (x,y,z) [radius R]");
	if (strstr(line, " radius "))
		sscanf(strstr(line, " radius "), " radius %f", &radius);
	else if (strstr(line, " r "))
		sscanf(strstr(line, " r "), " r %f", &radius);
	if (radius > 0.0f)
		rx = ry = rz = radius;
	if (strstr(line, " radii "))
	{
		if (sscanf(strstr(line, " radii "), " radii (%f,%f,%f)", &rx, &ry, &rz) != 3)
			sscanf(strstr(line, " radii "), " radii (%f, %f, %f)", &rx, &ry, &rz);
	}
	if (strstr(line, " rx "))
		sscanf(strstr(line, " rx "), " rx %f", &rx);
	if (strstr(line, " ry "))
		sscanf(strstr(line, " ry "), " ry %f", &ry);
	if (strstr(line, " rz "))
		sscanf(strstr(line, " rz "), " rz %f", &rz);
	if (strstr(line, " u_steps "))
		sscanf(strstr(line, " u_steps "), " u_steps %d", &u_steps);
	else if (strstr(line, " u "))
		sscanf(strstr(line, " u "), " u %d", &u_steps);
	if (strstr(line, " v_steps "))
		sscanf(strstr(line, " v_steps "), " v_steps %d", &v_steps);
	else if (strstr(line, " v "))
		sscanf(strstr(line, " v "), " v %d", &v_steps);
	if (strstr(line, " wobble "))
		sscanf(strstr(line, " wobble "), " wobble %f", &wobble);
	else if (strstr(line, " w "))
		sscanf(strstr(line, " w "), " w %f", &wobble);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	if (rx <= 0.0f) rx = 1.0f;
	if (ry <= 0.0f) ry = rx;
	if (rz <= 0.0f) rz = rx;
	ensure_object_name(p, name, sizeof(name), "blob3d");
	if (!add_blob3d_objects(p, name, vec3(x, y, z), vec3(rx, ry, rz), u_steps, v_steps, wobble,
		thickness, parse_colour(colour_name), opacity, jitter_strength))
		return set_error(p, line_no, "failed to create blob3d object");
	return 1;
}

static int parse_surface3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float ax = 0.0f, ay = 0.0f, az = 0.0f;
	float bx = 0.0f, by = 0.0f, bz = 0.0f;
	float cx = 0.0f, cy = 0.0f, cz = 0.0f;
	float dx = 0.0f, dy = 0.0f, dz = 0.0f;
	int u_steps = 4;
	int v_steps = 4;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	float opacity = effective_default_opacity(p, 0.10f);
	float jitter_strength = effective_default_jitter(p, WB_DEFAULT_OBJECT_JITTER_STRENGTH);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "surface3d %63s points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)",
		name, &ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz);
	if (matched < 12)
		matched = sscanf(line, "surface3d %63s points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)",
			name, &ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz);
	if (matched < 12 && sscanf(line, "surface3d %63s", name) == 1)
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)",
				&ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz) == 12 ||
			 sscanf(points, " points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)",
				&ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz) == 12))
			matched = 12;
	}
	if (matched < 12 && (strcmp(line, "surface3d") == 0 || starts_with(line, "surface3d points ")))
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)",
				&ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz) == 12 ||
			 sscanf(points, " points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)",
				&ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz) == 12))
			matched = 12;
	}
	if (matched < 12)
		return set_error(p, line_no, "expected surface3d [name] points (x,y,z) (x,y,z) (x,y,z) (x,y,z)");
	if (strstr(line, " u_steps "))
		sscanf(strstr(line, " u_steps "), " u_steps %d", &u_steps);
	else if (strstr(line, " u "))
		sscanf(strstr(line, " u "), " u %d", &u_steps);
	if (strstr(line, " v_steps "))
		sscanf(strstr(line, " v_steps "), " v_steps %d", &v_steps);
	else if (strstr(line, " v "))
		sscanf(strstr(line, " v "), " v %d", &v_steps);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	ensure_object_name(p, name, sizeof(name), "surface3d");
	if (!add_surface3d_objects(p, name, vec3(ax, ay, az), vec3(bx, by, bz), vec3(cx, cy, cz), vec3(dx, dy, dz),
		u_steps, v_steps, thickness, parse_colour(colour_name), opacity, jitter_strength))
		return set_error(p, line_no, "failed to create surface3d object");
	return 1;
}

static int parse_point3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, z = 0.0f, radius = WB_DEFAULT_POINT_RADIUS;
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "point3d %63s at (%f,%f,%f) radius %f colour %63s", name, &x, &y, &z, &radius, colour_name);
	if (matched < 4)
		matched = sscanf(line, "point3d %63s at (%f, %f, %f) radius %f colour %63s", name, &x, &y, &z, &radius, colour_name);
	if (matched < 4 && sscanf(line, "point3d %63s", name) == 1)
	{
		char *at = strstr(line, " at ");
		if (!at) at = strstr(line, " @ ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3 ||
			 sscanf(at, " @ (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " @ (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4 && (strcmp(line, "point3d") == 0 ||
		starts_with(line, "point3d at ") || starts_with(line, "point3d @ ")))
	{
		char *at = strstr(line, " at ");
		if (!at) at = strstr(line, " @ ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3 ||
			 sscanf(at, " @ (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " @ (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected point3d [name] at (x,y,z) radius N colour name");
	if (strstr(line, " radius "))
		sscanf(strstr(line, " radius "), " radius %f", &radius);
	else if (strstr(line, " r "))
		sscanf(strstr(line, " r "), " r %f", &radius);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_point3d(p->scene, x, y, z, matched >= 5 ? radius : WB_DEFAULT_POINT_RADIUS, parse_colour(matched >= 6 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create point3d object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_open_point3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, z = 0.0f, radius = WB_DEFAULT_OPEN_POINT_RADIUS, thickness = effective_default_thickness(p, WB_DEFAULT_OPEN_POINT_THICKNESS);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "open_point3d %63s at (%f,%f,%f) radius %f thickness %f colour %63s", name, &x, &y, &z, &radius, &thickness, colour_name);
	if (matched < 4)
		matched = sscanf(line, "open_point3d %63s at (%f, %f, %f) radius %f thickness %f colour %63s", name, &x, &y, &z, &radius, &thickness, colour_name);
	if (matched < 4 && sscanf(line, "open_point3d %63s", name) == 1)
	{
		char *at = strstr(line, " at ");
		if (!at) at = strstr(line, " @ ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3 ||
			 sscanf(at, " @ (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " @ (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4 && (strcmp(line, "open_point3d") == 0 ||
		starts_with(line, "open_point3d at ") || starts_with(line, "open_point3d @ ")))
	{
		char *at = strstr(line, " at ");
		if (!at) at = strstr(line, " @ ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3 ||
			 sscanf(at, " @ (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " @ (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected open_point3d [name] at (x,y,z) radius N thickness N colour name");
	if (strstr(line, " radius "))
		sscanf(strstr(line, " radius "), " radius %f", &radius);
	else if (strstr(line, " r "))
		sscanf(strstr(line, " r "), " r %f", &radius);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_open_point3d(p->scene, x, y, z, matched >= 5 ? radius : WB_DEFAULT_OPEN_POINT_RADIUS, matched >= 6 ? thickness : effective_default_thickness(p, WB_DEFAULT_OPEN_POINT_THICKNESS), parse_colour(matched >= 7 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create open_point3d object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_triangle3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, x2 = 0.0f, y2 = 0.0f, z2 = 0.0f, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "triangle3d %63s points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10)
		matched = sscanf(line, "triangle3d %63s points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) thickness %f colour %63s", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, &thickness, colour_name);
	if (matched < 10 && sscanf(line, "triangle3d %63s", name) == 1)
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9 ||
			 sscanf(points, " points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9))
			matched = 10;
	}
	if (matched < 10 && (strcmp(line, "triangle3d") == 0 || starts_with(line, "triangle3d points ")))
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9 ||
			 sscanf(points, " points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9))
			matched = 10;
	}
	if (matched < 10)
		return set_error(p, line_no, "expected triangle3d [name] points (x,y,z) (x,y,z) (x,y,z) thickness N colour name");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_triangle3d(p->scene, x0, y0, z0, x1, y1, z1, x2, y2, z2, matched >= 11 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), parse_colour(matched >= 12 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create triangle3d object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_triangle3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f, x2 = 0.0f, y2 = 0.0f, z2 = 0.0f, opacity = effective_default_opacity(p, WB_DEFAULT_SHADE_OPACITY);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "shade_triangle3d %63s points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) colour %63s opacity %f", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, colour_name, &opacity);
	if (matched < 10)
		matched = sscanf(line, "shade_triangle3d %63s points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) colour %63s opacity %f", name, &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2, colour_name, &opacity);
	if (matched < 10 && sscanf(line, "shade_triangle3d %63s", name) == 1)
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9 ||
			 sscanf(points, " points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9))
			matched = 10;
	}
	if (matched < 10 && (strcmp(line, "shade_triangle3d") == 0 || starts_with(line, "shade_triangle3d points ")))
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9 ||
			 sscanf(points, " points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1, &x2, &y2, &z2) == 9))
			matched = 10;
	}
	if (matched < 10)
		return set_error(p, line_no, "expected shade_triangle3d [name] points (x,y,z) (x,y,z) (x,y,z) colour name opacity A");
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	if (opacity < 0.0f)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	int id = wb_scene_add_shade_triangle3d(p->scene, x0, y0, z0, x1, y1, z1, x2, y2, z2, parse_colour(matched >= 11 ? colour_name : effective_default_colour_name(p, "blue")), matched >= 12 ? opacity : effective_default_opacity(p, WB_DEFAULT_SHADE_OPACITY));
	if (!id)
		return set_error(p, line_no, "failed to create shade_triangle3d object");
	apply_default_object_jitter(p, line, id);
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
		id = wb_scene_add_shade_triangle3d(p->scene, ax, ay, az, bx, by, bz, cx, cy, cz, colour, opacity);
		if (!id)
			return 0;
		remember_group_member(p, name, id);
		id = wb_scene_add_shade_triangle3d(p->scene, ax, ay, az, bx, by, bz, dx, dy, dz, colour, opacity * 0.92f);
		if (!id)
			return 0;
		remember_group_member(p, name, id);
		id = wb_scene_add_shade_triangle3d(p->scene, ax, ay, az, cx, cy, cz, dx, dy, dz, colour, opacity * 0.84f);
		if (!id)
			return 0;
		remember_group_member(p, name, id);
		id = wb_scene_add_shade_triangle3d(p->scene, bx, by, bz, cx, cy, cz, dx, dy, dz, colour, opacity * 0.76f);
		if (!id)
			return 0;
		remember_group_member(p, name, id);
	}
	
	id = wb_scene_add_line3d(p->scene, ax, ay, az, bx, by, bz, thickness, colour);
	if (!id)
		return 0;
	remember_group_member(p, name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	id = wb_scene_add_line3d(p->scene, ax, ay, az, cx, cy, cz, thickness, colour);
	if (!id)
		return 0;
	remember_group_member(p, name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	id = wb_scene_add_line3d(p->scene, ax, ay, az, dx, dy, dz, thickness, colour);
	if (!id)
		return 0;
	remember_group_member(p, name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	id = wb_scene_add_line3d(p->scene, bx, by, bz, cx, cy, cz, thickness, colour);
	if (!id)
		return 0;
	remember_group_member(p, name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	id = wb_scene_add_line3d(p->scene, bx, by, bz, dx, dy, dz, thickness, colour);
	if (!id)
		return 0;
	remember_group_member(p, name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	id = wb_scene_add_line3d(p->scene, cx, cy, cz, dx, dy, dz, thickness, colour);
	if (!id)
		return 0;
	remember_group_member(p, name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	
	id = wb_scene_add_point3d(p->scene, ax, ay, az, WB_DEFAULT_POINT_RADIUS, colour);
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_a", name);
	remember_name(p, part_name, id);
	remember_group_member(p, name, id);
	id = wb_scene_add_point3d(p->scene, bx, by, bz, WB_DEFAULT_POINT_RADIUS, colour);
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_b", name);
	remember_name(p, part_name, id);
	remember_group_member(p, name, id);
	id = wb_scene_add_point3d(p->scene, cx, cy, cz, WB_DEFAULT_POINT_RADIUS, colour);
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_c", name);
	remember_name(p, part_name, id);
	remember_group_member(p, name, id);
	id = wb_scene_add_point3d(p->scene, dx, dy, dz, WB_DEFAULT_POINT_RADIUS, colour);
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_d", name);
	remember_name(p, part_name, id);
	remember_group_member(p, name, id);
	
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static int parse_tetrahedron3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float ax = 0.0f, ay = 0.0f, az = 0.0f;
	float bx = 0.0f, by = 0.0f, bz = 0.0f;
	float cx = 0.0f, cy = 0.0f, cz = 0.0f;
	float dx = 0.0f, dy = 0.0f, dz = 0.0f;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	float opacity = effective_default_opacity(p, 0.10f);
	float jitter_strength = effective_default_jitter(p, WB_DEFAULT_OBJECT_JITTER_STRENGTH);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
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
	if (matched < 13 && (sscanf(line, "tetrahedron3d %63s", name) == 1 || sscanf(line, "tetra3d %63s", name) == 1))
	{
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)",
				&ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz) == 12 ||
			 sscanf(points, " points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)",
				&ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz) == 12))
			matched = 13;
	}
	if (matched < 13)
	{
		if (strcmp(line, "tetrahedron3d") != 0 && strcmp(line, "tetra3d") != 0 &&
			!starts_with(line, "tetrahedron3d points ") && !starts_with(line, "tetra3d points "))
			return set_error(p, line_no, "expected tetrahedron3d/tetra3d [name] points (x,y,z) (x,y,z) (x,y,z) (x,y,z) thickness N colour name opacity A");
		char *points = strstr(line, " points ");
		if (points &&
			(sscanf(points, " points (%f,%f,%f) (%f,%f,%f) (%f,%f,%f) (%f,%f,%f)",
				&ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz) == 12 ||
			 sscanf(points, " points (%f, %f, %f) (%f, %f, %f) (%f, %f, %f) (%f, %f, %f)",
				&ax, &ay, &az, &bx, &by, &bz, &cx, &cy, &cz, &dx, &dy, &dz) == 12))
			matched = 13;
	}
	if (matched < 13)
		return set_error(p, line_no, "expected tetrahedron3d/tetra3d [name] points (x,y,z) (x,y,z) (x,y,z) (x,y,z) thickness N colour name opacity A");
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	else if (strstr(line, " a "))
		sscanf(strstr(line, " a "), " a %f", &opacity);
	
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	ensure_object_name(p, name, sizeof(name), "tetra3d");
	if (!add_tetrahedron3d_objects(p, name, ax, ay, az, bx, by, bz, cx, cy, cz, dx, dy, dz, thickness, parse_colour(matched >= 15 ? colour_name : effective_default_colour_name(p, "blue")), matched >= 16 ? opacity : effective_default_opacity(p, 0.10f), jitter_strength))
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
	remember_group_member(p, name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	
	id = wb_scene_add_line3d(p->scene, x, y, z, x, y + length, z, thickness, parse_colour("green"));
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_y", name);
	remember_name(p, part_name, id);
	remember_group_member(p, name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	
	id = wb_scene_add_line3d(p->scene, x, y, z, x, y, z + length, thickness, parse_colour("blue"));
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_z", name);
	remember_name(p, part_name, id);
	remember_group_member(p, name, id);
	wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	
	id = wb_scene_add_point3d(p->scene, x, y, z, WB_DEFAULT_POINT_RADIUS, parse_colour("black"));
	if (!id)
		return 0;
	snprintf(part_name, sizeof(part_name), "%s_o", name);
	remember_name(p, part_name, id);
	remember_group_member(p, name, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_axes3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	float x = 0.0f, y = 0.0f, z = 0.0f;
	float length = 1.0f;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	float jitter_strength = effective_default_jitter(p, WB_DEFAULT_OBJECT_JITTER_STRENGTH);
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
	if (matched < 4 && (sscanf(line, "axes3d %63s", name) == 1 || sscanf(line, "axes %63s", name) == 1))
	{
		char *at = strstr(line, " at ");
		if (!at) at = strstr(line, " origin ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3 ||
			 sscanf(at, " origin (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " origin (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4)
	{
		if (strcmp(line, "axes3d") != 0 && strcmp(line, "axes") != 0 &&
			!starts_with(line, "axes3d at ") && !starts_with(line, "axes at ") &&
			!starts_with(line, "axes3d origin ") && !starts_with(line, "axes origin "))
			return set_error(p, line_no, "expected axes3d/axes [name] at (x,y,z) length N thickness N");
		char *at = strstr(line, " at ");
		if (!at) at = strstr(line, " origin ");
		if (at &&
			(sscanf(at, " at (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " at (%f, %f, %f)", &x, &y, &z) == 3 ||
			 sscanf(at, " origin (%f,%f,%f)", &x, &y, &z) == 3 || sscanf(at, " origin (%f, %f, %f)", &x, &y, &z) == 3))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected axes3d/axes [name] at (x,y,z) length N thickness N");
	if (strstr(line, " length "))
		sscanf(strstr(line, " length "), " length %f", &length);
	else if (strstr(line, " len "))
		sscanf(strstr(line, " len "), " len %f", &length);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	
	if (parse_jitter_token(line, &jitter_strength) == 0 && effective_default_jitter(p, BIG_NEGATIVE_FLOAT) == BIG_NEGATIVE_FLOAT)
		jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	ensure_object_name(p, name, sizeof(name), "axes3d");
	if (!add_axes3d_objects(p, name, x, y, z, matched >= 5 ? length : 1.0f, matched >= 6 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), jitter_strength))
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
			remember_group_member(p, name, p->scene->objects[p->scene->n_objects - 1].id);
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
		remember_group_member(p, name, id);
		wb_scene_set_object_jitter(p->scene, id, jitter_strength);
	}
	
	for (int i = 0; i < 8; i++)
	{
		id = wb_scene_add_point3d(p->scene, v[i].x, v[i].y, v[i].z, WB_DEFAULT_POINT_RADIUS, colour);
		if (!id)
			return 0;
		snprintf(part_name, sizeof(part_name), "%s_v%d", name, i);
		remember_name(p, part_name, id);
		remember_group_member(p, name, id);
	}
	
	remember_name(p, name, p->scene->objects[p->scene->n_objects - 1].id);
	return 1;
}

static int parse_cube3d_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float cx = 0.0f, cy = 0.0f, cz = 0.0f;
	float size = 1.0f;
	float thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	float opacity = effective_default_opacity(p, 0.08f);
	float jitter_strength = effective_default_jitter(p, WB_DEFAULT_OBJECT_JITTER_STRENGTH);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "cube3d %63s center (%f,%f,%f) size %f thickness %f colour %63s opacity %f", name, &cx, &cy, &cz, &size, &thickness, colour_name, &opacity);
	if (matched < 4)
		matched = sscanf(line, "cube3d %63s center (%f, %f, %f) size %f thickness %f colour %63s opacity %f", name, &cx, &cy, &cz, &size, &thickness, colour_name, &opacity);
	if (matched < 4)
		matched = sscanf(line, "cube %63s (%f,%f,%f) s %f t %f c %63s a %f", name, &cx, &cy, &cz, &size, &thickness, colour_name, &opacity);
	if (matched < 4)
		matched = sscanf(line, "cube %63s (%f, %f, %f) s %f t %f c %63s a %f", name, &cx, &cy, &cz, &size, &thickness, colour_name, &opacity);
	if (matched < 4 && (sscanf(line, "cube3d %63s", name) == 1 || sscanf(line, "cube %63s", name) == 1))
	{
		char *center = strstr(line, " center ");
		if (!center) center = strstr(line, " at ");
		if (center &&
			(sscanf(center, " center (%f,%f,%f)", &cx, &cy, &cz) == 3 || sscanf(center, " center (%f, %f, %f)", &cx, &cy, &cz) == 3 ||
			 sscanf(center, " at (%f,%f,%f)", &cx, &cy, &cz) == 3 || sscanf(center, " at (%f, %f, %f)", &cx, &cy, &cz) == 3))
			matched = 4;
	}
	if (matched < 4)
	{
		if (strcmp(line, "cube3d") != 0 && strcmp(line, "cube") != 0 &&
			!starts_with(line, "cube3d center ") && !starts_with(line, "cube center ") &&
			!starts_with(line, "cube3d at ") && !starts_with(line, "cube at "))
			return set_error(p, line_no, "expected cube3d/cube [name] center (x,y,z) size N thickness N colour name opacity A");
		char *center = strstr(line, " center ");
		if (!center) center = strstr(line, " at ");
		if (center &&
			(sscanf(center, " center (%f,%f,%f)", &cx, &cy, &cz) == 3 || sscanf(center, " center (%f, %f, %f)", &cx, &cy, &cz) == 3 ||
			 sscanf(center, " at (%f,%f,%f)", &cx, &cy, &cz) == 3 || sscanf(center, " at (%f, %f, %f)", &cx, &cy, &cz) == 3))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected cube3d/cube [name] center (x,y,z) size N thickness N colour name opacity A");
	if (strstr(line, " size "))
		sscanf(strstr(line, " size "), " size %f", &size);
	else if (strstr(line, " s "))
		sscanf(strstr(line, " s "), " s %f", &size);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	else if (strstr(line, " a "))
		sscanf(strstr(line, " a "), " a %f", &opacity);
	
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	if (parse_jitter_token(line, &jitter_strength) == 0 && effective_default_jitter(p, BIG_NEGATIVE_FLOAT) == BIG_NEGATIVE_FLOAT)
		jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	ensure_object_name(p, name, sizeof(name), "cube3d");
	if (!add_cube3d_objects(p, name, cx, cy, cz, matched >= 5 ? size : 1.0f, matched >= 6 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), parse_colour(matched >= 7 ? colour_name : effective_default_colour_name(p, "blue")), matched >= 8 ? opacity : effective_default_opacity(p, 0.08f), jitter_strength))
		return set_error(p, line_no, "failed to create cube3d object");
	return 1;
}

static int parse_point_object(wb_spec_parser *p, char *line, int line_no, int open)
{
	char name[64] = "";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, radius = WB_DEFAULT_POINT_RADIUS, thickness = effective_default_thickness(p, WB_DEFAULT_OPEN_POINT_THICKNESS);
	int matched = 0;
	int id = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	if (open)
	{
		matched = sscanf(line, "open_point %63s at (%f,%f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3)
		matched = sscanf(line, "open_point %63s at (%f, %f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3)
			matched = sscanf(line, "opt %63s (%f,%f) r %f t %f c %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3)
			matched = sscanf(line, "opt %63s (%f, %f) r %f t %f c %63s", name, &x, &y, &radius, &thickness, colour_name);
		if (matched < 3 && (sscanf(line, "open_point %63s", name) == 1 || sscanf(line, "opt %63s", name) == 1))
		{
			char *at = strstr(line, " at ");
			if (!at) at = strstr(line, " @ ");
			if (at &&
				(sscanf(at, " at (%f,%f)", &x, &y) == 2 || sscanf(at, " at (%f, %f)", &x, &y) == 2 ||
				 sscanf(at, " @ (%f,%f)", &x, &y) == 2 || sscanf(at, " @ (%f, %f)", &x, &y) == 2))
				matched = 3;
		}
		if (matched < 3 && (strcmp(line, "open_point") == 0 || strcmp(line, "opt") == 0 ||
			starts_with(line, "open_point at ") || starts_with(line, "opt at ") ||
			starts_with(line, "open_point @ ") || starts_with(line, "opt @ ")))
		{
			char *at = strstr(line, " at ");
			if (!at) at = strstr(line, " @ ");
			if (at &&
				(sscanf(at, " at (%f,%f)", &x, &y) == 2 || sscanf(at, " at (%f, %f)", &x, &y) == 2 ||
				 sscanf(at, " @ (%f,%f)", &x, &y) == 2 || sscanf(at, " @ (%f, %f)", &x, &y) == 2))
				matched = 3;
		}
		if (matched < 3)
			return set_error(p, line_no, "expected open_point/opt [name] at (x,y) radius N thickness N colour name");
		if (strstr(line, " radius "))
			sscanf(strstr(line, " radius "), " radius %f", &radius);
		else if (strstr(line, " r "))
			sscanf(strstr(line, " r "), " r %f", &radius);
		if (strstr(line, " thickness "))
			sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
		else if (strstr(line, " t "))
			sscanf(strstr(line, " t "), " t %f", &thickness);
		if (strstr(line, " colour "))
			sscanf(strstr(line, " colour "), " colour %63s", colour_name);
		else if (strstr(line, " color "))
			sscanf(strstr(line, " color "), " color %63s", colour_name);
		else if (strstr(line, " c "))
			sscanf(strstr(line, " c "), " c %63s", colour_name);
		id = wb_scene_add_open_point(p->scene, x, y, radius, matched >= 5 ? thickness : effective_default_thickness(p, WB_DEFAULT_OPEN_POINT_THICKNESS), parse_colour(matched >= 6 ? colour_name : effective_default_colour_name(p, "blue")));
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
		if (matched < 3 && (sscanf(line, "point %63s", name) == 1 || sscanf(line, "pt %63s", name) == 1))
		{
			char *at = strstr(line, " at ");
			if (!at) at = strstr(line, " @ ");
			if (at &&
				(sscanf(at, " at (%f,%f)", &x, &y) == 2 || sscanf(at, " at (%f, %f)", &x, &y) == 2 ||
				 sscanf(at, " @ (%f,%f)", &x, &y) == 2 || sscanf(at, " @ (%f, %f)", &x, &y) == 2))
				matched = 3;
		}
		if (matched < 3 && (strcmp(line, "point") == 0 || strcmp(line, "pt") == 0 ||
			starts_with(line, "point at ") || starts_with(line, "pt at ") ||
			starts_with(line, "point @ ") || starts_with(line, "pt @ ")))
		{
			char *at = strstr(line, " at ");
			if (!at) at = strstr(line, " @ ");
			if (at &&
				(sscanf(at, " at (%f,%f)", &x, &y) == 2 || sscanf(at, " at (%f, %f)", &x, &y) == 2 ||
				 sscanf(at, " @ (%f,%f)", &x, &y) == 2 || sscanf(at, " @ (%f, %f)", &x, &y) == 2))
				matched = 3;
		}
		if (matched < 3)
			return set_error(p, line_no, "expected point/pt [name] at (x,y) radius N colour name");
		if (strstr(line, " radius "))
			sscanf(strstr(line, " radius "), " radius %f", &radius);
		else if (strstr(line, " r "))
			sscanf(strstr(line, " r "), " r %f", &radius);
		if (strstr(line, " colour "))
			sscanf(strstr(line, " colour "), " colour %63s", colour_name);
		else if (strstr(line, " color "))
			sscanf(strstr(line, " color "), " color %63s", colour_name);
		else if (strstr(line, " c "))
			sscanf(strstr(line, " c "), " c %63s", colour_name);
		id = wb_scene_add_point(p->scene, x, y, matched >= 4 ? radius : WB_DEFAULT_POINT_RADIUS, parse_colour(matched >= 5 ? colour_name : effective_default_colour_name(p, "blue")));
	}
	
	if (!id)
		return set_error(p, line_no, open ? "failed to create open_point object" : "failed to create point object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_circle_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, radius = WB_DEFAULT_CIRCLE_RADIUS, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "circle %63s center (%f,%f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4)
		matched = sscanf(line, "circle %63s center (%f, %f) radius %f thickness %f colour %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4)
		matched = sscanf(line, "circ %63s (%f,%f) r %f t %f c %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4)
		matched = sscanf(line, "circ %63s (%f, %f) r %f t %f c %63s", name, &x, &y, &radius, &thickness, colour_name);
	if (matched < 4 && (sscanf(line, "circle %63s", name) == 1 || sscanf(line, "circ %63s", name) == 1))
	{
		char *center = strstr(line, " center ");
		if (!center) center = strstr(line, " at ");
		if (!center) center = strstr(line, " @ ");
		if (center &&
			(sscanf(center, " center (%f,%f)", &x, &y) == 2 || sscanf(center, " center (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " at (%f,%f)", &x, &y) == 2 || sscanf(center, " at (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " @ (%f,%f)", &x, &y) == 2 || sscanf(center, " @ (%f, %f)", &x, &y) == 2))
			matched = 4;
	}
	if (matched < 4 && (strcmp(line, "circle") == 0 || strcmp(line, "circ") == 0 ||
		starts_with(line, "circle center ") || starts_with(line, "circ center ") ||
		starts_with(line, "circle at ") || starts_with(line, "circ at ") ||
		starts_with(line, "circle @ ") || starts_with(line, "circ @ ")))
	{
		char *center = strstr(line, " center ");
		if (!center) center = strstr(line, " at ");
		if (!center) center = strstr(line, " @ ");
		if (center &&
			(sscanf(center, " center (%f,%f)", &x, &y) == 2 || sscanf(center, " center (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " at (%f,%f)", &x, &y) == 2 || sscanf(center, " at (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " @ (%f,%f)", &x, &y) == 2 || sscanf(center, " @ (%f, %f)", &x, &y) == 2))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected circle/circ [name] center (x,y) radius N thickness N colour name");
	if (strstr(line, " radius "))
		sscanf(strstr(line, " radius "), " radius %f", &radius);
	else if (strstr(line, " r "))
		sscanf(strstr(line, " r "), " r %f", &radius);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_circle(p->scene, x, y, radius, matched >= 5 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), parse_colour(matched >= 6 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create circle object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_ellipse_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, radius_x = WB_DEFAULT_ELLIPSE_RADIUS_X, radius_y = WB_DEFAULT_ELLIPSE_RADIUS_Y, thickness = effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "ellipse %63s center (%f,%f) radii (%f,%f) thickness %f colour %63s", name, &x, &y, &radius_x, &radius_y, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ellipse %63s center (%f, %f) radii (%f, %f) thickness %f colour %63s", name, &x, &y, &radius_x, &radius_y, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ell %63s (%f,%f) rx %f ry %f t %f c %63s", name, &x, &y, &radius_x, &radius_y, &thickness, colour_name);
	if (matched < 5)
		matched = sscanf(line, "ell %63s (%f, %f) rx %f ry %f t %f c %63s", name, &x, &y, &radius_x, &radius_y, &thickness, colour_name);
	if (matched < 5 && (sscanf(line, "ellipse %63s", name) == 1 || sscanf(line, "ell %63s", name) == 1))
	{
		char *center = strstr(line, " center ");
		if (!center) center = strstr(line, " at ");
		if (!center) center = strstr(line, " @ ");
		if (center &&
			(sscanf(center, " center (%f,%f)", &x, &y) == 2 || sscanf(center, " center (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " at (%f,%f)", &x, &y) == 2 || sscanf(center, " at (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " @ (%f,%f)", &x, &y) == 2 || sscanf(center, " @ (%f, %f)", &x, &y) == 2))
			matched = 5;
	}
	if (matched < 5 && (strcmp(line, "ellipse") == 0 || strcmp(line, "ell") == 0 ||
		starts_with(line, "ellipse center ") || starts_with(line, "ell center ") ||
		starts_with(line, "ellipse at ") || starts_with(line, "ell at ") ||
		starts_with(line, "ellipse @ ") || starts_with(line, "ell @ ")))
	{
		char *center = strstr(line, " center ");
		if (!center) center = strstr(line, " at ");
		if (!center) center = strstr(line, " @ ");
		if (center &&
			(sscanf(center, " center (%f,%f)", &x, &y) == 2 || sscanf(center, " center (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " at (%f,%f)", &x, &y) == 2 || sscanf(center, " at (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " @ (%f,%f)", &x, &y) == 2 || sscanf(center, " @ (%f, %f)", &x, &y) == 2))
			matched = 5;
	}
	if (matched < 5)
		return set_error(p, line_no, "expected ellipse/ell [name] center (x,y) radii (rx,ry) thickness N colour name");
	if (strstr(line, " radii "))
		sscanf(strstr(line, " radii "), " radii (%f,%f)", &radius_x, &radius_y) == 2 ||
		sscanf(strstr(line, " radii "), " radii (%f, %f)", &radius_x, &radius_y) == 2;
	if (strstr(line, " rx "))
		sscanf(strstr(line, " rx "), " rx %f", &radius_x);
	if (strstr(line, " ry "))
		sscanf(strstr(line, " ry "), " ry %f", &radius_y);
	if (strstr(line, " thickness "))
		sscanf(strstr(line, " thickness "), " thickness %f", &thickness);
	else if (strstr(line, " t "))
		sscanf(strstr(line, " t "), " t %f", &thickness);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	
	int id = wb_scene_add_ellipse(p->scene, x, y, radius_x, radius_y, matched >= 6 ? thickness : effective_default_thickness(p, WB_DEFAULT_LINE_THICKNESS), parse_colour(matched >= 7 ? colour_name : effective_default_colour_name(p, "blue")));
	if (!id)
		return set_error(p, line_no, "failed to create ellipse object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_shade_disc_object(wb_spec_parser *p, char *line, int line_no)
{
	char name[64] = "";
	char colour_name[64];
	float x = 0.0f, y = 0.0f, radius = WB_DEFAULT_CIRCLE_RADIUS, opacity = effective_default_opacity(p, WB_DEFAULT_SHADE_OPACITY);
	int matched = 0;
	snprintf(colour_name, sizeof(colour_name), "%s", effective_default_colour_name(p, "blue"));
	
	matched = sscanf(line, "shade_disc %63s center (%f,%f) radius %f colour %63s opacity %f", name, &x, &y, &radius, colour_name, &opacity);
	if (matched < 4)
		matched = sscanf(line, "shade_disc %63s center (%f, %f) radius %f colour %63s opacity %f", name, &x, &y, &radius, colour_name, &opacity);
	if (matched < 4 && sscanf(line, "shade_disc %63s", name) == 1)
	{
		if (strcmp(name, "center") == 0 || strcmp(name, "at") == 0 || strcmp(name, "@") == 0)
			name[0] = 0;
		else
		{
			char *center = strstr(line, " center ");
			if (!center) center = strstr(line, " at ");
			if (!center) center = strstr(line, " @ ");
			if (center &&
				(sscanf(center, " center (%f,%f)", &x, &y) == 2 || sscanf(center, " center (%f, %f)", &x, &y) == 2 ||
				 sscanf(center, " at (%f,%f)", &x, &y) == 2 || sscanf(center, " at (%f, %f)", &x, &y) == 2 ||
				 sscanf(center, " @ (%f,%f)", &x, &y) == 2 || sscanf(center, " @ (%f, %f)", &x, &y) == 2))
				matched = 4;
		}
	}
	if (matched < 4 && sscanf(line, "shade_disc %63s", name) == 1)
	{
		char *center = strstr(line, " center ");
		if (!center) center = strstr(line, " at ");
		if (!center) center = strstr(line, " @ ");
		if (center &&
			(sscanf(center, " center (%f,%f)", &x, &y) == 2 || sscanf(center, " center (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " at (%f,%f)", &x, &y) == 2 || sscanf(center, " at (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " @ (%f,%f)", &x, &y) == 2 || sscanf(center, " @ (%f, %f)", &x, &y) == 2))
			matched = 4;
	}
	if (matched < 4 && (strcmp(line, "shade_disc") == 0 ||
		starts_with(line, "shade_disc center ") ||
		starts_with(line, "shade_disc at ") ||
		starts_with(line, "shade_disc @ ")))
	{
		char *center = strstr(line, " center ");
		if (!center) center = strstr(line, " at ");
		if (!center) center = strstr(line, " @ ");
		if (center &&
			(sscanf(center, " center (%f,%f)", &x, &y) == 2 || sscanf(center, " center (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " at (%f,%f)", &x, &y) == 2 || sscanf(center, " at (%f, %f)", &x, &y) == 2 ||
			 sscanf(center, " @ (%f,%f)", &x, &y) == 2 || sscanf(center, " @ (%f, %f)", &x, &y) == 2))
			matched = 4;
	}
	if (matched < 4)
		return set_error(p, line_no, "expected shade_disc [name] center (x,y) radius N colour name opacity A");
	if (strstr(line, " radius "))
		sscanf(strstr(line, " radius "), " radius %f", &radius);
	else if (strstr(line, " r "))
		sscanf(strstr(line, " r "), " r %f", &radius);
	if (strstr(line, " colour "))
		sscanf(strstr(line, " colour "), " colour %63s", colour_name);
	else if (strstr(line, " color "))
		sscanf(strstr(line, " color "), " color %63s", colour_name);
	else if (strstr(line, " c "))
		sscanf(strstr(line, " c "), " c %63s", colour_name);
	if (strstr(line, " opacity "))
		sscanf(strstr(line, " opacity "), " opacity %f", &opacity);
	
	if (opacity < WB_MIN_OPACITY)
		opacity = WB_MIN_OPACITY;
	if (opacity > WB_MAX_OPACITY)
		opacity = WB_MAX_OPACITY;
	
	int id = wb_scene_add_shade_disc(p->scene, x, y, radius, parse_colour(matched >= 5 ? colour_name : effective_default_colour_name(p, "blue")), matched >= 6 ? opacity : effective_default_opacity(p, WB_DEFAULT_SHADE_OPACITY));
	if (!id)
		return set_error(p, line_no, "failed to create shade_disc object");
	apply_default_object_jitter(p, line, id);
	remember_name(p, name, id);
	return 1;
}

static int parse_spec_line(wb_spec_parser *p, char *line, int line_no)
{
	char raw_copy[2048];
	char *raw = line;
	char *s = trim_left(line);
	int raw_indent = 0;
	
	if (!p || !line)
		return 0;
	snprintf(raw_copy, sizeof(raw_copy), "%s", line);
	raw = raw_copy;
	raw_indent = leading_indent_width(raw);
	p->current_line_indent = raw_indent;
	if (p->n_group_scopes > 0 && *trim_left(raw) != 0 && *trim_left(raw) != '#')
		pop_finished_group_scopes(p, raw_indent);
	if (p->n_patch_scopes > 0 && *trim_left(raw) != 0 && *trim_left(raw) != '#')
		pop_finished_patch_scopes(p, raw_indent);
	if (p->n_default_scopes > 0 && *trim_left(raw) != 0 && *trim_left(raw) != '#')
	{
		while (p->n_default_scopes > 0 && raw_indent < p->default_scopes[p->n_default_scopes - 1].indent)
			p->n_default_scopes--;
	}
	
	if (p->pending_block_type != WB_PENDING_BLOCK_NONE)
	{
		char pending_line[2048];
		char pending_with_indent[2048];
		char raw_trimmed[2048];
		char *raw_s;
		int pending_indent = p->pending_block_indent;
		
		snprintf(raw_trimmed, sizeof(raw_trimmed), "%s", raw);
		raw_s = trim_left(raw_trimmed);
		trim_right(raw_s);
		if (pending_block_accepts_property(p->pending_block_type, raw_s))
		{
			if (p->pending_block_type == WB_PENDING_BLOCK_PATCH)
				return parse_patch_property(p, raw_s, line_no);
			size_t used = strlen(p->pending_line);
			char normalized_prop[256];
			const char *prop = raw_s;
			
			normalized_prop[0] = 0;
			if (p->pending_block_type == WB_PENDING_BLOCK_SCENE && looks_like_duration(raw_s))
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "duration %s", raw_s);
				prop = normalized_prop;
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_DRAW && looks_like_time_range(raw_s))
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "during %s", raw_s);
				prop = normalized_prop;
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_MOVE ||
				p->pending_block_type == WB_PENDING_BLOCK_MOVE_PATCH) && looks_like_time_range(raw_s))
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "during %s", raw_s);
				prop = normalized_prop;
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_TURN_PATCH ||
				p->pending_block_type == WB_PENDING_BLOCK_SCALE_PATCH) && looks_like_time_range(raw_s))
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "during %s", raw_s);
				prop = normalized_prop;
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_TURN ||
				p->pending_block_type == WB_PENDING_BLOCK_SCALE) && looks_like_time_range(raw_s))
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "during %s", raw_s);
				prop = normalized_prop;
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_FADE)
			{
				if (looks_like_time_range(raw_s))
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "during %s", raw_s);
					prop = normalized_prop;
				}
				else if (looks_like_scalar_arrow(raw_s))
				{
					float a0 = 0.0f;
					float a1 = 0.0f;
					if (sscanf(raw_s, "%f -> %f", &a0, &a1) == 2)
					{
						snprintf(normalized_prop, sizeof(normalized_prop), "from %g to %g", a0, a1);
						prop = normalized_prop;
					}
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_TURN_PATCH && looks_like_scalar_arrow(raw_s))
			{
				float a0 = 0.0f, a1 = 0.0f;
				if (sscanf(raw_s, "%f -> %f", &a0, &a1) == 2)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from %g to %g", a0, a1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_TURN && looks_like_scalar_arrow(raw_s))
			{
				float a0 = 0.0f, a1 = 0.0f;
				if (sscanf(raw_s, "%f -> %f", &a0, &a1) == 2)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from %g to %g", a0, a1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_TURN_PATCH && looks_like_vec3_arrow(raw_s))
			{
				float y0 = 0.0f, p0 = 0.0f, r0 = 0.0f, y1 = 0.0f, p1 = 0.0f, r1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f,%f) -> (%f,%f,%f)", &y0, &p0, &r0, &y1, &p1, &r1) == 6 ||
					sscanf(raw_s, "(%f, %f, %f) -> (%f, %f, %f)", &y0, &p0, &r0, &y1, &p1, &r1) == 6)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f,%.3f) to (%.3f,%.3f,%.3f)", y0, p0, r0, y1, p1, r1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_TURN && looks_like_vec3_arrow(raw_s))
			{
				float y0 = 0.0f, p0 = 0.0f, r0 = 0.0f, y1 = 0.0f, p1 = 0.0f, r1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f,%f) -> (%f,%f,%f)", &y0, &p0, &r0, &y1, &p1, &r1) == 6 ||
					sscanf(raw_s, "(%f, %f, %f) -> (%f, %f, %f)", &y0, &p0, &r0, &y1, &p1, &r1) == 6)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f,%.3f) to (%.3f,%.3f,%.3f)", y0, p0, r0, y1, p1, r1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_SCALE_PATCH && looks_like_scalar_arrow(raw_s))
			{
				float a0 = 0.0f, a1 = 0.0f;
				if (sscanf(raw_s, "%f -> %f", &a0, &a1) == 2)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from %g to %g", a0, a1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_SCALE && looks_like_scalar_arrow(raw_s))
			{
				float a0 = 0.0f, a1 = 0.0f;
				if (sscanf(raw_s, "%f -> %f", &a0, &a1) == 2)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from %g to %g", a0, a1);
					prop = normalized_prop;
				}
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_MATH ||
				p->pending_block_type == WB_PENDING_BLOCK_TEXT ||
				p->pending_block_type == WB_PENDING_BLOCK_POINT ||
				p->pending_block_type == WB_PENDING_BLOCK_OPEN_POINT ||
				p->pending_block_type == WB_PENDING_BLOCK_CIRCLE ||
				p->pending_block_type == WB_PENDING_BLOCK_ELLIPSE ||
				p->pending_block_type == WB_PENDING_BLOCK_SHADE_DISC) &&
				looks_like_vec2_literal(raw_s))
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "at %s", raw_s);
				prop = normalized_prop;
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_POINT3D ||
				p->pending_block_type == WB_PENDING_BLOCK_OPEN_POINT3D ||
				p->pending_block_type == WB_PENDING_BLOCK_AXES3D ||
				p->pending_block_type == WB_PENDING_BLOCK_CUBE3D ||
				p->pending_block_type == WB_PENDING_BLOCK_BLOB3D ||
				p->pending_block_type == WB_PENDING_BLOCK_PARAM3D ||
				p->pending_block_type == WB_PENDING_BLOCK_PARAM_SURFACE3D ||
				p->pending_block_type == WB_PENDING_BLOCK_VOLUME3D) &&
				looks_like_vec3_literal(raw_s))
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "at %s", raw_s);
				prop = normalized_prop;
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_LINE ||
				p->pending_block_type == WB_PENDING_BLOCK_ARROW ||
				p->pending_block_type == WB_PENDING_BLOCK_DOTTED ||
				p->pending_block_type == WB_PENDING_BLOCK_DASHED ||
				p->pending_block_type == WB_PENDING_BLOCK_RAY) &&
				looks_like_vec2_arrow(raw_s))
			{
				float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f) -> (%f,%f)", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "(%f, %f) -> (%f, %f)", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "[%f,%f] -> [%f,%f]", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "[%f, %f] -> [%f, %f]", &x0, &y0, &x1, &y1) == 4)
				{
					if (p->pending_block_type == WB_PENDING_BLOCK_RAY)
						snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f) through (%.3f,%.3f)", x0, y0, x1, y1);
					else
						snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f) to (%.3f,%.3f)", x0, y0, x1, y1);
					prop = normalized_prop;
				}
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_MOVE ||
				p->pending_block_type == WB_PENDING_BLOCK_MOVE_PATCH) && looks_like_vec2_arrow(raw_s))
			{
				float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f) -> (%f,%f)", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "(%f, %f) -> (%f, %f)", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "[%f,%f] -> [%f,%f]", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "[%f, %f] -> [%f, %f]", &x0, &y0, &x1, &y1) == 4)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f) to (%.3f,%.3f)", x0, y0, x1, y1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_MOVE && looks_like_vec3_arrow(raw_s))
			{
				float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f,%f) -> (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6 ||
					sscanf(raw_s, "(%f, %f, %f) -> (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f,%.3f) to (%.3f,%.3f,%.3f)", x0, y0, z0, x1, y1, z1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_MOVE_PATCH && looks_like_vec3_arrow(raw_s))
			{
				float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f,%f) -> (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6 ||
					sscanf(raw_s, "(%f, %f, %f) -> (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f,%.3f) to (%.3f,%.3f,%.3f)", x0, y0, z0, x1, y1, z1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_SCALE_PATCH && looks_like_vec2_arrow(raw_s))
			{
				float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f) -> (%f,%f)", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "(%f, %f) -> (%f, %f)", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "[%f,%f] -> [%f,%f]", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "[%f, %f] -> [%f, %f]", &x0, &y0, &x1, &y1) == 4)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f) to (%.3f,%.3f)", x0, y0, x1, y1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_SCALE && looks_like_vec2_arrow(raw_s))
			{
				float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f) -> (%f,%f)", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "(%f, %f) -> (%f, %f)", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "[%f,%f] -> [%f,%f]", &x0, &y0, &x1, &y1) == 4 ||
					sscanf(raw_s, "[%f, %f] -> [%f, %f]", &x0, &y0, &x1, &y1) == 4)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f) to (%.3f,%.3f)", x0, y0, x1, y1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_SCALE_PATCH && looks_like_vec3_arrow(raw_s))
			{
				float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f,%f) -> (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6 ||
					sscanf(raw_s, "(%f, %f, %f) -> (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f,%.3f) to (%.3f,%.3f,%.3f)", x0, y0, z0, x1, y1, z1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_SCALE && looks_like_vec3_arrow(raw_s))
			{
				float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f,%f) -> (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6 ||
					sscanf(raw_s, "(%f, %f, %f) -> (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f,%.3f) to (%.3f,%.3f,%.3f)", x0, y0, z0, x1, y1, z1);
					prop = normalized_prop;
				}
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_LINE3D && looks_like_vec3_arrow(raw_s))
			{
				float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f, x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
				if (sscanf(raw_s, "(%f,%f,%f) -> (%f,%f,%f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6 ||
					sscanf(raw_s, "(%f, %f, %f) -> (%f, %f, %f)", &x0, &y0, &z0, &x1, &y1, &z1) == 6)
				{
					snprintf(normalized_prop, sizeof(normalized_prop), "from (%.3f,%.3f,%.3f) to (%.3f,%.3f,%.3f)", x0, y0, z0, x1, y1, z1);
					prop = normalized_prop;
				}
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_TRIANGLE ||
				p->pending_block_type == WB_PENDING_BLOCK_SHADE_TRIANGLE) &&
				count_vec2_literals(raw_s) == 3)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "points %s", raw_s);
				prop = normalized_prop;
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_CURVE &&
				count_vec2_literals(raw_s) == 3)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "through %s", raw_s);
				prop = normalized_prop;
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_QUAD && count_vec2_literals(raw_s) == 4)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "points %s", raw_s);
				prop = normalized_prop;
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_POLYGON ||
				p->pending_block_type == WB_PENDING_BLOCK_SHADE_POLYGON ||
				p->pending_block_type == WB_PENDING_BLOCK_BLOB ||
				p->pending_block_type == WB_PENDING_BLOCK_SHADE_BLOB) &&
				count_vec2_literals(raw_s) >= 3)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "points %s", raw_s);
				prop = normalized_prop;
			}
			else if ((p->pending_block_type == WB_PENDING_BLOCK_TRIANGLE3D ||
				p->pending_block_type == WB_PENDING_BLOCK_SHADE_TRIANGLE3D) &&
				count_vec3_literals(raw_s) == 3)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "points %s", raw_s);
				prop = normalized_prop;
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_WIRE3D &&
				count_vec3_literals(raw_s) >= 3)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "points %s", raw_s);
				prop = normalized_prop;
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_SHADE_POLY3D &&
				count_vec3_literals(raw_s) >= 3)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "points %s", raw_s);
				prop = normalized_prop;
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_SURFACE3D &&
				count_vec3_literals(raw_s) == 4)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "points %s", raw_s);
				prop = normalized_prop;
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_MESH3D &&
				count_vec3_literals(raw_s) >= 3)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "vertices %s", raw_s);
				prop = normalized_prop;
			}
			else if (p->pending_block_type == WB_PENDING_BLOCK_TETRA3D && count_vec3_literals(raw_s) == 4)
			{
				snprintf(normalized_prop, sizeof(normalized_prop), "points %s", raw_s);
				prop = normalized_prop;
			}
			if (used + 1 < sizeof(p->pending_line))
			{
				snprintf(p->pending_line + used, sizeof(p->pending_line) - used, " %s", prop);
				return 1;
			}
			return set_error(p, line_no, "structured block is too long");
		}
		
		snprintf(pending_line, sizeof(pending_line), "%s", p->pending_line);
		p->pending_line[0] = 0;
		p->pending_line_no = 0;
		p->pending_block_type = WB_PENDING_BLOCK_NONE;
		p->pending_block_indent = 0;
		if (pending_line[0] == 0)
			goto pending_flushed;
		if (pending_indent > 0 && pending_indent + 1 < (int)sizeof(pending_with_indent) - 1)
		{
			int indent = pending_indent + 1;
			memset(pending_with_indent, ' ', sizeof(pending_with_indent));
			snprintf(pending_with_indent + indent, sizeof(pending_with_indent) - indent, "%s", pending_line);
			if (!parse_spec_line(p, pending_with_indent, line_no))
				return 0;
		}
		else
		{
			if (!parse_spec_line(p, pending_line, line_no))
				return 0;
		}
	}
pending_flushed:
	
	trim_right(s);
	apply_active_patch_to_line(p, s, sizeof(raw_copy) - (size_t)(s - raw_copy));
	if (*s)
	{
		size_t n = strlen(s);
		if (n > 0 && s[n - 1] == ':')
		{
			s[n - 1] = 0;
			trim_right(s);
			if (starts_with_word(s, "group"))
			{
				char group_name[64] = "";
				if (sscanf(s, "group %63s", group_name) != 1)
					return set_error(p, line_no, "expected group name:");
				if (p->n_group_scopes >= (int)(sizeof(p->group_scopes) / sizeof(p->group_scopes[0])))
					return set_error(p, line_no, "too many nested groups");
				snprintf(p->group_scopes[p->n_group_scopes].name, sizeof(p->group_scopes[p->n_group_scopes].name), "%s", group_name);
				p->group_scopes[p->n_group_scopes].indent = raw_indent;
				p->n_group_scopes++;
				return 1;
			}
			if (starts_with_word(s, "patch"))
			{
				if (!parse_patch(p, s, line_no))
					return 0;
				p->pending_line[0] = 0;
				p->pending_line_no = line_no;
				p->pending_block_type = WB_PENDING_BLOCK_PATCH;
				p->pending_block_indent = raw_indent;
				return 1;
			}
			if (starts_with_word(s, "space"))
				return parse_space(p, s, line_no);
			if (pending_block_type_for_line(s) != WB_PENDING_BLOCK_NONE)
			{
				snprintf(p->pending_line, sizeof(p->pending_line), "%s", s);
				p->pending_line_no = line_no;
				p->pending_block_type = pending_block_type_for_line(s);
				p->pending_block_indent = raw_indent;
				return 1;
			}
			if (starts_with_word(s, "scene") || starts_with_word(s, "video") ||
				starts_with_word(s, "transition") || starts_with_word(s, "background"))
			{
				if (pending_block_type_for_line(s) != WB_PENDING_BLOCK_NONE)
				{
					snprintf(p->pending_line, sizeof(p->pending_line), "%s", s);
					p->pending_line_no = line_no;
					p->pending_block_type = pending_block_type_for_line(s);
					p->pending_block_indent = raw_indent;
					return 1;
				}
			}
		}
	}
	
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

	{
		int objects_before = p->scene ? p->scene->n_objects : 0;
		int actions_before = p->scene ? p->scene->n_actions : 0;
		int ok = 0;
		if (starts_with_word(s, "video"))
			ok = parse_video(p, s, line_no);
		else if (starts_with(s, "scene "))
			ok = parse_scene(p, s, line_no);
		else if (starts_with(s, "transition "))
			ok = parse_transition(p, s, line_no);
		else
		{
			if (!p->scene && !start_new_scene(p, (float)FRAMES_PER_SCENE / FPS))
				return set_error(p, line_no, "failed to create default scene");
			if (starts_with_word(s, "layer"))
				ok = parse_layer(p, s, line_no);
			else if (starts_with_word(s, "space"))
				ok = parse_space(p, s, line_no);
			else if (starts_with_word(s, "camera") || strcmp(s, "cam") == 0 || starts_with(s, "cam "))
				ok = parse_camera(p, s, line_no);
			else if (starts_with_word(s, "background"))
				ok = parse_background(p, s, line_no);
			else if (starts_with_word(s, "defaults"))
				ok = parse_defaults(p, s, line_no);
			else if (starts_with_word(s, "math"))
				ok = parse_math(p, s, line_no);
			else if (starts_with_word(s, "text"))
				ok = parse_text(p, s, line_no);
			else if (starts_with_word(s, "curve"))
				ok = parse_curve_object(p, s, line_no);
			else if (starts_with_word(s, "line3d"))
				ok = parse_line3d_object(p, s, line_no);
			else if (starts_with_word(s, "curve3d"))
				ok = parse_curve3d_object(p, s, line_no);
			else if (starts_with_word(s, "wire3d") || starts_with_word(s, "wireframe3d") || starts_with_word(s, "polygon3d"))
				ok = parse_wire3d_object(p, s, line_no);
			else if (starts_with_word(s, "shade_poly3d") || starts_with_word(s, "shade_polygon3d"))
				ok = parse_shade_poly3d_object(p, s, line_no);
			else if (starts_with_word(s, "surface3d"))
				ok = parse_surface3d_object(p, s, line_no);
			else if (starts_with_word(s, "mesh3d"))
				ok = parse_mesh3d_object(p, s, line_no);
			else if (starts_with_word(s, "blob3d"))
				ok = parse_blob3d_object(p, s, line_no);
			else if (starts_with_word(s, "param3d") || starts_with_word(s, "parametric3d"))
				ok = parse_param3d_object(p, s, line_no);
			else if (starts_with_word(s, "param_surface3d") || starts_with_word(s, "parametric_surface3d"))
				ok = parse_param_surface3d_object(p, s, line_no);
			else if (starts_with_word(s, "volume3d") || starts_with_word(s, "ellipsoid3d"))
				ok = parse_volume3d_object(p, s, line_no);
			else if (starts_with_word(s, "point3d"))
				ok = parse_point3d_object(p, s, line_no);
			else if (starts_with_word(s, "open_point3d"))
				ok = parse_open_point3d_object(p, s, line_no);
			else if (starts_with_word(s, "axes3d") || starts_with_word(s, "axes"))
				ok = parse_axes3d_object(p, s, line_no);
			else if (starts_with_word(s, "cube3d") || starts_with_word(s, "cube"))
				ok = parse_cube3d_object(p, s, line_no);
			else if (starts_with_word(s, "tetrahedron3d") || starts_with_word(s, "tetra3d"))
				ok = parse_tetrahedron3d_object(p, s, line_no);
			else if (starts_with_word(s, "shade_triangle3d"))
				ok = parse_shade_triangle3d_object(p, s, line_no);
			else if (starts_with_word(s, "triangle3d"))
				ok = parse_triangle3d_object(p, s, line_no);
			else if (starts_with_word(s, "dotted_line"))
				ok = parse_dotted_line_object(p, s, line_no);
			else if (starts_with_word(s, "dashed_line") || starts_with_word(s, "dash"))
				ok = parse_dashed_line_object(p, s, line_no);
			else if (starts_with_word(s, "arrow"))
				ok = parse_arrow_object(p, s, line_no);
			else if (starts_with_word(s, "triangle") || starts_with_word(s, "tri"))
				ok = parse_triangle_object(p, s, line_no);
			else if (starts_with_word(s, "shade_triangle"))
				ok = parse_shade_triangle_object(p, s, line_no);
			else if (starts_with_word(s, "shade_polygon") || starts_with_word(s, "shade_poly"))
				ok = parse_shade_polygon_object(p, s, line_no);
			else if (starts_with_word(s, "shade_blob"))
				ok = parse_shade_blob_object(p, s, line_no);
			else if (starts_with_word(s, "quad"))
				ok = parse_quad_object(p, s, line_no);
			else if (starts_with_word(s, "polygon") || starts_with_word(s, "poly"))
				ok = parse_polygon_object(p, s, line_no);
			else if (starts_with_word(s, "blob"))
				ok = parse_blob_object(p, s, line_no);
			else if (starts_with_word(s, "ray"))
				ok = parse_ray_object(p, s, line_no);
			else if (starts_with_word(s, "line") || starts_with_word(s, "seg"))
				ok = parse_line_object(p, s, line_no);
			else if (starts_with_word(s, "circle") || starts_with_word(s, "circ"))
				ok = parse_circle_object(p, s, line_no);
			else if (starts_with_word(s, "ellipse") || starts_with_word(s, "ell"))
				ok = parse_ellipse_object(p, s, line_no);
			else if (starts_with_word(s, "shade_disc"))
				ok = parse_shade_disc_object(p, s, line_no);
			else if (starts_with_word(s, "point") || starts_with_word(s, "pt"))
				ok = parse_point_object(p, s, line_no, 0);
			else if (starts_with_word(s, "open_point") || starts_with_word(s, "opt"))
				ok = parse_point_object(p, s, line_no, 1);
			else if (starts_with(s, "move_layer "))
				ok = parse_move_layer(p, s, line_no);
			else if (starts_with(s, "move_patch "))
				ok = parse_move_patch(p, s, line_no);
			else if (starts_with(s, "turn_patch "))
				ok = parse_turn_patch(p, s, line_no);
			else if (starts_with(s, "scale_patch "))
				ok = parse_scale_patch(p, s, line_no);
			else if (starts_with(s, "turn "))
				ok = parse_turn(p, s, line_no);
			else if (starts_with(s, "scale "))
				ok = parse_scale(p, s, line_no);
			else if (starts_with(s, "move_camera "))
				ok = parse_move_camera(p, s, line_no);
			else if (starts_with(s, "orbit_camera "))
				ok = parse_orbit_camera(p, s, line_no);
			else if (starts_with(s, "fade_layer "))
				ok = parse_fade_layer(p, s, line_no);
			else if (starts_with(s, "fade "))
				ok = parse_fade(p, s, line_no);
			else if (starts_with(s, "move "))
				ok = parse_move(p, s, line_no);
			else if (starts_with(s, "draw "))
				ok = parse_draw(p, s, line_no);
			else if (starts_with_word(s, "patch"))
				ok = parse_patch(p, s, line_no);
		}
		if (ok)
		{
			if (p->scene)
			{
				for (int i = objects_before; i < p->scene->n_objects; i++)
				{
					p->scene->objects[i].patch_id = p->scene->current_patch_id;
					p->scene->objects[i].draw_order = ++p->scene->next_draw_order;
				}
				for (int i = actions_before; i < p->scene->n_actions; i++)
				{
					wb_scene_action *action = &p->scene->actions[i];
					action->patch_id = p->scene->current_patch_id;
					if (action->object_id > 0)
					{
						for (int j = 0; j < p->scene->n_objects; j++)
						{
							if (p->scene->objects[j].id == action->object_id)
							{
								action->patch_id = p->scene->objects[j].patch_id;
								break;
							}
						}
					}
				}
			}
			/* Flat scene content belongs to the implicit root manifold.  Explicit
			 * patches retain their own local coordinates and are flattened by the
			 * existing patch path. */
			if (p->scene && p->n_patch_scopes == 0 && current_layer_type(p) == WB_LAYER_2D)
			{
				for (int i = objects_before; i < p->scene->n_objects; i++)
					root_worldify_object(p->scene, &p->scene->objects[i]);
				for (int i = actions_before; i < p->scene->n_actions; i++)
					root_worldify_action(p->scene, &p->scene->actions[i]);
			}
			else if (p->scene && p->n_patch_scopes > 0 && current_layer_type(p) == WB_LAYER_2D)
			{
				float length_scale = active_patch_length_scale(p);
				for (int i = objects_before; i < p->scene->n_objects; i++)
					scale_2d_object_lengths(&p->scene->objects[i], length_scale);
			}
			if (p->n_group_scopes > 0 && p->scene)
			{
				for (int i = objects_before; i < p->scene->n_objects; i++)
				{
					for (int g = 0; g < p->n_group_scopes; g++)
						remember_group_member(p, p->group_scopes[g].name, p->scene->objects[i].id);
				}
			}
			return 1;
		}
	}
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
		expand_dimension_units(line, sizeof(line));
		expand_relative_coords(line, sizeof(line));
		if (!parse_spec_line(&parser, line, line_no))
			break;
	}
	if (!parser.error[0] && parser.pending_block_type != WB_PENDING_BLOCK_NONE)
	{
		char pending_line[2048];
		snprintf(pending_line, sizeof(pending_line), "%s", parser.pending_line);
		parser.pending_line[0] = 0;
		parser.pending_line_no = 0;
		parser.pending_block_type = WB_PENDING_BLOCK_NONE;
		parse_spec_line(&parser, pending_line, line_no);
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
