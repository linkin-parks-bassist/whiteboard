#include "whiteboard.h"

static wb_scene_layer *find_layer(wb_scene *scene, int layer_id);

void init_camera(wb_camera *camera)
{
	if (!camera)
		return;
	
	camera->position = vec3(0, 0, 0);
}

wb_scene *new_scene()
{
	wb_scene *result = malloc(sizeof(wb_scene));
	
	if (!result)
		return NULL;
	
	memset(result, 0, sizeof(wb_scene));
	result->next_object_id = 1;
	result->background_type = WB_BACKGROUND_RADIAL;
	result->background_center_colour = 0xFFFFFF;
	result->background_edge_colour = 0xF1F2F4;
	init_camera(&result->camera);
	wb_scene_add_layer(result, "default", WB_LAYER_2D, 1.0f);
	
	return result;
}

void free_scene(wb_scene *scene)
{
	if (!scene)
		return;
	
	for (int i = 0; i < scene->n_objects; i++)
	{
		if (scene->objects[i].type == WB_OBJECT_MATH)
			wb_math_free(scene->objects[i].math);
	}
	
	free(scene->layers);
	free(scene->objects);
	free(scene->actions);
	free(scene);
}

void wb_scene_set_radial_background(wb_scene *scene, uint32_t center_colour, uint32_t edge_colour)
{
	if (!scene)
		return;
	
	scene->background_type = WB_BACKGROUND_RADIAL;
	scene->background_center_colour = center_colour;
	scene->background_edge_colour = edge_colour;
}

int wb_scene_add_layer(wb_scene *scene, const char *name, int type, float opacity)
{
	wb_scene_layer *layer;
	
	if (!scene)
		return 0;
	
	if (scene->n_layers >= scene->cap_layers)
	{
		int new_cap = scene->cap_layers ? scene->cap_layers * 2 : 4;
		wb_scene_layer *layers = realloc(scene->layers, sizeof(wb_scene_layer) * new_cap);
		
		if (!layers)
			return 0;
		
		scene->layers = layers;
		scene->cap_layers = new_cap;
	}
	
	layer = &scene->layers[scene->n_layers];
	memset(layer, 0, sizeof(*layer));
	layer->id = scene->n_layers + 1;
	snprintf(layer->name, sizeof(layer->name), "%s", (name && *name) ? name : "layer");
	layer->type = type ? type : WB_LAYER_2D;
	layer->opacity = opacity;
	layer->blur_radius = 0.0f;
	layer->jitter_strength = 1.0f;
	layer->camera_distance = 5.0f;
	layer->camera_scale = 260.0f;
	layer->camera_center = vec2(WIDTH * 0.5f, HEIGHT * 0.5f);
	layer->offset = vec2(0, 0);
	layer->render_offset = layer->offset;
	scene->n_layers++;
	scene->current_layer_id = layer->id;
	
	return layer->id;
}

void wb_scene_set_layer_blur(wb_scene *scene, int layer_id, float blur_radius)
{
	wb_scene_layer *layer = find_layer(scene, layer_id);
	
	if (!layer)
		return;
	
	if (blur_radius < 0.0f)
		blur_radius = 0.0f;
	if (blur_radius > 32.0f)
		blur_radius = 32.0f;
	
	layer->blur_radius = blur_radius;
}

void wb_scene_set_layer_jitter(wb_scene *scene, int layer_id, float jitter_strength)
{
	wb_scene_layer *layer = find_layer(scene, layer_id);
	
	if (!layer)
		return;
	
	if (jitter_strength < 0.0f)
		jitter_strength = 0.0f;
	layer->jitter_strength = jitter_strength;
}

void wb_scene_set_layer_camera(wb_scene *scene, int layer_id, float distance, float scale, float center_x, float center_y)
{
	wb_scene_layer *layer = find_layer(scene, layer_id);
	
	if (!layer)
		return;
	
	if (distance < 0.1f)
		distance = 0.1f;
	if (scale < 1.0f)
		scale = 1.0f;
	
	layer->camera_distance = distance;
	layer->camera_scale = scale;
	layer->camera_center = vec2(center_x, center_y);
}

void wb_scene_set_current_layer(wb_scene *scene, int layer_id)
{
	if (!scene)
		return;
	
	for (int i = 0; i < scene->n_layers; i++)
	{
		if (scene->layers[i].id == layer_id)
		{
			scene->current_layer_id = layer_id;
			return;
		}
	}
}

static wb_scene_object *find_object(wb_scene *scene, int object_id)
{
	if (!scene)
		return NULL;
	
	for (int i = 0; i < scene->n_objects; i++)
	{
		if (scene->objects[i].id == object_id)
			return &scene->objects[i];
	}
	
	return NULL;
}

static wb_scene_layer *find_layer(wb_scene *scene, int layer_id)
{
	if (!scene)
		return NULL;
	
	for (int i = 0; i < scene->n_layers; i++)
	{
		if (scene->layers[i].id == layer_id)
			return &scene->layers[i];
	}
	
	return NULL;
}

void wb_scene_set_object_jitter(wb_scene *scene, int object_id, float jitter_strength)
{
	wb_scene_object *obj = find_object(scene, object_id);
	
	if (!obj)
		return;
	
	if (jitter_strength < 0.0f)
		jitter_strength = 0.0f;
	obj->jitter_strength = jitter_strength;
}

