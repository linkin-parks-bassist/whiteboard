#include "whiteboard.h"

static int ensure_render_buffers(wb_scene *scene);
static void init_render_context(wb_scene *scene);
static wb_render_context *find_layer(wb_scene *scene, int layer_id);

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
	result->background_center_colour = WB_DEFAULT_BACKGROUND_CENTER_COLOUR;
	result->background_edge_colour = WB_DEFAULT_BACKGROUND_EDGE_COLOUR;
	result->root_viewport.center = vec2(0, 0);
	result->root_viewport.half_height = 1.0f;
	init_camera(&result->camera);
	init_render_context(result);
	result->root_patch_id = wb_scene_add_patch(result, "root", 0, WB_LAYER_2D, 0);
	result->current_patch_id = result->root_patch_id;
	
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
		free(scene->objects[i].text);
		free(scene->objects[i].points3d);
	}
	
	free(scene->patches);
	free(scene->objects);
	free(scene->actions);
	free(scene->render_scratch_alpha);
	free(scene->render_layer_alpha);
	free(scene->render_glow_alpha);
	free(scene->render_scratch_buf);
	free(scene->render_layer_buf);
	free(scene->render_glow_buf);
	free(scene);
}

static int ensure_render_buffers(wb_scene *scene)
{
	uint8_t *layer_buf;
	uint8_t *scratch_buf;
	uint8_t *glow_buf;
	uint8_t *layer_alpha;
	uint8_t *scratch_alpha;
	uint8_t *glow_alpha;
	size_t colour_bytes;
	size_t alpha_bytes;
	
	if (!scene)
		return 0;
	
	if (scene->render_layer_buf && scene->render_scratch_buf && scene->render_glow_buf &&
		scene->render_layer_alpha && scene->render_scratch_alpha && scene->render_glow_alpha)
		return 1;
	
	colour_bytes = (size_t)WIDTH * (size_t)HEIGHT * 3u;
	alpha_bytes = (size_t)WIDTH * (size_t)HEIGHT;
	layer_buf = malloc(colour_bytes);
	if (!layer_buf)
		return 0;
	scratch_buf = malloc(colour_bytes);
	if (!scratch_buf)
	{
		free(layer_buf);
		return 0;
	}
	glow_buf = malloc(colour_bytes);
	if (!glow_buf)
	{
		free(scratch_buf);
		free(layer_buf);
		return 0;
	}
	layer_alpha = malloc(alpha_bytes);
	if (!layer_alpha)
	{
		free(glow_buf);
		free(scratch_buf);
		free(layer_buf);
		return 0;
	}
	scratch_alpha = malloc(alpha_bytes);
	if (!scratch_alpha)
	{
		free(layer_alpha);
		free(glow_buf);
		free(scratch_buf);
		free(layer_buf);
		return 0;
	}
	glow_alpha = malloc(alpha_bytes);
	if (!glow_alpha)
	{
		free(scratch_alpha);
		free(layer_alpha);
		free(glow_buf);
		free(scratch_buf);
		free(layer_buf);
		return 0;
	}
	
	scene->render_layer_buf = layer_buf;
	scene->render_scratch_buf = scratch_buf;
	scene->render_glow_buf = glow_buf;
	scene->render_layer_alpha = layer_alpha;
	scene->render_scratch_alpha = scratch_alpha;
	scene->render_glow_alpha = glow_alpha;
	return 1;
}

void wb_scene_set_radial_background(wb_scene *scene, uint32_t center_colour, uint32_t edge_colour)
{
	if (!scene)
		return;
	
	scene->background_type = WB_BACKGROUND_RADIAL;
	scene->background_center_colour = center_colour;
	scene->background_edge_colour = edge_colour;
}

void wb_scene_set_paper_background(wb_scene *scene, uint32_t center_colour, uint32_t edge_colour)
{
	if (!scene)
		return;
	
	scene->background_type = WB_BACKGROUND_PAPER;
	scene->background_center_colour = center_colour;
	scene->background_edge_colour = edge_colour;
}

void wb_scene_set_root_viewport(wb_scene *scene, float center_x, float center_y, float half_height)
{
	if (!scene || half_height <= 0.0f)
		return;
	scene->root_viewport.center = vec2(center_x, center_y);
	scene->root_viewport.half_height = half_height;
}

static float hash01_2d(int x, int y, int seed)
{
	uint32_t n = (uint32_t)(x * 374761393u) ^ (uint32_t)(y * 668265263u) ^ (uint32_t)(seed * 2246822519u);
	n = (n ^ (n >> 13)) * 1274126177u;
	n ^= n >> 16;
	return (float)(n & 0x00FFFFFFu) / 16777215.0f;
}

static void apply_paper_texture(uint8_t *buf)
{
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			int ind = (y * WIDTH + x) * 3;
			float grain = hash01_2d(x / 2, y / 2, 17) - 0.5f;
			float fibre = hash01_2d(x / 18, y * 3, 29) - 0.5f;
			float wash = hash01_2d(x / 80, y / 80, 43) - 0.5f;
			float delta = grain * 6.0f + fibre * 4.0f + wash * 8.0f;
			for (int c = 0; c < 3; c++)
			{
				int value = (int)buf[ind + c] + (int)roundf(delta);
				if (value < 0)
					value = 0;
				if (value > 255)
					value = 255;
				buf[ind + c] = (uint8_t)value;
			}
		}
	}
}

static void init_render_context(wb_scene *scene)
{
	wb_render_context *layer;
	
	if (!scene)
		return;
	
	layer = &scene->render_context;
	memset(layer, 0, sizeof(*layer));
	layer->id = 1;
	snprintf(layer->name, sizeof(layer->name), "%s", "render");
	layer->type = WB_LAYER_2D;
	layer->opacity = WB_DEFAULT_LAYER_OPACITY;
	layer->render_opacity = layer->opacity;
	layer->blur_radius = WB_DEFAULT_LAYER_BLUR_RADIUS;
	layer->glow_radius = WB_DEFAULT_LAYER_GLOW_RADIUS;
	layer->glow_opacity = WB_DEFAULT_LAYER_GLOW_OPACITY;
	layer->jitter_strength = WB_DEFAULT_LAYER_JITTER_STRENGTH;
	layer->jitter_explicit = 0;
	layer->render_jitter_strength = layer->jitter_strength;
	layer->camera_distance = WB_DEFAULT_LAYER_CAMERA_DISTANCE;
	layer->camera_scale = WB_DEFAULT_LAYER_CAMERA_SCALE;
	layer->camera_yaw = WB_DEFAULT_LAYER_CAMERA_YAW;
	layer->camera_projection = WB_DEFAULT_LAYER_CAMERA_PROJECTION;
	layer->camera_center = vec2(WB_DEFAULT_LAYER_CAMERA_CENTER_X, WB_DEFAULT_LAYER_CAMERA_CENTER_Y);
	layer->camera_target_explicit = 0;
	layer->camera_target = vec3(0, 0, 0);
	layer->render_camera_distance = layer->camera_distance;
	layer->render_camera_scale = layer->camera_scale;
	layer->render_camera_yaw = layer->camera_yaw;
	layer->render_camera_projection = layer->camera_projection;
	layer->render_camera_center = layer->camera_center;
	layer->render_camera_target_explicit = layer->camera_target_explicit;
	layer->render_camera_target = layer->camera_target;
	layer->offset = vec2(WB_DEFAULT_LAYER_OFFSET_X, WB_DEFAULT_LAYER_OFFSET_Y);
	layer->render_offset = layer->offset;
	
}

void wb_scene_set_layer_blur(wb_scene *scene, int layer_id, float blur_radius)
{
	wb_render_context *layer = find_layer(scene, layer_id);
	
	if (!layer)
		return;
	
	if (blur_radius < WB_MIN_OPACITY)
		blur_radius = WB_MIN_OPACITY;
	if (blur_radius > WB_MAX_LAYER_BLUR_RADIUS)
		blur_radius = WB_MAX_LAYER_BLUR_RADIUS;
	
	layer->blur_radius = blur_radius;
}

void wb_scene_set_layer_glow(wb_scene *scene, int layer_id, float glow_radius, float glow_opacity)
{
	wb_render_context *layer = find_layer(scene, layer_id);
	
	if (!layer)
		return;
	
	if (glow_radius < WB_MIN_OPACITY)
		glow_radius = WB_MIN_OPACITY;
	if (glow_radius > WB_MAX_LAYER_BLUR_RADIUS)
		glow_radius = WB_MAX_LAYER_BLUR_RADIUS;
	if (glow_opacity < WB_MIN_OPACITY)
		glow_opacity = WB_MIN_OPACITY;
	if (glow_opacity > WB_MAX_OPACITY)
		glow_opacity = WB_MAX_OPACITY;
	
	layer->glow_radius = glow_radius;
	layer->glow_opacity = glow_opacity;
}

void wb_scene_set_layer_jitter(wb_scene *scene, int layer_id, float jitter_strength)
{
	wb_render_context *layer = find_layer(scene, layer_id);
	
	if (!layer)
		return;
	
	if (jitter_strength < WB_MIN_JITTER_STRENGTH)
		jitter_strength = WB_MIN_JITTER_STRENGTH;
	layer->jitter_strength = jitter_strength;
	layer->jitter_explicit = 1;
	layer->render_jitter_strength = jitter_strength;
}

void wb_scene_set_layer_camera(wb_scene *scene, int layer_id, float distance, float scale, float yaw, int projection, float center_x, float center_y)
{
	wb_render_context *layer = find_layer(scene, layer_id);
	
	if (!layer)
		return;
	
	if (distance < WB_MIN_LAYER_CAMERA_DISTANCE)
		distance = WB_MIN_LAYER_CAMERA_DISTANCE;
	if (scale < WB_MIN_LAYER_CAMERA_SCALE)
		scale = WB_MIN_LAYER_CAMERA_SCALE;
	
	layer->camera_distance = distance;
	layer->camera_scale = scale;
	layer->camera_yaw = yaw;
	layer->camera_projection = projection == WB_CAMERA_PROJECTION_ORTHOGRAPHIC ? WB_CAMERA_PROJECTION_ORTHOGRAPHIC : WB_CAMERA_PROJECTION_PERSPECTIVE;
	layer->camera_center = vec2(center_x, center_y);
	layer->render_camera_distance = layer->camera_distance;
	layer->render_camera_scale = layer->camera_scale;
	layer->render_camera_yaw = layer->camera_yaw;
	layer->render_camera_projection = layer->camera_projection;
	layer->render_camera_center = layer->camera_center;
}

