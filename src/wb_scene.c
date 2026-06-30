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

static void draw_scene_object(wb_scene_object *obj, wb_vec2 layer_offset, int frame, uint8_t *buf)
{
	if (!obj || obj->draw_progress <= 0.0f)
		return;
	
	if (obj->type == WB_OBJECT_MATH)
		wb_math_draw_seeded(obj->math ? buf : NULL, obj->math, obj->x + layer_offset.x, obj->y + layer_offset.y, obj->size, obj->colour, frame + obj->id * 1009);
	else if (obj->type == WB_OBJECT_LINE)
		draw_sausage(buf, vec2(obj->x + obj->p0.x + layer_offset.x, obj->y + obj->p0.y + layer_offset.y), vec2(obj->x + obj->p1.x + layer_offset.x, obj->y + obj->p1.y + layer_offset.y), obj->thickness, obj->colour);
	else if (obj->type == WB_OBJECT_POINT)
		draw_disc(buf, obj->x + layer_offset.x, obj->y + layer_offset.y, obj->radius, obj->colour);
	else if (obj->type == WB_OBJECT_OPEN_POINT)
	{
		draw_disc(buf, obj->x + layer_offset.x, obj->y + layer_offset.y, obj->radius, obj->colour);
		draw_disc(buf, obj->x + layer_offset.x, obj->y + layer_offset.y, binary_max(0.0f, obj->radius - obj->thickness), 0xFFFFFF);
	}
}

static void clear_layer_buffer(uint8_t *buf)
{
	if (!buf)
		return;
	memset(buf, 0, WIDTH * HEIGHT * 3);
}

static void composite_layer_buffer(uint8_t *dst, uint8_t *src, float opacity)
{
	if (!dst || !src || opacity <= 0.0f)
		return;
	
	if (opacity > 1.0f)
		opacity = 1.0f;
	
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		int ind = i * 3;
		int b = src[ind + 0];
		int g = src[ind + 1];
		int r = src[ind + 2];
		
		if (r == 0 && g == 0 && b == 0)
			continue;
		
		float inv = 1.0f - opacity;
		dst[ind + 0] = (uint8_t)(dst[ind + 0] * inv + b * opacity);
		dst[ind + 1] = (uint8_t)(dst[ind + 1] * inv + g * opacity);
		dst[ind + 2] = (uint8_t)(dst[ind + 2] * inv + r * opacity);
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
	
	for (int layer_i = 0; layer_i < scene->n_layers; layer_i++)
	{
		wb_scene_layer *layer = &scene->layers[layer_i];
		clear_layer_buffer(layer_buf);
		
		for (int i = 0; i < scene->n_objects; i++)
		{
			if (scene->objects[i].layer_id == layer->id)
				draw_scene_object(&scene->objects[i], layer->render_offset, frame, layer_buf);
		}
		
		if (layer->blur_radius > 0.0f)
			blur_layer_buffer(layer_buf, scratch_buf, layer->blur_radius);
		composite_layer_buffer(buf, layer_buf, layer->opacity);
	}
	
	free(scratch_buf);
	free(layer_buf);
}