static wb_scene_object *append_object(wb_scene *scene)
{
	if (!scene)
		return NULL;
	
	if (scene->n_objects >= scene->cap_objects)
	{
		int new_cap = scene->cap_objects ? scene->cap_objects * 2 : 8;
		wb_scene_object *objects = realloc(scene->objects, sizeof(wb_scene_object) * new_cap);
		
		if (!objects)
			return NULL;
		
		scene->objects = objects;
		scene->cap_objects = new_cap;
	}
	
	return &scene->objects[scene->n_objects++];
}

static wb_scene_action *append_action(wb_scene *scene)
{
	if (!scene)
		return NULL;
	
	if (scene->n_actions >= scene->cap_actions)
	{
		int new_cap = scene->cap_actions ? scene->cap_actions * 2 : 8;
		wb_scene_action *actions = realloc(scene->actions, sizeof(wb_scene_action) * new_cap);
		
		if (!actions)
			return NULL;
		
		scene->actions = actions;
		scene->cap_actions = new_cap;
	}
	
	return &scene->actions[scene->n_actions++];
}

int wb_scene_add_math(wb_scene *scene, const char *src, float x, float y, float size, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_MATH;
	obj->layer_id = scene->current_layer_id;
	obj->math = wb_math_parse(src);
	obj->x = x;
	obj->y = y;
	obj->size = size;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	if (!obj->math)
		return 0;
	
	return obj->id;
}