void wb_scene_set_layer_camera_target(wb_scene *scene, int layer_id, int explicit_target, float x, float y, float z)
{
	wb_render_context *layer = find_layer(scene, layer_id);
	
	if (!layer)
		return;
	
	layer->camera_target_explicit = explicit_target ? 1 : 0;
	layer->camera_target = vec3(x, y, z);
	layer->render_camera_target_explicit = layer->camera_target_explicit;
	layer->render_camera_target = layer->camera_target;
}

void wb_scene_set_current_layer(wb_scene *scene, int layer_id)
{
	if (!scene)
		return;
	
	(void)layer_id;
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

static wb_render_context *find_layer(wb_scene *scene, int layer_id)
{
	if (!scene)
		return NULL;
	
	return scene->render_context.id == layer_id ? &scene->render_context : NULL;
}

wb_scene_patch *wb_scene_find_patch(wb_scene *scene, int patch_id)
{
	if (!scene || patch_id <= 0)
		return NULL;
	for (int i = 0; i < scene->n_patches; i++)
	{
		if (scene->patches[i].id == patch_id)
			return &scene->patches[i];
	}
	return NULL;
}

int wb_scene_add_patch(wb_scene *scene, const char *name, int parent_id, int dimension, int coord_type)
{
	wb_scene_patch *patch;

	if (!scene)
		return 0;
	if (parent_id > 0 && !wb_scene_find_patch(scene, parent_id))
		return 0;
	if (scene->n_patches >= scene->cap_patches)
	{
		int new_cap = scene->cap_patches ? scene->cap_patches * 2 : 8;
		wb_scene_patch *patches = realloc(scene->patches, sizeof(wb_scene_patch) * new_cap);
		if (!patches)
			return 0;
		scene->patches = patches;
		scene->cap_patches = new_cap;
	}

	patch = &scene->patches[scene->n_patches++];
	memset(patch, 0, sizeof(*patch));
	patch->id = ++scene->next_patch_id;
	patch->parent_id = parent_id;
	patch->draw_order = ++scene->next_draw_order;
	snprintf(patch->name, sizeof(patch->name), "%s", (name && *name) ? name : "patch");
	patch->dimension = dimension == WB_LAYER_3D ? WB_LAYER_3D : WB_LAYER_2D;
	patch->coord_type = coord_type;
	patch->scale = vec2(1, 1);
	patch->scale3 = vec3(1, 1, 1);
	patch->opacity = 1.0f;
	patch->render_opacity = patch->opacity;
	patch->glow_opacity = WB_DEFAULT_LAYER_GLOW_OPACITY;
	patch->jitter_strength = WB_DEFAULT_LAYER_JITTER_STRENGTH;
	patch->render_jitter_strength = patch->jitter_strength;
	patch->render_translation = vec2(0, 0);
	patch->render_translation3d = vec3(0, 0, 0);
	patch->camera_distance = WB_DEFAULT_LAYER_CAMERA_DISTANCE;
	patch->camera_scale = WB_DEFAULT_LAYER_CAMERA_SCALE;
	patch->camera_yaw = WB_DEFAULT_LAYER_CAMERA_YAW;
	patch->camera_projection = WB_DEFAULT_LAYER_CAMERA_PROJECTION;
	patch->camera_center = vec2(WB_DEFAULT_LAYER_CAMERA_CENTER_X, WB_DEFAULT_LAYER_CAMERA_CENTER_Y);
	patch->camera_target = vec3(0, 0, 0);
	patch->render_camera_distance = patch->camera_distance;
	patch->render_camera_scale = patch->camera_scale;
	patch->render_camera_yaw = patch->camera_yaw;
	patch->render_camera_projection = patch->camera_projection;
	patch->render_camera_center = patch->camera_center;
	patch->render_camera_target = patch->camera_target;
	return patch->id;
}

void wb_scene_set_current_patch(wb_scene *scene, int patch_id)
{
	if (scene && wb_scene_find_patch(scene, patch_id))
		scene->current_patch_id = patch_id;
}

void wb_scene_set_patch_transform(wb_scene *scene, int patch_id, wb_vec2 origin, wb_vec2 scale, float rotation, wb_vec3 origin3, wb_vec3 scale3, wb_vec3 rotation3)
{
	wb_scene_patch *patch = wb_scene_find_patch(scene, patch_id);
	if (!patch)
		return;
	patch->origin = origin;
	patch->scale = scale;
	patch->rotation = rotation;
	patch->origin3 = origin3;
	patch->scale3 = scale3;
	patch->rotation3 = rotation3;
}

void wb_scene_set_object_jitter(wb_scene *scene, int object_id, float jitter_strength)
{
	wb_scene_object *obj = find_object(scene, object_id);
	
	if (!obj)
		return;
	
	if (jitter_strength < WB_MIN_JITTER_STRENGTH)
		jitter_strength = WB_MIN_JITTER_STRENGTH;
	obj->jitter_strength = jitter_strength;
	obj->jitter_explicit = 1;
	obj->render_jitter_strength = jitter_strength;
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
	obj->math = wb_math_parse(src);
	obj->x = x;
	obj->y = y;
	obj->size = size;
	obj->colour = colour;
	obj->thickness = WB_DEFAULT_MATH_THICKNESS;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	obj->render_translation = vec2(0, 0);
	obj->render_patch_pivot = vec2(0, 0);
	obj->render_patch_scale = vec2(1, 1);
	obj->render_patch_rotation = 0.0f;

	
	if (!obj->math)
		return 0;
	
	return obj->id;
}

int wb_scene_add_text(wb_scene *scene, const char *src, float x, float y, float size, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_TEXT;
	obj->text = src ? strdup(src) : NULL;
	obj->x = x;
	obj->y = y;
	obj->size = size;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	obj->render_alpha = 1.0f;
	obj->render_translation = vec2(0, 0);
	obj->render_patch_pivot = vec2(0, 0);
	obj->render_patch_scale = vec2(1, 1);
	obj->render_patch_rotation = 0.0f;
	
	if (!obj->text)
		return 0;
	
	return obj->id;
}

int wb_scene_add_curve(wb_scene *scene, float x0, float y0, float x1, float y1, float x2, float y2, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_CURVE;
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->x = x2;
	obj->y = y2;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	obj->render_translation = vec2(0, 0);
	obj->render_patch_pivot = vec2(0, 0);
	obj->render_patch_scale = vec2(1, 1);
	obj->render_patch_rotation = 0.0f;
	
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
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;

	
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
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->thickness = thickness;
	obj->size = gap;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
	return obj->id;
}

int wb_scene_add_dashed_line(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, float gap, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_DASHED_LINE;
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->thickness = thickness;
	obj->size = gap;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->thickness = thickness;
	obj->size = head_size;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->x = x2;
	obj->y = y2;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->x = x2;
	obj->y = y2;
	obj->size = opacity;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	obj->render_alpha = 1.0f;
	
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
	obj->p0 = vec2(x0, y0);
	obj->p1 = vec2(x1, y1);
	obj->q0 = vec3(x2, y2, 0);
	obj->q1 = vec3(x3, y3, 0);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
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

int wb_scene_add_shade_polygon(wb_scene *scene, const wb_vec2 *points, int n_points, uint32_t colour, float opacity)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj || !points || n_points < 3 || n_points > 7)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_SHADE_POLYGON;
	obj->p0 = points[0];
	obj->p1 = points[1];
	obj->q0 = vec3(points[2].x, points[2].y, 0);
	obj->q1 = vec3(points[3].x, points[3].y, 0);
	obj->q2 = vec3(points[4].x, points[4].y, 0);
	obj->x = points[5].x;
	obj->y = points[5].y;
	obj->q2.z = points[6].x;
	obj->size = points[6].y;
	obj->radius = (float)n_points;
	obj->colour = colour;
	obj->thickness = 0.0f;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	obj->render_alpha = 1.0f;
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
	obj->thickness = opacity;
	
	return obj->id;
}

int wb_scene_add_blob(wb_scene *scene, const wb_vec2 *points, int n_points, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj || !points || n_points < 3 || n_points > 7)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_BLOB;
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
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
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

int wb_scene_add_shade_blob(wb_scene *scene, const wb_vec2 *points, int n_points, uint32_t colour, float opacity)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj || !points || n_points < 3 || n_points > 7)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_SHADE_BLOB;
	obj->p0 = points[0];
	obj->p1 = points[1];
	obj->q0 = vec3(points[2].x, points[2].y, 0);
	obj->q1 = vec3(points[3].x, points[3].y, 0);
	obj->q2 = vec3(points[4].x, points[4].y, 0);
	obj->x = points[5].x;
	obj->y = points[5].y;
	obj->q2.z = points[6].x;
	obj->size = points[6].y;
	obj->radius = (float)n_points;
	obj->colour = colour;
	obj->thickness = opacity;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	obj->render_alpha = 1.0f;
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

int wb_scene_add_point3d(wb_scene *scene, float x, float y, float z, float radius, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_POINT3D;
	obj->q0 = vec3(x, y, z);
	obj->radius = radius;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
	return obj->id;
}

int wb_scene_add_open_point3d(wb_scene *scene, float x, float y, float z, float radius, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_OPEN_POINT3D;
	obj->q0 = vec3(x, y, z);
	obj->radius = radius;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
	return obj->id;
}

int wb_scene_add_triangle3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_TRIANGLE3D;
	obj->q0 = vec3(x0, y0, z0);
	obj->q1 = vec3(x1, y1, z1);
	obj->q2 = vec3(x2, y2, z2);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
	return obj->id;
}

int wb_scene_add_shade_triangle3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, uint32_t colour, float opacity)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_SHADE_TRIANGLE3D;
	obj->q0 = vec3(x0, y0, z0);
	obj->q1 = vec3(x1, y1, z1);
	obj->q2 = vec3(x2, y2, z2);
	obj->size = opacity;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	obj->render_alpha = 1.0f;
	
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
	obj->q0 = vec3(x0, y0, z0);
	obj->q1 = vec3(x1, y1, z1);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->q0 = vec3(x0, y0, z0);
	obj->q1 = vec3(x1, y1, z1);
	obj->q2 = vec3(x2, y2, z2);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
	return obj->id;
}