int wb_scene_add_line(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_LINE;
	obj->layer_id = scene->current_layer_id;
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_ray(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_RAY;
	obj->layer_id = scene->current_layer_id;
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_dotted_line(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, float gap, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_DOTTED_LINE;
	obj->layer_id = scene->current_layer_id;
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->thickness = thickness;
	obj->size = gap;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_arrow(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, float head_size, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_ARROW;
	obj->layer_id = scene->current_layer_id;
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->thickness = thickness;
	obj->size = head_size;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_triangle(wb_scene *scene, float x0, float y0, float x1, float y1, float x2, float y2, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_TRIANGLE;
	obj->layer_id = scene->current_layer_id;
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->x = x2;
	obj->y = y2;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_shade_triangle(wb_scene *scene, float x0, float y0, float x1, float y1, float x2, float y2, uint32_t colour, float opacity)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_SHADE_TRIANGLE;
	obj->layer_id = scene->current_layer_id;
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->x = x2;
	obj->y = y2;
	obj->size = opacity;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_quad(wb_scene *scene, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_QUAD;
	obj->layer_id = scene->current_layer_id;
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->q0 = vec3(x2, y2, 0);
	obj->q1 = vec3(x3, y3, 0);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_polygon(wb_scene *scene, const wb_vec2 *points, int n_points, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj || !points || n_points < 3 || n_points > 7)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_POLYGON;
	obj->layer_id = scene->current_layer_id;
	obj->p0 = points[0];
	obj->p1 = points[1];
	obj->q0 = vec3(points[2].x, points[2].y, 0);
	obj->q1 = vec3(points[3].x, points[3].y, 0);
	obj->q2 = vec3(points[4].x, points[4].y, 0);
	obj->x = points[5].x;
	obj->y = points[5].y;
	obj->size = points[6].y;
	obj->q2.z = points[6].x;
	obj->radius = (float)n_points;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	if (n_points < 4)
		obj->q1 = obj->q0;
	if (n_points < 5)
		obj->q2 = obj->q1;
	if (n_points < 6)
	{
		obj->x = obj->q2.x;
		obj->y = obj->q2.y;
	}
	if (n_points < 7)
	{
		obj->q2.z = obj->x;
		obj->size = obj->y;
	}
	
	return obj->id;
}

int wb_scene_add_line3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_LINE3D;
	obj->layer_id = scene->current_layer_id;
	obj->q0 = vec3(x0, y0, z0);
	obj->q1 = vec3(x1, y1, z1);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_curve3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_CURVE3D;
	obj->layer_id = scene->current_layer_id;
	obj->q0 = vec3(x0, y0, z0);
	obj->q1 = vec3(x1, y1, z1);
	obj->q2 = vec3(x2, y2, z2);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_point(wb_scene *scene, float x, float y, float radius, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_POINT;
	obj->layer_id = scene->current_layer_id;
	obj->x = x;
	obj->y = y;
	obj->radius = radius;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_open_point(wb_scene *scene, float x, float y, float radius, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_OPEN_POINT;
	obj->layer_id = scene->current_layer_id;
	obj->x = x;
	obj->y = y;
	obj->radius = radius;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_circle(wb_scene *scene, float x, float y, float radius, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_CIRCLE;
	obj->layer_id = scene->current_layer_id;
	obj->x = x;
	obj->y = y;
	obj->radius = radius;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_ellipse(wb_scene *scene, float x, float y, float radius_x, float radius_y, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_ELLIPSE;
	obj->layer_id = scene->current_layer_id;
	obj->x = x;
	obj->y = y;
	obj->p0 = vec2(radius_x, radius_y);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

int wb_scene_add_shade_disc(wb_scene *scene, float x, float y, float radius, uint32_t colour, float opacity)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_SHADE_DISC;
	obj->layer_id = scene->current_layer_id;
	obj->x = x;
	obj->y = y;
	obj->radius = radius;
	obj->size = opacity;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = 1.0f;
	
	return obj->id;
}

void wb_scene_move(wb_scene *scene, int object_id, float start_time, float end_time, float x1, float y1, float x2, float y2)
{
	wb_scene_action *action = append_action(scene);
	
	if (!action)
		return;
	
	action->object_id = object_id;
	action->type = WB_ACTION_MOVE;
	action->start_time = start_time;
	action->end_time = end_time;
	action->from = vec2(x1, y1);
	action->to = vec2(x2, y2);
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_move_layer(wb_scene *scene, int layer_id, float start_time, float end_time, float x1, float y1, float x2, float y2)
{
	wb_scene_action *action = append_action(scene);
	
	if (!action)
		return;
	
	action->object_id = 0;
	action->layer_id = layer_id;
	action->type = WB_ACTION_LAYER_MOVE;
	action->start_time = start_time;
	action->end_time = end_time;
	action->from = vec2(x1, y1);
	action->to = vec2(x2, y2);
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_draw_in(wb_scene *scene, int object_id, float start_time, float end_time)
{
	wb_scene_action *action = append_action(scene);
	
	if (!action)
		return;
	
	action->object_id = object_id;
	action->type = WB_ACTION_DRAW;
	action->start_time = start_time;
	action->end_time = end_time;
	action->from = vec2(0, 0);
	action->to = vec2(1, 0);
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

static float clamp01(float x)
{
	if (x < 0.0f)
		return 0.0f;
	
	if (x > 1.0f)
		return 1.0f;
	
	return x;
}

float wb_ease_grassroots(float t)
{
	t = clamp01(t);
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float action_alpha(wb_scene_action *action, float time)
{
	if (!action || action->end_time <= action->start_time)
		return 1.0f;
	
	return wb_ease_grassroots((time - action->start_time) / (action->end_time - action->start_time));
}

static float scene_seeded_unit(int seed)
{
	uint32_t x = (uint32_t)seed;
	x ^= x >> 16;
	x *= 0x7feb352dU;
	x ^= x >> 15;
	x *= 0x846ca68bU;
	x ^= x >> 16;
	return (float)(x & 0xffff) / 65535.0f;
}

static wb_nurbs_pcurve *new_jittered_line_curve(wb_vec2 a, wb_vec2 b, float thickness, float jitter_strength, int seed)
{
	wb_nurbs_pcurve *curve = malloc(sizeof(*curve));
	wb_vec2 d = vec2_diff(b, a);
	wb_vec2 n = vec2_perp(vec2_normalised(d));
	float amp = binary_max(1.0f, thickness * 0.65f) * jitter_strength;
	
	if (!curve)
		return NULL;
	
	curve->nx = new_nurbs(3, 5);
	curve->ny = new_nurbs(3, 5);
	curve->colour = 0;
	
	if (!curve->nx || !curve->ny)
	{
		free_nurbs_pcurve(curve);
		return NULL;
	}
	
	for (int i = 0; i < 5; i++)
	{
		float t = (float)i / 4.0f;
		float along = (scene_seeded_unit(seed + i * 97) * 2.0f - 1.0f) * amp * 0.35f;
		float across = (scene_seeded_unit(seed + i * 193 + 17) * 2.0f - 1.0f) * amp;
		
		if (i == 0 || i == 4)
		{
			along *= 0.25f;
			across *= 0.35f;
		}
		
		curve->nx->control_points[i] = a.x + d.x * t + d.x * along / binary_max(1.0f, vec2_norm(d)) + n.x * across;
		curve->ny->control_points[i] = a.y + d.y * t + d.y * along / binary_max(1.0f, vec2_norm(d)) + n.y * across;
	}
	
	return curve;
}

static void draw_curve_stroke_progress(uint8_t *buf, wb_nurbs_pcurve *curve, float thickness, uint32_t colour, float progress)
{
	wb_plane_polyline *pl;
	int visible_points;
	
	if (!buf || !curve)
		return;
	
	progress = clamp01(progress);
	if (progress <= 0.0f)
		return;
	
	pl = nurbs_pcurve_to_ppolyline(curve, N_SAMPLE_POINTS, thickness);
	if (!pl)
		return;
	
	visible_points = 1 + (int)((pl->n_points - 1) * progress);
	if (visible_points < 2)
		visible_points = 2;
	if (visible_points < pl->n_points)
		pl->n_points = visible_points;
	
	draw_ppolyline_in_colour(buf, pl, colour);
	free_plane_polyline(pl);
}

static void draw_curve_stroke(uint8_t *buf, wb_nurbs_pcurve *curve, float thickness, uint32_t colour)
{
	draw_curve_stroke_progress(buf, curve, thickness, colour, 1.0f);
}

static void draw_hand_open_point(uint8_t *buf, float x, float y, float radius, float thickness, uint32_t colour, float jitter_strength, int seed, float progress);
static void draw_hand_line(uint8_t *buf, wb_vec2 a, wb_vec2 b, float thickness, uint32_t colour, float jitter_strength, int seed, float progress);
static void draw_hand_ellipse(uint8_t *buf, float x, float y, float radius_x, float radius_y, float thickness, uint32_t colour, float jitter_strength, int seed, float progress);

static void draw_hand_line(uint8_t *buf, wb_vec2 a, wb_vec2 b, float thickness, uint32_t colour, float jitter_strength, int seed, float progress)
{
	wb_nurbs_pcurve *curve = new_jittered_line_curve(a, b, thickness, jitter_strength, seed);
	wb_vec2 partial_b;
	
	progress = clamp01(progress);
	partial_b = vec2(a.x + (b.x - a.x) * progress, a.y + (b.y - a.y) * progress);
	
	if (!curve)
	{
		draw_sausage(buf, a, partial_b, thickness, colour);
		return;
	}
	
	draw_curve_stroke_progress(buf, curve, thickness, colour, progress);
	free_nurbs_pcurve(curve);
}

static wb_vec2 extend_ray_endpoint(wb_vec2 a, wb_vec2 through)
{
	wb_vec2 dir = vec2_diff(through, a);
	float len = vec2_norm(dir);
	float t = 0.0f;
	
	if (len <= 0.001f)
		return through;
	
	dir = vec2(dir.x / len, dir.y / len);
	
	if (fabsf(dir.x) > 0.0001f)
	{
		float tx = dir.x > 0.0f ? ((float)WIDTH + 120.0f - a.x) / dir.x : (-120.0f - a.x) / dir.x;
		if (tx > t)
			t = tx;
	}
	if (fabsf(dir.y) > 0.0001f)
	{
		float ty = dir.y > 0.0f ? ((float)HEIGHT + 120.0f - a.y) / dir.y : (-120.0f - a.y) / dir.y;
		if (ty > t)
			t = ty;
	}
	if (t < len)
		t = len + 120.0f;
	
	return vec2(a.x + dir.x * t, a.y + dir.y * t);
}

static void draw_hand_dotted_line(uint8_t *buf, wb_vec2 a, wb_vec2 b, float thickness, float gap, uint32_t colour, float jitter_strength, int seed, float progress)
{
	wb_vec2 d = vec2_diff(b, a);
	float len = vec2_norm(d);
	int n_dots;
	
	if (!buf || len <= 0.0f || progress <= 0.0f)
		return;
	
	if (gap < thickness * 2.5f)
		gap = thickness * 2.5f;
	n_dots = 1 + (int)(len / gap);
	
	for (int i = 0; i < n_dots; i++)
	{
		float t = n_dots <= 1 ? 0.0f : (float)i / (float)(n_dots - 1);
		wb_vec2 c;
		
		if (t > progress)
			break;
		
		c = vec2(a.x + d.x * t, a.y + d.y * t);
		draw_hand_open_point(buf, c.x, c.y, thickness * 0.9f, thickness, colour, jitter_strength, seed + i * 811, 1.0f);
	}
}

static void draw_hand_arrow(uint8_t *buf, wb_vec2 a, wb_vec2 b, float thickness, float head_size, uint32_t colour, float jitter_strength, int seed, float progress)
{
	wb_vec2 d = vec2_diff(b, a);
	float len = vec2_norm(d);
	wb_vec2 dir;
	wb_vec2 normal;
	wb_vec2 left;
	wb_vec2 right;
	float shaft_progress;
	float head_progress;
	
	if (!buf || len <= 0.0f || progress <= 0.0f)
		return;
	
	progress = clamp01(progress);
	if (head_size <= 0.0f)
		head_size = binary_max(12.0f, thickness * 5.0f);
	
	dir = vec2_normalised(d);
	normal = vec2_perp(dir);
	left = vec2(b.x - dir.x * head_size + normal.x * head_size * 0.45f, b.y - dir.y * head_size + normal.y * head_size * 0.45f);
	right = vec2(b.x - dir.x * head_size - normal.x * head_size * 0.45f, b.y - dir.y * head_size - normal.y * head_size * 0.45f);
	
	shaft_progress = progress < 0.85f ? progress / 0.85f : 1.0f;
	head_progress = progress <= 0.85f ? 0.0f : (progress - 0.85f) / 0.15f;
	draw_hand_line(buf, a, b, thickness, colour, jitter_strength, seed, shaft_progress);
	if (head_progress > 0.0f)
	{
		draw_hand_line(buf, b, left, thickness, colour, jitter_strength, seed + 103, head_progress);
		draw_hand_line(buf, b, right, thickness, colour, jitter_strength, seed + 211, head_progress);
	}
}

static void draw_hand_triangle(uint8_t *buf, wb_vec2 a, wb_vec2 b, wb_vec2 c, float thickness, uint32_t colour, float jitter_strength, int seed, float progress)
{
	float p;
	
	if (!buf || progress <= 0.0f)
		return;
	
	progress = clamp01(progress);
	p = progress * 3.0f;
	if (p > 0.0f)
		draw_hand_line(buf, a, b, thickness, colour, jitter_strength, seed + 101, p < 1.0f ? p : 1.0f);
	if (p > 1.0f)
		draw_hand_line(buf, b, c, thickness, colour, jitter_strength, seed + 211, p < 2.0f ? p - 1.0f : 1.0f);
	if (p > 2.0f)
		draw_hand_line(buf, c, a, thickness, colour, jitter_strength, seed + 307, p - 2.0f);
}

static void draw_hand_quad(uint8_t *buf, wb_vec2 a, wb_vec2 b, wb_vec2 c, wb_vec2 d, float thickness, uint32_t colour, float jitter_strength, int seed, float progress)
{
	float p;
	
	if (!buf || progress <= 0.0f)
		return;
	
	progress = clamp01(progress);
	p = progress * 4.0f;
	if (p > 0.0f)
		draw_hand_line(buf, a, b, thickness, colour, jitter_strength, seed + 101, p < 1.0f ? p : 1.0f);
	if (p > 1.0f)
		draw_hand_line(buf, b, c, thickness, colour, jitter_strength, seed + 211, p < 2.0f ? p - 1.0f : 1.0f);
	if (p > 2.0f)
		draw_hand_line(buf, c, d, thickness, colour, jitter_strength, seed + 307, p < 3.0f ? p - 2.0f : 1.0f);
	if (p > 3.0f)
		draw_hand_line(buf, d, a, thickness, colour, jitter_strength, seed + 409, p - 3.0f);
}

static void draw_hand_polygon(uint8_t *buf, const wb_vec2 *points, int n_points, float thickness, uint32_t colour, float jitter_strength, int seed, float progress)
{
	float p;
	
	if (!buf || !points || n_points < 3 || progress <= 0.0f)
		return;
	
	progress = clamp01(progress);
	p = progress * (float)n_points;
	for (int i = 0; i < n_points; i++)
	{
		float edge_progress = p - (float)i;
		if (edge_progress <= 0.0f)
			break;
		draw_hand_line(buf, points[i], points[(i + 1) % n_points], thickness, colour, jitter_strength, seed + 101 * (i + 1), edge_progress > 1.0f ? 1.0f : edge_progress);
	}
}

static void draw_hand_open_point(uint8_t *buf, float x, float y, float radius, float thickness, uint32_t colour, float jitter_strength, int seed, float progress)
{
	wb_nurbs_pcurve *curve = circle_nurbs_pcurve(x, y, radius, 9, scene_seeded_unit(seed + 401) * TAU);
	
	if (!curve)
		return;
	
	if (jitter_strength > 0.0f)
		jitter_nurbs_pcurve(curve, binary_max(0.6f, thickness * 0.45f) * jitter_strength);
	draw_curve_stroke_progress(buf, curve, thickness, colour, progress);
	free_nurbs_pcurve(curve);
}

static void draw_hand_ellipse(uint8_t *buf, float x, float y, float radius_x, float radius_y, float thickness, uint32_t colour, float jitter_strength, int seed, float progress)
{
	wb_nurbs_pcurve *curve = circle_nurbs_pcurve(x, y, 1.0f, 9, scene_seeded_unit(seed + 401) * TAU);
	
	if (!curve)
		return;
	
	for (int i = 0; i < curve->nx->n_control_points; i++)
	{
		curve->nx->control_points[i] = x + (curve->nx->control_points[i] - x) * radius_x;
		curve->ny->control_points[i] = y + (curve->ny->control_points[i] - y) * radius_y;
	}
	
	if (jitter_strength > 0.0f)
		jitter_nurbs_pcurve(curve, binary_max(0.6f, thickness * 0.45f) * jitter_strength);
	draw_curve_stroke_progress(buf, curve, thickness, colour, progress);
	free_nurbs_pcurve(curve);
}

static int project_3d_point(wb_vec3 p, wb_scene_layer *layer, wb_vec2 *out)
{
	float camera_distance = layer ? layer->camera_distance : 5.0f;
	float scale = layer ? layer->camera_scale : 260.0f;
	wb_vec2 center = layer ? layer->camera_center : vec2(WIDTH * 0.5f, HEIGHT * 0.5f);
	float z = p.z + camera_distance;
	
	if (!out || z <= 0.1f)
		return 0;
	
	out->x = center.x + (p.x / z) * scale;
	out->y = center.y - (p.y / z) * scale;
	return 1;
}

static wb_nurbs_pcurve *new_projected_curve3d(wb_vec3 q0, wb_vec3 q1, wb_vec3 q2, wb_scene_layer *layer)
{
	wb_vec2 p0;
	wb_vec2 p1;
	wb_vec2 p2;
	wb_nurbs_pcurve *curve;
	
	if (!project_3d_point(q0, layer, &p0) || !project_3d_point(q1, layer, &p1) || !project_3d_point(q2, layer, &p2))
		return NULL;
	
	curve = malloc(sizeof(*curve));
	if (!curve)
		return NULL;
	
	curve->nx = new_nurbs(2, 3);
	curve->ny = new_nurbs(2, 3);
	curve->colour = 0;
	
	if (!curve->nx || !curve->ny)
	{
		free_nurbs_pcurve(curve);
		return NULL;
	}
	
	curve->nx->control_points[0] = p0.x;
	curve->nx->control_points[1] = p1.x;
	curve->nx->control_points[2] = p2.x;
	curve->ny->control_points[0] = p0.y;
	curve->ny->control_points[1] = p1.y;
	curve->ny->control_points[2] = p2.y;
	return curve;
}

static void draw_scene_object(wb_scene_object *obj, wb_scene_layer *layer, int frame, uint8_t *buf)
{
	wb_vec2 layer_offset;
	float jitter_strength;
	
	if (!obj || obj->draw_progress <= 0.0f)
		return;
	
	layer_offset = layer ? layer->render_offset : vec2(0, 0);
	jitter_strength = obj->jitter_strength * (layer ? layer->jitter_strength : 1.0f);
	
	if (obj->type == WB_OBJECT_MATH)
	{
		wb_set_math_jitter_strength(jitter_strength);
		wb_math_draw_seeded(obj->math ? buf : NULL, obj->math, obj->x + layer_offset.x, obj->y + layer_offset.y, obj->size, obj->colour, frame + obj->id * 1009);
		wb_set_math_jitter_strength(1.0f);
	}
	else if (obj->type == WB_OBJECT_LINE)
		draw_hand_line(buf, vec2(obj->x + obj->p0.x + layer_offset.x, obj->y + obj->p0.y + layer_offset.y), vec2(obj->x + obj->p1.x + layer_offset.x, obj->y + obj->p1.y + layer_offset.y), obj->thickness, obj->colour, jitter_strength, frame + obj->id * 4099, obj->draw_progress);
	else if (obj->type == WB_OBJECT_RAY)
	{
		wb_vec2 a = vec2(obj->x + obj->p0.x + layer_offset.x, obj->y + obj->p0.y + layer_offset.y);
		wb_vec2 through = vec2(obj->x + obj->p1.x + layer_offset.x, obj->y + obj->p1.y + layer_offset.y);
		draw_hand_line(buf, a, extend_ray_endpoint(a, through), obj->thickness, obj->colour, jitter_strength, frame + obj->id * 4421, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_DOTTED_LINE)
		draw_hand_dotted_line(buf, vec2(obj->x + obj->p0.x + layer_offset.x, obj->y + obj->p0.y + layer_offset.y), vec2(obj->x + obj->p1.x + layer_offset.x, obj->y + obj->p1.y + layer_offset.y), obj->thickness, obj->size, obj->colour, jitter_strength, frame + obj->id * 5003, obj->draw_progress);
	else if (obj->type == WB_OBJECT_ARROW)
		draw_hand_arrow(buf, vec2(obj->x + obj->p0.x + layer_offset.x, obj->y + obj->p0.y + layer_offset.y), vec2(obj->x + obj->p1.x + layer_offset.x, obj->y + obj->p1.y + layer_offset.y), obj->thickness, obj->size, obj->colour, jitter_strength, frame + obj->id * 6947, obj->draw_progress);
	else if (obj->type == WB_OBJECT_TRIANGLE)
		draw_hand_triangle(buf, vec2(obj->p0.x + layer_offset.x, obj->p0.y + layer_offset.y), vec2(obj->p1.x + layer_offset.x, obj->p1.y + layer_offset.y), vec2(obj->x + layer_offset.x, obj->y + layer_offset.y), obj->thickness, obj->colour, jitter_strength, frame + obj->id * 7103, obj->draw_progress);
	else if (obj->type == WB_OBJECT_SHADE_TRIANGLE)
		draw_triangle_with_alpha(buf, vec2(obj->p0.x + layer_offset.x, obj->p0.y + layer_offset.y), vec2(obj->p1.x + layer_offset.x, obj->p1.y + layer_offset.y), vec2(obj->x + layer_offset.x, obj->y + layer_offset.y), obj->colour, obj->size * obj->draw_progress);
	else if (obj->type == WB_OBJECT_QUAD)
		draw_hand_quad(buf, vec2(obj->p0.x + layer_offset.x, obj->p0.y + layer_offset.y), vec2(obj->p1.x + layer_offset.x, obj->p1.y + layer_offset.y), vec2(obj->q0.x + layer_offset.x, obj->q0.y + layer_offset.y), vec2(obj->q1.x + layer_offset.x, obj->q1.y + layer_offset.y), obj->thickness, obj->colour, jitter_strength, frame + obj->id * 7349, obj->draw_progress);
	else if (obj->type == WB_OBJECT_POLYGON)
	{
		wb_vec2 points[7];
		int n_points = (int)(obj->radius + 0.5f);
		
		points[0] = vec2(obj->p0.x + layer_offset.x, obj->p0.y + layer_offset.y);
		points[1] = vec2(obj->p1.x + layer_offset.x, obj->p1.y + layer_offset.y);
		points[2] = vec2(obj->q0.x + layer_offset.x, obj->q0.y + layer_offset.y);
		points[3] = vec2(obj->q1.x + layer_offset.x, obj->q1.y + layer_offset.y);
		points[4] = vec2(obj->q2.x + layer_offset.x, obj->q2.y + layer_offset.y);
		points[5] = vec2(obj->x + layer_offset.x, obj->y + layer_offset.y);
		points[6] = vec2(obj->q2.z + layer_offset.x, obj->size + layer_offset.y);
		draw_hand_polygon(buf, points, n_points, obj->thickness, obj->colour, jitter_strength, frame + obj->id * 7523, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_SHADE_DISC)
		draw_disc_with_alpha(buf, obj->x + layer_offset.x, obj->y + layer_offset.y, obj->radius, obj->colour, obj->size * obj->draw_progress);
	else if (obj->type == WB_OBJECT_POINT)
		draw_disc(buf, obj->x + layer_offset.x, obj->y + layer_offset.y, obj->radius, obj->colour);
	else if (obj->type == WB_OBJECT_OPEN_POINT)
		draw_hand_open_point(buf, obj->x + layer_offset.x, obj->y + layer_offset.y, obj->radius, obj->thickness, obj->colour, jitter_strength, frame + obj->id * 6151, obj->draw_progress);
	else if (obj->type == WB_OBJECT_CIRCLE)
		draw_hand_open_point(buf, obj->x + layer_offset.x, obj->y + layer_offset.y, obj->radius, obj->thickness, obj->colour, jitter_strength, frame + obj->id * 6151, obj->draw_progress);
	else if (obj->type == WB_OBJECT_ELLIPSE)
		draw_hand_ellipse(buf, obj->x + layer_offset.x, obj->y + layer_offset.y, obj->p0.x, obj->p0.y, obj->thickness, obj->colour, jitter_strength, frame + obj->id * 6553, obj->draw_progress);
	else if (obj->type == WB_OBJECT_LINE3D)
	{
		wb_vec2 a;
		wb_vec2 b;
		
		if (project_3d_point(obj->q0, layer, &a) && project_3d_point(obj->q1, layer, &b))
			draw_hand_line(buf, vec2(a.x + layer_offset.x, a.y + layer_offset.y), vec2(b.x + layer_offset.x, b.y + layer_offset.y), obj->thickness, obj->colour, jitter_strength, frame + obj->id * 7901, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_CURVE3D)
	{
		wb_nurbs_pcurve *curve = new_projected_curve3d(obj->q0, obj->q1, obj->q2, layer);
		
		if (!curve)
			return;
		
		translate_nurbs_pcurve(curve, layer_offset.x, layer_offset.y);
		if (jitter_strength > 0.0f)
			jitter_nurbs_pcurve(curve, binary_max(0.6f, obj->thickness * 0.45f) * jitter_strength);
		draw_curve_stroke_progress(buf, curve, obj->thickness, obj->colour, obj->draw_progress);
		free_nurbs_pcurve(curve);
	}
}

static void clear_layer_buffer(uint8_t *buf)
{
	if (!buf)
		return;
	memset(buf, 0, WIDTH * HEIGHT * 3);
}

static void clear_alpha_buffer(uint8_t *alpha)
{
	if (!alpha)
		return;
	memset(alpha, 0, WIDTH * HEIGHT);
}

static void composite_layer_buffer(uint8_t *dst, uint8_t *src, uint8_t *alpha, float opacity)
{
	if (!dst || !src || !alpha || opacity <= 0.0f)
		return;
	
	if (opacity > 1.0f)
		opacity = 1.0f;
	
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		int ind = i * 3;
		int b = src[ind + 0];
		int g = src[ind + 1];
		int r = src[ind + 2];
		float a = ((float)alpha[i] / 255.0f) * opacity;
		
		if (a <= 0.0f)
			continue;
		
		float inv = 1.0f - a;
		dst[ind + 0] = (uint8_t)(dst[ind + 0] * inv + b * a);
		dst[ind + 1] = (uint8_t)(dst[ind + 1] * inv + g * a);
		dst[ind + 2] = (uint8_t)(dst[ind + 2] * inv + r * a);
	}
}

static void blur_alpha_buffer(uint8_t *buf, uint8_t *scratch, float radius)
{
	int r = (int)ceilf(radius);
	float weights[65];
	float sigma;
	float total_weight = 0.0f;
	
	if (!buf || !scratch || r <= 0)
		return;
	if (r > 32)
		r = 32;
	
	sigma = binary_max(0.5f, radius * 0.5f);
	for (int i = -r; i <= r; i++)
	{
		float w = expf(-((float)(i * i)) / (2.0f * sigma * sigma));
		weights[i + r] = w;
		total_weight += w;
	}
	
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			float sum = 0.0f;
			float sum_w = 0.0f;
			
			for (int ox = -r; ox <= r; ox++)
			{
				int sx = x + ox;
				float w = weights[ox + r];
				
				if (sx < 0 || sx >= WIDTH)
					continue;
				
				sum += (float)buf[y * WIDTH + sx] * w;
				sum_w += w;
			}
			
			if (sum_w <= 0.0f)
				sum_w = total_weight;
			scratch[y * WIDTH + x] = (uint8_t)(sum / sum_w);
		}
	}
	
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			float sum = 0.0f;
			float sum_w = 0.0f;
			
			for (int oy = -r; oy <= r; oy++)
			{
				int sy = y + oy;
				float w = weights[oy + r];
				
				if (sy < 0 || sy >= HEIGHT)
					continue;
				
				sum += (float)scratch[sy * WIDTH + x] * w;
				sum_w += w;
			}
			
			if (sum_w <= 0.0f)
				sum_w = total_weight;
			buf[y * WIDTH + x] = (uint8_t)(sum / sum_w);
		}
	}
}

static void blur_layer_buffer(uint8_t *buf, uint8_t *scratch, float radius)
{
	int r = (int)ceilf(radius);
	float weights[65];
	float sigma;
	float total_weight = 0.0f;
	
	if (!buf || !scratch || r <= 0)
		return;
	if (r > 32)
		r = 32;
	
	sigma = binary_max(0.5f, radius * 0.5f);
	for (int i = -r; i <= r; i++)
	{
		float w = expf(-((float)(i * i)) / (2.0f * sigma * sigma));
		weights[i + r] = w;
		total_weight += w;
	}
	
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			float sum_b = 0.0f;
			float sum_g = 0.0f;
			float sum_r = 0.0f;
			float sum_w = 0.0f;
			int dst_ind = (y * WIDTH + x) * 3;
			
			for (int ox = -r; ox <= r; ox++)
			{
				int sx = x + ox;
				float w = weights[ox + r];
				
				if (sx < 0 || sx >= WIDTH)
					continue;
				
				int src_ind = (y * WIDTH + sx) * 3;
				sum_b += (float)buf[src_ind + 0] * w;
				sum_g += (float)buf[src_ind + 1] * w;
				sum_r += (float)buf[src_ind + 2] * w;
				sum_w += w;
			}
			
			if (sum_w <= 0.0f)
				sum_w = total_weight;
			scratch[dst_ind + 0] = (uint8_t)(sum_b / sum_w);
			scratch[dst_ind + 1] = (uint8_t)(sum_g / sum_w);
			scratch[dst_ind + 2] = (uint8_t)(sum_r / sum_w);
		}
	}
	
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			float sum_b = 0.0f;
			float sum_g = 0.0f;
			float sum_r = 0.0f;
			float sum_w = 0.0f;
			int dst_ind = (y * WIDTH + x) * 3;
			
			for (int oy = -r; oy <= r; oy++)
			{
				int sy = y + oy;
				float w = weights[oy + r];
				
				if (sy < 0 || sy >= HEIGHT)
					continue;
				
				int src_ind = (sy * WIDTH + x) * 3;
				sum_b += (float)scratch[src_ind + 0] * w;
				sum_g += (float)scratch[src_ind + 1] * w;
				sum_r += (float)scratch[src_ind + 2] * w;
				sum_w += w;
			}
			
			if (sum_w <= 0.0f)
				sum_w = total_weight;
			buf[dst_ind + 0] = (uint8_t)(sum_b / sum_w);
			buf[dst_ind + 1] = (uint8_t)(sum_g / sum_w);
			buf[dst_ind + 2] = (uint8_t)(sum_r / sum_w);
		}
	}
}

void wb_scene_render(wb_scene *scene, float time, int frame, uint8_t *buf)
{
	uint8_t *layer_buf;
	uint8_t *scratch_buf;
	uint8_t *layer_alpha;
	uint8_t *scratch_alpha;
	
	if (!scene || !buf)
		return;
	
	if (scene->background_type == WB_BACKGROUND_RADIAL)
		fill_with_radial_gradient(buf, scene->background_center_colour, scene->background_edge_colour);
	else
		fill_with_colour(buf, 0xFFFFFF);
	
	for (int i = 0; i < scene->n_objects; i++)
		scene->objects[i].draw_progress = 1.0f;
	for (int i = 0; i < scene->n_layers; i++)
		scene->layers[i].render_offset = scene->layers[i].offset;
	
	for (int i = 0; i < scene->n_actions; i++)
	{
		wb_scene_action *action = &scene->actions[i];
		wb_scene_object *obj = find_object(scene, action->object_id);
		
		if (action->type == WB_ACTION_MOVE)
		{
			if (!obj)
				continue;
			float a = action_alpha(action, time);
			obj->x = action->from.x + (action->to.x - action->from.x) * a;
			obj->y = action->from.y + (action->to.y - action->from.y) * a;
		}
		else if (action->type == WB_ACTION_DRAW)
		{
			if (!obj)
				continue;
			if (time < action->start_time)
				obj->draw_progress = 0.0f;
			else if (time < action->end_time)
				obj->draw_progress = action_alpha(action, time);
		}
		else if (action->type == WB_ACTION_LAYER_MOVE)
		{
			wb_scene_layer *layer = find_layer(scene, action->layer_id);
			
			if (!layer)
				continue;
			
			float a = action_alpha(action, time);
			layer->render_offset.x = action->from.x + (action->to.x - action->from.x) * a;
			layer->render_offset.y = action->from.y + (action->to.y - action->from.y) * a;
		}
	}
	
	layer_buf = malloc(WIDTH * HEIGHT * 3);
	if (!layer_buf)
		return;
	scratch_buf = malloc(WIDTH * HEIGHT * 3);
	if (!scratch_buf)
	{
		free(layer_buf);
		return;
	}
	layer_alpha = malloc(WIDTH * HEIGHT);
	if (!layer_alpha)
	{
		free(scratch_buf);
		free(layer_buf);
		return;
	}
	scratch_alpha = malloc(WIDTH * HEIGHT);
	if (!scratch_alpha)
	{
		free(layer_alpha);
		free(scratch_buf);
		free(layer_buf);
		return;
	}
	
	for (int layer_i = 0; layer_i < scene->n_layers; layer_i++)
	{
		wb_scene_layer *layer = &scene->layers[layer_i];
		clear_layer_buffer(layer_buf);
		clear_alpha_buffer(layer_alpha);
		set_draw_alpha_buffer(layer_alpha);
		
		for (int i = 0; i < scene->n_objects; i++)
		{
			if (scene->objects[i].layer_id == layer->id)
				draw_scene_object(&scene->objects[i], layer, frame, layer_buf);
		}
		
		set_draw_alpha_buffer(NULL);
		if (layer->blur_radius > 0.0f)
		{
			blur_layer_buffer(layer_buf, scratch_buf, layer->blur_radius);
			blur_alpha_buffer(layer_alpha, scratch_alpha, layer->blur_radius);
		}
		composite_layer_buffer(buf, layer_buf, layer_alpha, layer->opacity);
	}
	
	free(scratch_alpha);
	free(layer_alpha);
	free(scratch_buf);
	free(layer_buf);
}