int wb_scene_add_wire3d(wb_scene *scene, const wb_vec3 *points, int n_points, float thickness, uint32_t colour)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj || !points || n_points < 3)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_WIRE3D;
	obj->points3d = malloc(sizeof(wb_vec3) * n_points);
	if (!obj->points3d)
		return 0;
	memcpy(obj->points3d, points, sizeof(wb_vec3) * n_points);
	obj->n_points3d = n_points;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
	return obj->id;
}

int wb_scene_add_shade_poly3d(wb_scene *scene, const wb_vec3 *points, int n_points, uint32_t colour, float opacity)
{
	wb_scene_object *obj = append_object(scene);
	
	if (!obj || !points || n_points < 3)
		return 0;
	
	memset(obj, 0, sizeof(*obj));
	obj->id = scene->next_object_id++;
	obj->type = WB_OBJECT_SHADE_POLY3D;
	obj->points3d = malloc(sizeof(wb_vec3) * n_points);
	if (!obj->points3d)
		return 0;
	memcpy(obj->points3d, points, sizeof(wb_vec3) * n_points);
	obj->n_points3d = n_points;
	obj->colour = colour;
	obj->thickness = opacity;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	obj->render_alpha = 1.0f;
	
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
	obj->x = x;
	obj->y = y;
	obj->radius = radius;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->x = x;
	obj->y = y;
	obj->radius = radius;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->x = x;
	obj->y = y;
	obj->radius = radius;
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->x = x;
	obj->y = y;
	obj->p0 = vec2(radius_x, radius_y);
	obj->thickness = thickness;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	
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
	obj->x = x;
	obj->y = y;
	obj->radius = radius;
	obj->size = opacity;
	obj->colour = colour;
	obj->draw_progress = 1.0f;
	obj->jitter_strength = WB_DEFAULT_OBJECT_JITTER_STRENGTH;
	obj->jitter_explicit = 0;
	obj->render_jitter_strength = obj->jitter_strength;
	obj->render_alpha = 1.0f;
	
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

void wb_scene_translate(wb_scene *scene, int object_id, float start_time, float end_time, float x1, float y1, float x2, float y2)
{
	wb_scene_action *action = append_action(scene);
	
	if (!action)
		return;
	
	action->object_id = object_id;
	action->type = WB_ACTION_TRANSLATE;
	action->start_time = start_time;
	action->end_time = end_time;
	action->from = vec2(x1, y1);
	action->to = vec2(x2, y2);
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_translate3d(wb_scene *scene, int object_id, float start_time, float end_time, float x1, float y1, float z1, float x2, float y2, float z2)
{
	wb_scene_action *action = append_action(scene);
	
	if (!action)
		return;
	
	action->object_id = object_id;
	action->type = WB_ACTION_TRANSLATE3D;
	action->start_time = start_time;
	action->end_time = end_time;
	action->q0 = vec3(x1, y1, z1);
	action->q1 = vec3(x2, y2, z2);
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_transform3d(wb_scene *scene, int object_id, float start_time, float end_time, float pivot_x, float pivot_y, float pivot_z, float scale_x1, float scale_y1, float scale_z1, float yaw1, float pitch1, float roll1, float scale_x2, float scale_y2, float scale_z2, float yaw2, float pitch2, float roll2)
{
	wb_scene_action *action = append_action(scene);
	
	if (!action)
		return;
	
	action->object_id = object_id;
	action->type = WB_ACTION_TRANSFORM3D;
	action->start_time = start_time;
	action->end_time = end_time;
	action->q0 = vec3(pivot_x, pivot_y, pivot_z);
	action->from = vec2(scale_x1, scale_y1);
	action->to = vec2(scale_x2, scale_y2);
	action->from_z = scale_z1;
	action->to_z = scale_z2;
	action->q1 = vec3(yaw1, pitch1, roll1);
	action->q2 = vec3(yaw2, pitch2, roll2);
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_transform(wb_scene *scene, int object_id, float start_time, float end_time, float pivot_x, float pivot_y, float scale_x1, float scale_y1, float rotation1, float scale_x2, float scale_y2, float rotation2)
{
	wb_scene_action *action = append_action(scene);
	
	if (!action)
		return;
	
	action->object_id = object_id;
	action->type = WB_ACTION_TRANSFORM;
	action->start_time = start_time;
	action->end_time = end_time;
	action->q0 = vec3(pivot_x, pivot_y, 0);
	action->from = vec2(scale_x1, scale_y1);
	action->to = vec2(scale_x2, scale_y2);
	action->aux0 = rotation1;
	action->aux1 = rotation2;
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_fade_patch(wb_scene *scene, int patch_id, float start_time, float end_time, float opacity1, float opacity2)
{
	wb_scene_action *action;
	if (!scene || !wb_scene_find_patch(scene, patch_id))
		return;
	action = append_action(scene);
	if (!action)
		return;
	action->patch_id = patch_id;
	action->type = WB_ACTION_PATCH_FADE;
	action->start_time = start_time;
	action->end_time = end_time;
	action->from_z = opacity1;
	action->to_z = opacity2;
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_translate_patch(wb_scene *scene, int patch_id, float start_time, float end_time, wb_vec2 from, wb_vec2 to)
{
	wb_scene_action *action;
	if (!scene || !wb_scene_find_patch(scene, patch_id))
		return;
	action = append_action(scene);
	if (!action)
		return;
	action->patch_id = patch_id;
	action->type = WB_ACTION_PATCH_TRANSLATE;
	action->start_time = start_time;
	action->end_time = end_time;
	action->from = from;
	action->to = to;
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_translate_patch3d(wb_scene *scene, int patch_id, float start_time, float end_time, wb_vec3 from, wb_vec3 to)
{
	wb_scene_action *action;
	if (!scene || !wb_scene_find_patch(scene, patch_id))
		return;
	action = append_action(scene);
	if (!action)
		return;
	action->patch_id = patch_id;
	action->type = WB_ACTION_PATCH_TRANSLATE3D;
	action->start_time = start_time;
	action->end_time = end_time;
	action->q0 = from;
	action->q1 = to;
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_transform_patch(wb_scene *scene, int patch_id, float start_time, float end_time, wb_vec2 pivot, wb_vec2 scale1, float rotation1, wb_vec2 scale2, float rotation2)
{
	wb_scene_action *action;
	if (!scene || !wb_scene_find_patch(scene, patch_id)) return;
	action = append_action(scene);
	if (!action) return;
	action->patch_id = patch_id;
	action->type = WB_ACTION_PATCH_TRANSFORM;
	action->start_time = start_time;
	action->end_time = end_time;
	action->q0 = vec3(pivot.x, pivot.y, 0);
	action->from = scale1;
	action->to = scale2;
	action->aux0 = rotation1;
	action->aux1 = rotation2;
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_transform_patch3d(wb_scene *scene, int patch_id, float start_time, float end_time, wb_vec3 pivot, wb_vec3 scale1, wb_vec3 rotation1, wb_vec3 scale2, wb_vec3 rotation2)
{
	wb_scene_action *action;
	if (!scene || !wb_scene_find_patch(scene, patch_id)) return;
	action = append_action(scene);
	if (!action) return;
	action->patch_id = patch_id;
	action->type = WB_ACTION_PATCH_TRANSFORM3D;
	action->start_time = start_time;
	action->end_time = end_time;
	action->q0 = pivot;
	action->from = vec2(scale1.x, scale1.y);
	action->from_z = scale1.z;
	action->to = vec2(scale2.x, scale2.y);
	action->to_z = scale2.z;
	action->q1 = rotation1;
	action->q2 = rotation2;
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_set_patch_camera(wb_scene *scene, int patch_id, float distance, float scale, float yaw, int projection, wb_vec2 center, int target_explicit, wb_vec3 target)
{
	wb_scene_patch *patch = wb_scene_find_patch(scene, patch_id);
	if (!patch) return;
	patch->camera_distance = binary_max(WB_MIN_LAYER_CAMERA_DISTANCE, distance);
	patch->camera_scale = binary_max(WB_MIN_LAYER_CAMERA_SCALE, scale);
	patch->camera_yaw = yaw;
	patch->camera_projection = projection;
	patch->camera_center = center;
	patch->camera_target_explicit = target_explicit;
	patch->camera_target = target;
	patch->render_camera_distance = patch->camera_distance;
	patch->render_camera_scale = patch->camera_scale;
	patch->render_camera_yaw = patch->camera_yaw;
	patch->render_camera_projection = patch->camera_projection;
	patch->render_camera_center = patch->camera_center;
	patch->render_camera_target_explicit = patch->camera_target_explicit;
	patch->render_camera_target = patch->camera_target;
}

void wb_scene_move_patch_camera(wb_scene *scene, int patch_id, float start_time, float end_time, float distance1, float scale1, float yaw1, wb_vec2 center1, int target1_explicit, wb_vec3 target1, float distance2, float scale2, float yaw2, wb_vec2 center2, int target2_explicit, wb_vec3 target2)
{
	wb_scene_action *action;
	if (!scene || !wb_scene_find_patch(scene, patch_id)) return;
	action = append_action(scene); if (!action) return;
	action->patch_id = patch_id; action->type = WB_ACTION_PATCH_CAMERA_MOVE;
	action->start_time = start_time; action->end_time = end_time;
	action->from = center1; action->to = center2; action->from_z = distance1; action->to_z = distance2;
	action->aux0 = scale1; action->aux1 = scale2; action->aux2 = yaw1; action->aux3 = yaw2;
	action->q0 = target1; action->q1 = target2; action->flags = (target1_explicit ? 1 : 0) | (target2_explicit ? 2 : 0);
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_orbit_patch_camera(wb_scene *scene, int patch_id, float start_time, float end_time, float yaw1, float yaw2)
{
	wb_scene_action *action;
	if (!scene || !wb_scene_find_patch(scene, patch_id)) return;
	action = append_action(scene); if (!action) return;
	action->patch_id = patch_id; action->type = WB_ACTION_PATCH_CAMERA_ORBIT;
	action->start_time = start_time; action->end_time = end_time; action->aux2 = yaw1; action->aux3 = yaw2;
	scene->total_duration = binary_max(scene->total_duration, end_time);
}

void wb_scene_fade_object(wb_scene *scene, int object_id, float start_time, float end_time, float opacity1, float opacity2)
{
	wb_scene_action *action = append_action(scene);
	
	if (!action)
		return;
	
	action->object_id = object_id;
	action->type = WB_ACTION_FADE;
	action->start_time = start_time;
	action->end_time = end_time;
	action->from_z = opacity1;
	action->to_z = opacity2;
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

typedef struct
{
	wb_nurbs_pcurve curve;
	wb_nurbs nx;
	wb_nurbs ny;
	float x_knots[9];
	float y_knots[9];
	float x_control_points[5];
	float y_control_points[5];
	float x_weights[5];
	float y_weights[5];
} wb_stack_line_curve;

typedef struct
{
	wb_nurbs_pcurve curve;
	wb_nurbs nx;
	wb_nurbs ny;
	float x_knots[6];
	float y_knots[6];
	float x_control_points[3];
	float y_control_points[3];
	float x_weights[3];
	float y_weights[3];
} wb_stack_quad_curve;

static void init_stack_nurbs(wb_nurbs *nurbs, int degree, int n_control_points, float *knots, float *control_points, float *weights)
{
	if (!nurbs)
		return;
	
	nurbs->degree = degree;
	nurbs->n_control_points = n_control_points;
	nurbs->n_knots = degree + n_control_points + 1;
	nurbs->knots = knots;
	nurbs->control_points = control_points;
	nurbs->weights = weights;
	for (int i = 0; i < nurbs->n_knots; i++)
		nurbs->knots[i] = (float)i;
	for (int i = 0; i < n_control_points; i++)
		nurbs->weights[i] = 1.0f;
}

static void init_stack_pcurve(wb_nurbs_pcurve *curve, wb_nurbs *nx, wb_nurbs *ny)
{
	if (!curve)
		return;
	
	curve->nx = nx;
	curve->ny = ny;
	curve->colour = 0;
}

static int build_jittered_line_curve(wb_stack_line_curve *storage, wb_vec2 a, wb_vec2 b, float thickness, float jitter_strength, int seed)
{
	wb_vec2 d = vec2_diff(b, a);
	wb_vec2 n = vec2_perp(vec2_normalised(d));
	float d_norm = vec2_norm(d);
	float amp = binary_max(1.0f, thickness * 0.65f) * jitter_strength;
	
	if (!storage)
		return 0;
	
	init_stack_nurbs(&storage->nx, 3, 5, storage->x_knots, storage->x_control_points, storage->x_weights);
	init_stack_nurbs(&storage->ny, 3, 5, storage->y_knots, storage->y_control_points, storage->y_weights);
	init_stack_pcurve(&storage->curve, &storage->nx, &storage->ny);
	
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
		
		storage->nx.control_points[i] = a.x + d.x * t + d.x * along / binary_max(1.0f, d_norm) + n.x * across;
		storage->ny.control_points[i] = a.y + d.y * t + d.y * along / binary_max(1.0f, d_norm) + n.y * across;
	}
	
	return 1;
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
	wb_stack_line_curve curve_storage;
	wb_vec2 partial_b;
	
	progress = clamp01(progress);
	partial_b = vec2(a.x + (b.x - a.x) * progress, a.y + (b.y - a.y) * progress);
	
	if (!build_jittered_line_curve(&curve_storage, a, b, thickness, jitter_strength, seed))
	{
		draw_sausage(buf, a, partial_b, thickness, colour);
		return;
	}
	
	draw_curve_stroke_progress(buf, &curve_storage.curve, thickness, colour, progress);
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

static void draw_hand_dashed_line(uint8_t *buf, wb_vec2 a, wb_vec2 b, float thickness, float gap, uint32_t colour, float jitter_strength, int seed, float progress)
{
	wb_vec2 d = vec2_diff(b, a);
	float len = vec2_norm(d);
	wb_vec2 dir;
	float dash_len;
	float cycle;
	int n_dashes;
	
	if (!buf || len <= 0.0f || progress <= 0.0f)
		return;
	
	if (gap < thickness * 2.5f)
		gap = thickness * 2.5f;
	dash_len = gap * 1.4f;
	cycle = dash_len + gap;
	n_dashes = 1 + (int)(len / cycle);
	dir = vec2_normalised(d);
	
	for (int i = 0; i < n_dashes; i++)
	{
		float start_dist = (float)i * cycle;
		float end_dist = binary_min(len, start_dist + dash_len);
		float start_t;
		float end_t;
		float segment_progress;
		wb_vec2 s;
		wb_vec2 e;
		
		if (start_dist / len > progress)
			break;
		
		start_t = start_dist / len;
		end_t = end_dist / len;
		segment_progress = (progress - start_t) / binary_max(0.0001f, end_t - start_t);
		segment_progress = clamp01(segment_progress);
		s = vec2(a.x + dir.x * start_dist, a.y + dir.y * start_dist);
		e = vec2(a.x + dir.x * end_dist, a.y + dir.y * end_dist);
		draw_hand_line(buf, s, e, thickness, colour, jitter_strength, seed + i * 149, segment_progress);
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

static int unpack_object_points(const wb_scene_object *obj, wb_vec2 *points, int cap)
{
	int n_points;
	
	if (!obj || !points || cap < 7)
		return 0;
	n_points = (int)(obj->radius + 0.5f);
	if (n_points < 3 || n_points > 7)
		return 0;
	
	points[0] = obj->p0;
	points[1] = obj->p1;
	points[2] = vec2(obj->q0.x, obj->q0.y);
	points[3] = vec2(obj->q1.x, obj->q1.y);
	points[4] = vec2(obj->q2.x, obj->q2.y);
	points[5] = vec2(obj->x, obj->y);
	points[6] = vec2(obj->q2.z, obj->size);
	return n_points;
}

static int build_blob_outline_points(const wb_scene_object *obj, wb_vec2 layer_offset, wb_vec2 *out_points, int cap)
{
	wb_vec2 controls[7];
	int n_controls;
	int out_n = 0;
	
	if (!obj || !out_points || cap < 24)
		return 0;
	n_controls = unpack_object_points(obj, controls, 7);
	if (n_controls < 3)
		return 0;
	
	for (int i = 0; i < n_controls; i++)
	{
		wb_vec2 prev = controls[(i - 1 + n_controls) % n_controls];
		wb_vec2 curr = controls[i];
		wb_vec2 next = controls[(i + 1) % n_controls];
		wb_vec2 start = vec2((prev.x + curr.x) * 0.5f, (prev.y + curr.y) * 0.5f);
		wb_vec2 end = vec2((curr.x + next.x) * 0.5f, (curr.y + next.y) * 0.5f);
		int samples = 8;
		
		for (int s = 0; s < samples; s++)
		{
			float t = (float)s / (float)samples;
			float u = 1.0f - t;
			if (out_n >= cap)
				return out_n;
			out_points[out_n++] = vec2(
				(start.x * u * u + 2.0f * curr.x * u * t + end.x * t * t) + layer_offset.x,
				(start.y * u * u + 2.0f * curr.y * u * t + end.y * t * t) + layer_offset.y);
		}
	}
	
	return out_n;
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

static int project_3d_point(wb_vec3 p, wb_render_context *layer, wb_vec2 *out)
{
	float camera_distance = layer ? layer->render_camera_distance : WB_DEFAULT_LAYER_CAMERA_DISTANCE;
	float scale = layer ? layer->render_camera_scale : WB_DEFAULT_LAYER_CAMERA_SCALE;
	float yaw = layer ? layer->render_camera_yaw : WB_DEFAULT_LAYER_CAMERA_YAW;
	int projection = layer ? layer->render_camera_projection : WB_DEFAULT_LAYER_CAMERA_PROJECTION;
	wb_vec2 center = layer ? layer->render_camera_center : vec2(WB_DEFAULT_LAYER_CAMERA_CENTER_X, WB_DEFAULT_LAYER_CAMERA_CENTER_Y);
	wb_vec3 target = layer ? layer->render_camera_target : vec3(0, 0, 0);
	int target_explicit = layer ? layer->render_camera_target_explicit : 0;
	float c = cosf(yaw);
	float s = sinf(yaw);
	if (target_explicit)
		p = vec3_diff(p, target);
	float rx = c * p.x + s * p.z;
	float rz = -s * p.x + c * p.z;
	float z = rz + camera_distance;
	
	if (!out)
		return 0;
	if (projection == WB_CAMERA_PROJECTION_ORTHOGRAPHIC)
	{
		out->x = center.x + rx * scale;
		out->y = center.y - p.y * scale;
		return 1;
	}
	if (z <= WB_MIN_LAYER_CAMERA_DISTANCE)
		return 0;
	out->x = center.x + (rx / z) * scale;
	out->y = center.y - (p.y / z) * scale;
	return 1;
}

static int build_projected_curve3d(wb_stack_quad_curve *storage, wb_vec3 q0, wb_vec3 q1, wb_vec3 q2, wb_render_context *layer)
{
	wb_vec2 p0;
	wb_vec2 p1;
	wb_vec2 p2;
	
	if (!storage || !project_3d_point(q0, layer, &p0) || !project_3d_point(q1, layer, &p1) || !project_3d_point(q2, layer, &p2))
		return 0;
	
	init_stack_nurbs(&storage->nx, 2, 3, storage->x_knots, storage->x_control_points, storage->x_weights);
	init_stack_nurbs(&storage->ny, 2, 3, storage->y_knots, storage->y_control_points, storage->y_weights);
	init_stack_pcurve(&storage->curve, &storage->nx, &storage->ny);
	
	storage->nx.control_points[0] = p0.x;
	storage->nx.control_points[1] = p1.x;
	storage->nx.control_points[2] = p2.x;
	storage->ny.control_points[0] = p0.y;
	storage->ny.control_points[1] = p1.y;
	storage->ny.control_points[2] = p2.y;
	return 1;
}

static int build_curve2d(wb_stack_quad_curve *storage, wb_vec2 p0, wb_vec2 p1, wb_vec2 p2)
{
	if (!storage)
		return 0;
	
	init_stack_nurbs(&storage->nx, 2, 3, storage->x_knots, storage->x_control_points, storage->x_weights);
	init_stack_nurbs(&storage->ny, 2, 3, storage->y_knots, storage->y_control_points, storage->y_weights);
	init_stack_pcurve(&storage->curve, &storage->nx, &storage->ny);
	
	storage->nx.control_points[0] = p0.x;
	storage->nx.control_points[1] = p1.x;
	storage->nx.control_points[2] = p2.x;
	storage->ny.control_points[0] = p0.y;
	storage->ny.control_points[1] = p1.y;
	storage->ny.control_points[2] = p2.y;
	return 1;
}

static wb_vec2 transform_object_point(wb_vec2 p, wb_vec2 pivot, wb_vec2 scale, float rotation)
{
	float c = cosf(rotation);
	float s = sinf(rotation);
	p = vec2_diff(p, pivot);
	p = vec2(p.x * scale.x, p.y * scale.y);
	p = vec2(c * p.x - s * p.y, s * p.x + c * p.y);
	return vec2_sum(p, pivot);
}

static wb_vec2 transform_object_point_seq(wb_vec2 p, const wb_scene_object *obj)
{
	if (!obj)
		return p;
	for (int i = 0; i < obj->n_render_patch_transforms; i++)
		p = transform_object_point(p, obj->render_patch_transforms[i].pivot, obj->render_patch_transforms[i].scale, obj->render_patch_transforms[i].rotation);
	return p;
}

static float transform_object_radius(float r, wb_vec2 scale)
{
	float sx = fabsf(scale.x);
	float sy = fabsf(scale.y);
	return r * (sx + sy) * 0.5f;
}

static float transform_object_radius_seq(float r, const wb_scene_object *obj)
{
	if (!obj)
		return r;
	for (int i = 0; i < obj->n_render_patch_transforms; i++)
		r = transform_object_radius(r, obj->render_patch_transforms[i].scale);
	return r;
}

static wb_vec3 transform_object_point3d(wb_vec3 p, wb_vec3 pivot, wb_vec3 scale, wb_vec3 rotation)
{
	float cy = cosf(rotation.x);
	float sy = sinf(rotation.x);
	float cx = cosf(rotation.y);
	float sx = sinf(rotation.y);
	float cz = cosf(rotation.z);
	float sz = sinf(rotation.z);
	float x;
	float y;
	float z;

	p = vec3_diff(p, pivot);
	p = vec3(p.x * scale.x, p.y * scale.y, p.z * scale.z);

	x = cy * p.x + sy * p.z;
	z = -sy * p.x + cy * p.z;
	p = vec3(x, p.y, z);

	y = cx * p.y - sx * p.z;
	z = sx * p.y + cx * p.z;
	p = vec3(p.x, y, z);

	x = cz * p.x - sz * p.y;
	y = sz * p.x + cz * p.y;
	p = vec3(x, y, p.z);
	return vec3_sum(p, pivot);
}

static wb_vec3 transform_object_point3d_seq(wb_vec3 p, const wb_scene_object *obj)
{
	if (!obj)
		return p;
	for (int i = 0; i < obj->n_render_patch_transforms3d; i++)
		p = transform_object_point3d(p, obj->render_patch_transforms3d[i].pivot, obj->render_patch_transforms3d[i].scale, obj->render_patch_transforms3d[i].rotation);
	return p;
}

static void draw_scene_object(wb_scene *scene, wb_scene_object *obj, wb_render_context *layer, int frame, uint8_t *buf)
{
	wb_scene_object rendered;
	wb_render_context patch_camera;
	wb_scene_patch *patch;
	wb_vec2 layer_offset;
	wb_vec2 object_offset;
	wb_vec3 object_offset3d;
	float jitter_strength;
	
	if (!obj || obj->draw_progress <= 0.0f)
		return;
	patch = wb_scene_find_patch(scene, obj->patch_id);
	for (wb_scene_patch *ancestor = patch; ancestor; ancestor = wb_scene_find_patch(scene, ancestor->parent_id))
	{
		if (ancestor->dimension == WB_LAYER_3D)
		{
			patch_camera = *layer;
			patch_camera.render_camera_distance = ancestor->render_camera_distance;
			patch_camera.render_camera_scale = ancestor->render_camera_scale;
			patch_camera.render_camera_yaw = ancestor->render_camera_yaw;
			patch_camera.render_camera_projection = ancestor->render_camera_projection;
			patch_camera.render_camera_center = ancestor->render_camera_center;
			patch_camera.render_camera_target_explicit = ancestor->render_camera_target_explicit;
			patch_camera.render_camera_target = ancestor->render_camera_target;
			layer = &patch_camera;
			break;
		}
	}
	rendered = *obj;
	for (wb_scene_patch *ancestor = patch; ancestor; ancestor = wb_scene_find_patch(scene, ancestor->parent_id))
	{
		for (int i = 0; i < ancestor->n_render_transforms && rendered.n_render_patch_transforms < 8; i++)
		{
			int index = rendered.n_render_patch_transforms++;
			rendered.render_patch_transforms[index].pivot = ancestor->render_transforms[i].pivot;
			rendered.render_patch_transforms[index].scale = ancestor->render_transforms[i].scale;
			rendered.render_patch_transforms[index].rotation = ancestor->render_transforms[i].rotation;
		}
		for (int i = 0; i < ancestor->n_render_transforms3d && rendered.n_render_patch_transforms3d < 8; i++)
		{
			int index = rendered.n_render_patch_transforms3d++;
			rendered.render_patch_transforms3d[index].pivot = ancestor->render_transforms3d[i].pivot;
			rendered.render_patch_transforms3d[index].scale = ancestor->render_transforms3d[i].scale;
			rendered.render_patch_transforms3d[index].rotation = ancestor->render_transforms3d[i].rotation;
		}
	}
	obj = &rendered;
	
	layer_offset = layer ? layer->render_offset : vec2(0, 0);
	object_offset = obj->render_translation;
	object_offset3d = obj->render_translation3d;
	for (wb_scene_patch *ancestor = patch; ancestor; ancestor = wb_scene_find_patch(scene, ancestor->parent_id))
	{
		object_offset.x += ancestor->render_translation.x;
		object_offset.y += ancestor->render_translation.y;
		object_offset3d.x += ancestor->render_translation3d.x;
		object_offset3d.y += ancestor->render_translation3d.y;
		object_offset3d.z += ancestor->render_translation3d.z;
	}
	if (obj->jitter_explicit)
		jitter_strength = obj->render_jitter_strength;
	else if (patch && patch->jitter_explicit)
		jitter_strength = patch->render_jitter_strength;
	else if (layer && layer->jitter_explicit)
		jitter_strength = layer->render_jitter_strength;
	else
		jitter_strength = binary_max(obj->render_jitter_strength, (layer ? layer->render_jitter_strength : WB_DEFAULT_LAYER_JITTER_STRENGTH));
	
	if (obj->type == WB_OBJECT_MATH)
	{
		wb_vec2 anchor = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		float size = transform_object_radius_seq(obj->size, obj);
		float stroke_scale = transform_object_radius_seq(obj->thickness, obj);
		wb_set_math_jitter_strength(jitter_strength);
		wb_set_symbol_stroke_scale(stroke_scale);
		wb_math_draw_seeded(obj->math ? buf : NULL, obj->math, anchor.x, anchor.y, size, obj->colour, frame + obj->id * 1009);
		wb_set_symbol_stroke_scale(1.0f);
		wb_set_math_jitter_strength(1.0f);
	}
	else if (obj->type == WB_OBJECT_TEXT)
	{
		wb_vec2 anchor = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		float size = transform_object_radius_seq(obj->size, obj);
		draw_string_with_alpha(buf, obj->text, (int)roundf(anchor.x), (int)roundf(anchor.y), (int)roundf(size), obj->colour, jitter_strength, obj->render_alpha, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_CURVE)
	{
		wb_stack_quad_curve curve_storage;
		wb_vec2 a = transform_object_point_seq(vec2(obj->p0.x + layer_offset.x + object_offset.x, obj->p0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 b = transform_object_point_seq(vec2(obj->p1.x + layer_offset.x + object_offset.x, obj->p1.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 c = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		
		if (!build_curve2d(&curve_storage, a, b, c))
			return;
		
		if (jitter_strength > 0.0f)
			jitter_nurbs_pcurve(&curve_storage.curve, binary_max(0.6f, obj->thickness * 0.45f) * jitter_strength);
		draw_curve_stroke_progress(buf, &curve_storage.curve, transform_object_radius_seq(obj->thickness, obj), obj->colour, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_LINE)
	{
		wb_vec2 a = transform_object_point_seq(vec2(obj->x + obj->p0.x + layer_offset.x + object_offset.x, obj->y + obj->p0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 b = transform_object_point_seq(vec2(obj->x + obj->p1.x + layer_offset.x + object_offset.x, obj->y + obj->p1.y + layer_offset.y + object_offset.y), obj);
		draw_hand_line(buf, a, b, transform_object_radius_seq(obj->thickness, obj), obj->colour, jitter_strength, frame + obj->id * 4099, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_RAY)
	{
		wb_vec2 a = transform_object_point_seq(vec2(obj->x + obj->p0.x + layer_offset.x + object_offset.x, obj->y + obj->p0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 through = transform_object_point_seq(vec2(obj->x + obj->p1.x + layer_offset.x + object_offset.x, obj->y + obj->p1.y + layer_offset.y + object_offset.y), obj);
		draw_hand_line(buf, a, extend_ray_endpoint(a, through), transform_object_radius_seq(obj->thickness, obj), obj->colour, jitter_strength, frame + obj->id * 4421, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_DOTTED_LINE)
	{
		wb_vec2 a = transform_object_point_seq(vec2(obj->x + obj->p0.x + layer_offset.x + object_offset.x, obj->y + obj->p0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 b = transform_object_point_seq(vec2(obj->x + obj->p1.x + layer_offset.x + object_offset.x, obj->y + obj->p1.y + layer_offset.y + object_offset.y), obj);
		draw_hand_dotted_line(buf, a, b, transform_object_radius_seq(obj->thickness, obj), transform_object_radius_seq(obj->size, obj), obj->colour, jitter_strength, frame + obj->id * 5003, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_DASHED_LINE)
	{
		wb_vec2 a = transform_object_point_seq(vec2(obj->x + obj->p0.x + layer_offset.x + object_offset.x, obj->y + obj->p0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 b = transform_object_point_seq(vec2(obj->x + obj->p1.x + layer_offset.x + object_offset.x, obj->y + obj->p1.y + layer_offset.y + object_offset.y), obj);
		draw_hand_dashed_line(buf, a, b, transform_object_radius_seq(obj->thickness, obj), transform_object_radius_seq(obj->size, obj), obj->colour, jitter_strength, frame + obj->id * 5333, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_ARROW)
	{
		wb_vec2 a = transform_object_point_seq(vec2(obj->x + obj->p0.x + layer_offset.x + object_offset.x, obj->y + obj->p0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 b = transform_object_point_seq(vec2(obj->x + obj->p1.x + layer_offset.x + object_offset.x, obj->y + obj->p1.y + layer_offset.y + object_offset.y), obj);
		draw_hand_arrow(buf, a, b, transform_object_radius_seq(obj->thickness, obj), transform_object_radius_seq(obj->size, obj), obj->colour, jitter_strength, frame + obj->id * 6947, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_TRIANGLE)
	{
		wb_vec2 a = transform_object_point_seq(vec2(obj->p0.x + layer_offset.x + object_offset.x, obj->p0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 b = transform_object_point_seq(vec2(obj->p1.x + layer_offset.x + object_offset.x, obj->p1.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 c = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		draw_hand_triangle(buf, a, b, c, transform_object_radius_seq(obj->thickness, obj), obj->colour, jitter_strength, frame + obj->id * 7103, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_SHADE_TRIANGLE)
	{
		wb_vec2 a = transform_object_point_seq(vec2(obj->p0.x + layer_offset.x + object_offset.x, obj->p0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 b = transform_object_point_seq(vec2(obj->p1.x + layer_offset.x + object_offset.x, obj->p1.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 c = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		draw_triangle_with_alpha(buf, a, b, c, obj->colour, obj->size * obj->draw_progress * obj->render_alpha);
	}
	else if (obj->type == WB_OBJECT_QUAD)
	{
		wb_vec2 a = transform_object_point_seq(vec2(obj->p0.x + layer_offset.x + object_offset.x, obj->p0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 b = transform_object_point_seq(vec2(obj->p1.x + layer_offset.x + object_offset.x, obj->p1.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 c = transform_object_point_seq(vec2(obj->q0.x + layer_offset.x + object_offset.x, obj->q0.y + layer_offset.y + object_offset.y), obj);
		wb_vec2 d = transform_object_point_seq(vec2(obj->q1.x + layer_offset.x + object_offset.x, obj->q1.y + layer_offset.y + object_offset.y), obj);
		draw_hand_quad(buf, a, b, c, d, transform_object_radius_seq(obj->thickness, obj), obj->colour, jitter_strength, frame + obj->id * 7349, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_POLYGON)
	{
		wb_vec2 points[7];
		int n_points = unpack_object_points(obj, points, 7);
		
		for (int i = 0; i < n_points; i++)
			points[i] = transform_object_point_seq(vec2(points[i].x + layer_offset.x + object_offset.x, points[i].y + layer_offset.y + object_offset.y), obj);
		draw_hand_polygon(buf, points, n_points, transform_object_radius_seq(obj->thickness, obj), obj->colour, jitter_strength, frame + obj->id * 7523, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_SHADE_POLYGON)
	{
		wb_vec2 points[7];
		int n_points = unpack_object_points(obj, points, 7);
		
		for (int i = 0; i < n_points; i++)
			points[i] = transform_object_point_seq(vec2(points[i].x + layer_offset.x + object_offset.x, points[i].y + layer_offset.y + object_offset.y), obj);
		draw_polygon_with_alpha(buf, points, n_points, obj->colour, obj->thickness * obj->draw_progress * obj->render_alpha);
	}
	else if (obj->type == WB_OBJECT_BLOB)
	{
		wb_vec2 points[64];
		int n_points = build_blob_outline_points(obj, vec2(layer_offset.x + object_offset.x, layer_offset.y + object_offset.y), points, 64);
		for (int i = 0; i < n_points; i++)
			points[i] = transform_object_point_seq(points[i], obj);
		if (n_points >= 3)
			draw_hand_polygon(buf, points, n_points, transform_object_radius_seq(obj->thickness, obj), obj->colour, jitter_strength, frame + obj->id * 7681, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_SHADE_BLOB)
	{
		wb_vec2 points[64];
		int n_points = build_blob_outline_points(obj, vec2(layer_offset.x + object_offset.x, layer_offset.y + object_offset.y), points, 64);
		for (int i = 0; i < n_points; i++)
			points[i] = transform_object_point_seq(points[i], obj);
		if (n_points >= 3)
			draw_polygon_with_alpha(buf, points, n_points, obj->colour, obj->thickness * obj->draw_progress * obj->render_alpha);
	}
	else if (obj->type == WB_OBJECT_SHADE_DISC)
	{
		wb_vec2 center = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		draw_disc_with_alpha(buf, center.x, center.y, transform_object_radius_seq(obj->radius, obj), obj->colour, obj->size * obj->draw_progress * obj->render_alpha);
	}
	else if (obj->type == WB_OBJECT_POINT)
	{
		wb_vec2 center = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		draw_disc(buf, center.x, center.y, transform_object_radius_seq(obj->radius, obj), obj->colour);
	}
	else if (obj->type == WB_OBJECT_OPEN_POINT)
	{
		wb_vec2 center = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		draw_hand_open_point(buf, center.x, center.y, transform_object_radius_seq(obj->radius, obj), transform_object_radius_seq(obj->thickness, obj), obj->colour, jitter_strength, frame + obj->id * 6151, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_CIRCLE)
	{
		wb_vec2 center = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		draw_hand_open_point(buf, center.x, center.y, transform_object_radius_seq(obj->radius, obj), transform_object_radius_seq(obj->thickness, obj), obj->colour, jitter_strength, frame + obj->id * 6151, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_ELLIPSE)
	{
		wb_vec2 center = transform_object_point_seq(vec2(obj->x + layer_offset.x + object_offset.x, obj->y + layer_offset.y + object_offset.y), obj);
		draw_hand_ellipse(buf, center.x, center.y, transform_object_radius_seq(obj->p0.x, obj), transform_object_radius_seq(obj->p0.y, obj), transform_object_radius_seq(obj->thickness, obj), obj->colour, jitter_strength, frame + obj->id * 6553, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_POINT3D)
	{
		wb_vec2 p;
		
		if (project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q0, object_offset3d), obj), layer, &p))
			draw_disc(buf, p.x + layer_offset.x + object_offset.x, p.y + layer_offset.y + object_offset.y, obj->radius, obj->colour);
	}
	else if (obj->type == WB_OBJECT_OPEN_POINT3D)
	{
		wb_vec2 p;
		
		if (project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q0, object_offset3d), obj), layer, &p))
			draw_hand_open_point(buf, p.x + layer_offset.x + object_offset.x, p.y + layer_offset.y + object_offset.y, obj->radius, obj->thickness, obj->colour, jitter_strength, frame + obj->id * 6197, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_TRIANGLE3D)
	{
		wb_vec2 a;
		wb_vec2 b;
		wb_vec2 c;
		
		if (project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q0, object_offset3d), obj), layer, &a) && project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q1, object_offset3d), obj), layer, &b) && project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q2, object_offset3d), obj), layer, &c))
			draw_hand_triangle(buf, vec2(a.x + layer_offset.x + object_offset.x, a.y + layer_offset.y + object_offset.y), vec2(b.x + layer_offset.x + object_offset.x, b.y + layer_offset.y + object_offset.y), vec2(c.x + layer_offset.x + object_offset.x, c.y + layer_offset.y + object_offset.y), obj->thickness, obj->colour, jitter_strength, frame + obj->id * 8003, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_SHADE_TRIANGLE3D)
	{
		wb_vec2 a;
		wb_vec2 b;
		wb_vec2 c;
		
		if (project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q0, object_offset3d), obj), layer, &a) && project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q1, object_offset3d), obj), layer, &b) && project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q2, object_offset3d), obj), layer, &c))
			draw_triangle_with_alpha(buf, vec2(a.x + layer_offset.x + object_offset.x, a.y + layer_offset.y + object_offset.y), vec2(b.x + layer_offset.x + object_offset.x, b.y + layer_offset.y + object_offset.y), vec2(c.x + layer_offset.x + object_offset.x, c.y + layer_offset.y + object_offset.y), obj->colour, obj->size * obj->draw_progress * obj->render_alpha);
	}
	else if (obj->type == WB_OBJECT_LINE3D)
	{
		wb_vec2 a;
		wb_vec2 b;
		
		if (project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q0, object_offset3d), obj), layer, &a) && project_3d_point(transform_object_point3d_seq(vec3_sum(obj->q1, object_offset3d), obj), layer, &b))
			draw_hand_line(buf, vec2(a.x + layer_offset.x + object_offset.x, a.y + layer_offset.y + object_offset.y), vec2(b.x + layer_offset.x + object_offset.x, b.y + layer_offset.y + object_offset.y), obj->thickness, obj->colour, jitter_strength, frame + obj->id * 7901, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_CURVE3D)
	{
		wb_stack_quad_curve curve_storage;
		
		if (!build_projected_curve3d(&curve_storage, transform_object_point3d_seq(vec3_sum(obj->q0, object_offset3d), obj), transform_object_point3d_seq(vec3_sum(obj->q1, object_offset3d), obj), transform_object_point3d_seq(vec3_sum(obj->q2, object_offset3d), obj), layer))
			return;
		
		translate_nurbs_pcurve(&curve_storage.curve, layer_offset.x + object_offset.x, layer_offset.y + object_offset.y);
		if (jitter_strength > 0.0f)
			jitter_nurbs_pcurve(&curve_storage.curve, binary_max(0.6f, obj->thickness * 0.45f) * jitter_strength);
		draw_curve_stroke_progress(buf, &curve_storage.curve, obj->thickness, obj->colour, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_WIRE3D)
	{
		wb_vec2 points[16];
		int n_points = obj->n_points3d;
		
		if (!obj->points3d || n_points < 3 || n_points > 16)
			return;
		for (int i = 0; i < n_points; i++)
		{
			if (!project_3d_point(transform_object_point3d_seq(vec3_sum(obj->points3d[i], object_offset3d), obj), layer, &points[i]))
				return;
			points[i] = vec2(points[i].x + layer_offset.x + object_offset.x, points[i].y + layer_offset.y + object_offset.y);
		}
		draw_hand_polygon(buf, points, n_points, obj->thickness, obj->colour, jitter_strength, frame + obj->id * 8123, obj->draw_progress);
	}
	else if (obj->type == WB_OBJECT_SHADE_POLY3D)
	{
		wb_vec2 points[16];
		int n_points = obj->n_points3d;
		
		if (!obj->points3d || n_points < 3 || n_points > 16)
			return;
		for (int i = 0; i < n_points; i++)
		{
			if (!project_3d_point(transform_object_point3d_seq(vec3_sum(obj->points3d[i], object_offset3d), obj), layer, &points[i]))
				return;
			points[i] = vec2(points[i].x + layer_offset.x + object_offset.x, points[i].y + layer_offset.y + object_offset.y);
		}
		draw_polygon_with_alpha(buf, points, n_points, obj->colour, obj->thickness * obj->draw_progress * obj->render_alpha);
	}
}

/* Patches now own the painter order of their direct content.  Layers are
 * still the temporary compositing backend, so this traversal renders only
 * content assigned to the requested layer while preserving the patch tree's
 * source order. */
static int object_draw_order(const wb_scene *scene, int index)
{
	if (scene->objects[index].draw_order > 0)
		return scene->objects[index].draw_order;
	return scene->next_draw_order + index + 1;
}

static int patch_draw_order(const wb_scene *scene, int index)
{
	if (scene->patches[index].draw_order > 0)
		return scene->patches[index].draw_order;
	return scene->next_draw_order + scene->n_objects + index + 1;
}

static void composite_patch_buffer(uint8_t *dst, uint8_t *dst_alpha,
		uint8_t *src, const uint8_t *src_alpha, float opacity);
static void render_patch_layer_contents(wb_scene *scene, int patch_id,
		wb_render_context *layer, int frame, uint8_t *buf, uint8_t *alpha, int depth);

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

static void composite_patch_buffer(uint8_t *dst, uint8_t *dst_alpha,
		uint8_t *src, const uint8_t *src_alpha, float opacity)
{
	if (!dst || !dst_alpha || !src || !src_alpha)
		return;
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		int pixel = i * 3;
		float a = ((float)src_alpha[i] / 255.0f) * opacity;
		float dst_a = (float)dst_alpha[i] / 255.0f;
		float out_a;
		if (a <= 0.0f)
			continue;
		if (a > 1.0f)
			a = 1.0f;
		out_a = a + dst_a * (1.0f - a);
		dst[pixel + 0] = (uint8_t)(dst[pixel + 0] * (1.0f - a) + src[pixel + 0] * a);
		dst[pixel + 1] = (uint8_t)(dst[pixel + 1] * (1.0f - a) + src[pixel + 1] * a);
		dst[pixel + 2] = (uint8_t)(dst[pixel + 2] * (1.0f - a) + src[pixel + 2] * a);
		dst_alpha[i] = (uint8_t)(out_a * 255.0f);
	}
}

static void render_patch_layer_contents(wb_scene *scene, int patch_id,
		wb_render_context *layer, int frame, uint8_t *buf, uint8_t *alpha, int depth)
{
	int previous_order = -1;

	if (!scene || !layer || !buf || !alpha || depth > scene->n_patches ||
		!wb_scene_find_patch(scene, patch_id))
		return;

	for (;;)
	{
		int next_order = 0x7fffffff;
		int next_object = -1;
		int next_patch = -1;

		for (int i = 0; i < scene->n_objects; i++)
		{
			int order = object_draw_order(scene, i);
			if (scene->objects[i].patch_id == patch_id &&
				order > previous_order && order < next_order)
			{
				next_order = order;
				next_object = i;
				next_patch = -1;
			}
		}
		for (int i = 0; i < scene->n_patches; i++)
		{
			int order = patch_draw_order(scene, i);
			if (scene->patches[i].parent_id == patch_id &&
				order > previous_order && order < next_order)
			{
				next_order = order;
				next_object = -1;
				next_patch = i;
			}
		}
		if (next_object < 0 && next_patch < 0)
			break;

		previous_order = next_order;
		if (next_object >= 0)
		{
			set_draw_alpha_buffer(alpha);
			draw_scene_object(scene, &scene->objects[next_object], layer, frame, buf);
		}
		else
		{
			wb_scene_patch *patch = &scene->patches[next_patch];
			uint8_t *child_buf = calloc(WIDTH * HEIGHT, 3);
			uint8_t *child_alpha = calloc(WIDTH * HEIGHT, 1);
			if (!child_buf || !child_alpha)
			{
				free(child_buf);
				free(child_alpha);
				continue;
			}
			render_patch_layer_contents(scene, patch->id, layer, frame,
				child_buf, child_alpha, depth + 1);
			if (patch->glow_radius > 0.0f && patch->glow_opacity > 0.0f)
			{
				memcpy(scene->render_glow_buf, child_buf, WIDTH * HEIGHT * 3);
				memcpy(scene->render_glow_alpha, child_alpha, WIDTH * HEIGHT);
				blur_layer_buffer(scene->render_glow_buf, scene->render_scratch_buf, patch->glow_radius);
				blur_alpha_buffer(scene->render_glow_alpha, scene->render_scratch_alpha, patch->glow_radius);
				composite_patch_buffer(buf, alpha, scene->render_glow_buf,
					scene->render_glow_alpha, patch->render_opacity * patch->glow_opacity);
			}
			if (patch->blur_radius > 0.0f)
			{
				blur_layer_buffer(child_buf, scene->render_scratch_buf, patch->blur_radius);
				blur_alpha_buffer(child_alpha, scene->render_scratch_alpha, patch->blur_radius);
			}
			composite_patch_buffer(buf, alpha, child_buf, child_alpha, patch->render_opacity);
			free(child_buf);
			free(child_alpha);
		}
	}
	set_draw_alpha_buffer(alpha);
}

void wb_scene_render(wb_scene *scene, float time, int frame, uint8_t *buf)
{
	uint8_t *layer_buf;
	uint8_t *scratch_buf;
	uint8_t *glow_buf;
	uint8_t *layer_alpha;
	uint8_t *scratch_alpha;
	uint8_t *glow_alpha;
	
	if (!scene || !buf)
		return;
	
	if (scene->background_type == WB_BACKGROUND_RADIAL)
		fill_with_radial_gradient(buf, scene->background_center_colour, scene->background_edge_colour);
	else if (scene->background_type == WB_BACKGROUND_PAPER)
	{
		fill_with_radial_gradient(buf, scene->background_center_colour, scene->background_edge_colour);
		apply_paper_texture(buf);
	}
	else
		fill_with_colour(buf, 0xFFFFFF);
	
	for (int i = 0; i < scene->n_objects; i++)
	{
		scene->objects[i].draw_progress = 1.0f;
		scene->objects[i].render_alpha = 1.0f;
		scene->objects[i].render_jitter_strength = scene->objects[i].jitter_explicit ? scene->objects[i].jitter_strength : WB_DEFAULT_OBJECT_JITTER_STRENGTH;
		scene->objects[i].render_translation = vec2(0, 0);
		scene->objects[i].render_translation3d = vec3(0, 0, 0);
		scene->objects[i].render_patch_pivot = vec2(0, 0);
		scene->objects[i].render_patch_scale = vec2(1, 1);
		scene->objects[i].render_patch_rotation = 0.0f;
		scene->objects[i].render_patch_pivot3d = vec3(0, 0, 0);
		scene->objects[i].render_patch_scale3d = vec3(1, 1, 1);
		scene->objects[i].render_patch_rotation3d = vec3(0, 0, 0);
		scene->objects[i].n_render_patch_transforms = 0;
		scene->objects[i].n_render_patch_transforms3d = 0;
	}
	scene->render_context.render_opacity = scene->render_context.opacity;
	scene->render_context.render_offset = scene->render_context.offset;
	scene->render_context.render_jitter_strength = scene->render_context.jitter_explicit ? scene->render_context.jitter_strength : WB_DEFAULT_LAYER_JITTER_STRENGTH;
	scene->render_context.render_camera_distance = scene->render_context.camera_distance;
	scene->render_context.render_camera_scale = scene->render_context.camera_scale;
	scene->render_context.render_camera_yaw = scene->render_context.camera_yaw;
	scene->render_context.render_camera_projection = scene->render_context.camera_projection;
	scene->render_context.render_camera_center = scene->render_context.camera_center;
	scene->render_context.render_camera_target_explicit = scene->render_context.camera_target_explicit;
	scene->render_context.render_camera_target = scene->render_context.camera_target;
	for (int i = 0; i < scene->n_patches; i++)
	{
		scene->patches[i].render_opacity = scene->patches[i].opacity;
		scene->patches[i].render_jitter_strength = scene->patches[i].jitter_explicit ?
			scene->patches[i].jitter_strength : WB_DEFAULT_LAYER_JITTER_STRENGTH;
		scene->patches[i].render_translation = vec2(0, 0);
		scene->patches[i].render_translation3d = vec3(0, 0, 0);
		scene->patches[i].n_render_transforms = 0;
		scene->patches[i].n_render_transforms3d = 0;
		scene->patches[i].render_camera_distance = scene->patches[i].camera_distance;
		scene->patches[i].render_camera_scale = scene->patches[i].camera_scale;
		scene->patches[i].render_camera_yaw = scene->patches[i].camera_yaw;
		scene->patches[i].render_camera_projection = scene->patches[i].camera_projection;
		scene->patches[i].render_camera_center = scene->patches[i].camera_center;
		scene->patches[i].render_camera_target_explicit = scene->patches[i].camera_target_explicit;
		scene->patches[i].render_camera_target = scene->patches[i].camera_target;
	}
	
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
			if (!obj->jitter_explicit)
			{
				float motion_jitter = WB_AUTO_OBJECT_MOVE_JITTER_STRENGTH * sinf(a * PI);
				if (motion_jitter > obj->render_jitter_strength)
					obj->render_jitter_strength = motion_jitter;
			}
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
		else if (action->type == WB_ACTION_TRANSLATE)
		{
			if (!obj)
				continue;
			{
				float a = action_alpha(action, time);
				obj->render_translation.x += action->from.x + (action->to.x - action->from.x) * a;
				obj->render_translation.y += action->from.y + (action->to.y - action->from.y) * a;
			}
			if (!obj->jitter_explicit)
			{
				float a = action_alpha(action, time);
				float motion_jitter = WB_AUTO_OBJECT_MOVE_JITTER_STRENGTH * sinf(a * PI);
				if (motion_jitter > obj->render_jitter_strength)
					obj->render_jitter_strength = motion_jitter;
			}
		}
		else if (action->type == WB_ACTION_TRANSLATE3D)
		{
			if (!obj)
				continue;
			{
				float a = action_alpha(action, time);
				obj->render_translation3d.x += action->q0.x + (action->q1.x - action->q0.x) * a;
				obj->render_translation3d.y += action->q0.y + (action->q1.y - action->q0.y) * a;
				obj->render_translation3d.z += action->q0.z + (action->q1.z - action->q0.z) * a;
			}
			if (!obj->jitter_explicit)
			{
				float a = action_alpha(action, time);
				float motion_jitter = WB_AUTO_OBJECT_MOVE_JITTER_STRENGTH * sinf(a * PI);
				if (motion_jitter > obj->render_jitter_strength)
					obj->render_jitter_strength = motion_jitter;
			}
		}
		else if (action->type == WB_ACTION_TRANSFORM)
		{
			if (!obj)
				continue;
			{
				float a = action_alpha(action, time);
				if (obj->n_render_patch_transforms < (int)(sizeof(obj->render_patch_transforms) / sizeof(obj->render_patch_transforms[0])))
				{
					int idx = obj->n_render_patch_transforms++;
					obj->render_patch_transforms[idx].pivot = vec2(action->q0.x, action->q0.y);
					obj->render_patch_transforms[idx].scale = vec2(action->from.x + (action->to.x - action->from.x) * a, action->from.y + (action->to.y - action->from.y) * a);
					obj->render_patch_transforms[idx].rotation = action->aux0 + (action->aux1 - action->aux0) * a;
				}
			}
			if (!obj->jitter_explicit)
			{
				float a = action_alpha(action, time);
				float motion_jitter = WB_AUTO_OBJECT_MOVE_JITTER_STRENGTH * sinf(a * PI);
				if (motion_jitter > obj->render_jitter_strength)
					obj->render_jitter_strength = motion_jitter;
			}
		}
		else if (action->type == WB_ACTION_TRANSFORM3D)
		{
			if (!obj)
				continue;
			{
				float a = action_alpha(action, time);
				if (obj->n_render_patch_transforms3d < (int)(sizeof(obj->render_patch_transforms3d) / sizeof(obj->render_patch_transforms3d[0])))
				{
					int idx = obj->n_render_patch_transforms3d++;
					obj->render_patch_transforms3d[idx].pivot = action->q0;
					obj->render_patch_transforms3d[idx].scale = vec3(action->from.x + (action->to.x - action->from.x) * a, action->from.y + (action->to.y - action->from.y) * a, action->from_z + (action->to_z - action->from_z) * a);
					obj->render_patch_transforms3d[idx].rotation = vec3(action->q1.x + (action->q2.x - action->q1.x) * a, action->q1.y + (action->q2.y - action->q1.y) * a, action->q1.z + (action->q2.z - action->q1.z) * a);
				}
			}
			if (!obj->jitter_explicit)
			{
				float a = action_alpha(action, time);
				float motion_jitter = WB_AUTO_OBJECT_MOVE_JITTER_STRENGTH * sinf(a * PI);
				if (motion_jitter > obj->render_jitter_strength)
					obj->render_jitter_strength = motion_jitter;
			}
		}
		else if (action->type == WB_ACTION_PATCH_FADE)
		{
			wb_scene_patch *patch = wb_scene_find_patch(scene, action->patch_id);
			float a;
			if (!patch)
				continue;
			a = action_alpha(action, time);
			patch->render_opacity = action->from_z + (action->to_z - action->from_z) * a;
		}
		else if (action->type == WB_ACTION_PATCH_TRANSLATE)
		{
			wb_scene_patch *patch = wb_scene_find_patch(scene, action->patch_id);
			float a;
			if (!patch)
				continue;
			a = action_alpha(action, time);
			patch->render_translation.x += action->from.x + (action->to.x - action->from.x) * a;
			patch->render_translation.y += action->from.y + (action->to.y - action->from.y) * a;
		}
		else if (action->type == WB_ACTION_PATCH_TRANSLATE3D)
		{
			wb_scene_patch *patch = wb_scene_find_patch(scene, action->patch_id);
			float a;
			if (!patch)
				continue;
			patch->render_translation3d.x += action->q0.x + (action->q1.x - action->q0.x) * a;
			patch->render_translation3d.y += action->q0.y + (action->q1.y - action->q0.y) * a;
			patch->render_translation3d.z += action->q0.z + (action->q1.z - action->q0.z) * a;
		}
		else if (action->type == WB_ACTION_PATCH_TRANSFORM)
		{
			wb_scene_patch *patch = wb_scene_find_patch(scene, action->patch_id);
			float a;
			if (!patch || patch->n_render_transforms >= 8) continue;
			a = action_alpha(action, time);
			patch->render_transforms[patch->n_render_transforms].pivot = vec2(action->q0.x, action->q0.y);
			patch->render_transforms[patch->n_render_transforms].scale = vec2(action->from.x + (action->to.x - action->from.x) * a, action->from.y + (action->to.y - action->from.y) * a);
			patch->render_transforms[patch->n_render_transforms++].rotation = action->aux0 + (action->aux1 - action->aux0) * a;
		}
		else if (action->type == WB_ACTION_PATCH_TRANSFORM3D)
		{
			wb_scene_patch *patch = wb_scene_find_patch(scene, action->patch_id);
			float a;
			int index;
			if (!patch || patch->n_render_transforms3d >= 8) continue;
			a = action_alpha(action, time);
			index = patch->n_render_transforms3d++;
			patch->render_transforms3d[index].pivot = action->q0;
			patch->render_transforms3d[index].scale = vec3(action->from.x + (action->to.x - action->from.x) * a, action->from.y + (action->to.y - action->from.y) * a, action->from_z + (action->to_z - action->from_z) * a);
			patch->render_transforms3d[index].rotation = vec3(action->q1.x + (action->q2.x - action->q1.x) * a, action->q1.y + (action->q2.y - action->q1.y) * a, action->q1.z + (action->q2.z - action->q1.z) * a);
		}
		else if (action->type == WB_ACTION_PATCH_CAMERA_MOVE)
		{
			wb_scene_patch *patch = wb_scene_find_patch(scene, action->patch_id);
			float a;
			if (!patch) continue;
			a = action_alpha(action, time);
			patch->render_camera_distance = action->from_z + (action->to_z - action->from_z) * a;
			patch->render_camera_scale = action->aux0 + (action->aux1 - action->aux0) * a;
			patch->render_camera_yaw = action->aux2 + (action->aux3 - action->aux2) * a;
			patch->render_camera_center = vec2(action->from.x + (action->to.x - action->from.x) * a, action->from.y + (action->to.y - action->from.y) * a);
			patch->render_camera_target_explicit = (action->flags & 1) || (action->flags & 2);
			patch->render_camera_target = vec3(action->q0.x + (action->q1.x - action->q0.x) * a, action->q0.y + (action->q1.y - action->q0.y) * a, action->q0.z + (action->q1.z - action->q0.z) * a);
		}
		else if (action->type == WB_ACTION_PATCH_CAMERA_ORBIT)
		{
			wb_scene_patch *patch = wb_scene_find_patch(scene, action->patch_id);
			if (patch) patch->render_camera_yaw = action->aux2 + (action->aux3 - action->aux2) * action_alpha(action, time);
		}
		else if (action->type == WB_ACTION_FADE)
		{
			if (!obj)
				continue;
			
			float a = action_alpha(action, time);
			obj->render_alpha = action->from_z + (action->to_z - action->from_z) * a;
			if (obj->render_alpha < 0.0f)
				obj->render_alpha = 0.0f;
			if (obj->render_alpha > 1.0f)
				obj->render_alpha = 1.0f;
		}
	}
	
	if (!ensure_render_buffers(scene))
		return;
	
	layer_buf = scene->render_layer_buf;
	scratch_buf = scene->render_scratch_buf;
	glow_buf = scene->render_glow_buf;
	layer_alpha = scene->render_layer_alpha;
	scratch_alpha = scene->render_scratch_alpha;
	glow_alpha = scene->render_glow_alpha;
	
	/* Layers no longer partition painter traversal.  The retained root patch is
	 * rendered once; the default layer is only a temporary draw-context shell
	 * for old primitive helpers while the layer type is removed. */
	{
		wb_render_context *layer = &scene->render_context;
		wb_scene_patch *root_patch = wb_scene_find_patch(scene, scene->root_patch_id);
		clear_layer_buffer(layer_buf);
		clear_alpha_buffer(layer_alpha);
		set_draw_alpha_buffer(layer_alpha);
		
		render_patch_layer_contents(scene, scene->root_patch_id, layer,
			frame, layer_buf, layer_alpha, 0);
		
		set_draw_alpha_buffer(NULL);
		if (root_patch && root_patch->glow_radius > 0.0f && root_patch->glow_opacity > 0.0f)
		{
			memcpy(glow_buf, layer_buf, WIDTH * HEIGHT * 3);
			memcpy(glow_alpha, layer_alpha, WIDTH * HEIGHT);
			blur_layer_buffer(glow_buf, scratch_buf, root_patch->glow_radius);
			blur_alpha_buffer(glow_alpha, scratch_alpha, root_patch->glow_radius);
			composite_layer_buffer(buf, glow_buf, glow_alpha,
				root_patch->render_opacity * root_patch->glow_opacity);
		}
		if (root_patch && root_patch->blur_radius > 0.0f)
		{
			blur_layer_buffer(layer_buf, scratch_buf, root_patch->blur_radius);
			blur_alpha_buffer(layer_alpha, scratch_alpha, root_patch->blur_radius);
		}
		if (layer->glow_radius > 0.0f && layer->glow_opacity > 0.0f)
		{
			memcpy(glow_buf, layer_buf, WIDTH * HEIGHT * 3);
			memcpy(glow_alpha, layer_alpha, WIDTH * HEIGHT);
			blur_layer_buffer(glow_buf, scratch_buf, layer->glow_radius);
			blur_alpha_buffer(glow_alpha, scratch_alpha, layer->glow_radius);
			composite_layer_buffer(buf, glow_buf, glow_alpha, layer->render_opacity * layer->glow_opacity);
		}
		if (layer->blur_radius > 0.0f)
		{
			blur_layer_buffer(layer_buf, scratch_buf, layer->blur_radius);
			blur_alpha_buffer(layer_alpha, scratch_alpha, layer->blur_radius);
		}
		composite_layer_buffer(buf, layer_buf, layer_alpha,
			layer->render_opacity * (root_patch ? root_patch->render_opacity : 1.0f));
	}
}
