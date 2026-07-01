#ifndef WB_SCENE_H_
#define WB_SCENE_H_

#define WB_SCENE_2D 0
#define WB_SCENE_3D 0

typedef struct
{
	int type;
	float time;
} wb_scene_event;

DECLARE_LINKED_PTR_LIST(wb_scene_event);

typedef struct
{
	wb_quaternion orientation;
	wb_vec3 position;
} wb_camera;

void init_camera(wb_camera *camera);

typedef struct
{
	int id;
	char name[64];
	int type;
	float opacity;
	float blur_radius;
	float jitter_strength;
	float camera_distance;
	float camera_scale;
	wb_vec2 camera_center;
	wb_vec2 offset;
	wb_vec2 render_offset;
} wb_scene_layer;

typedef struct
{
	int id;
	int type;
	int layer_id;
	wb_math_formula *math;
	float x;
	float y;
	wb_vec2 p0;
	wb_vec2 p1;
	wb_vec3 q0;
	wb_vec3 q1;
	wb_vec3 q2;
	float radius;
	float thickness;
	float size;
	uint32_t colour;
	float draw_progress;
	float jitter_strength;
} wb_scene_object;

typedef struct
{
	int object_id;
	int layer_id;
	int type;
	float start_time;
	float end_time;
	wb_vec2 from;
	wb_vec2 to;
} wb_scene_action;

#define WB_OBJECT_MATH 1
#define WB_OBJECT_LINE 2
#define WB_OBJECT_POINT 3
#define WB_OBJECT_OPEN_POINT 4
#define WB_OBJECT_LINE3D 5
#define WB_OBJECT_CURVE3D 6
#define WB_OBJECT_CIRCLE 7
#define WB_OBJECT_DOTTED_LINE 8
#define WB_OBJECT_ARROW 9
#define WB_OBJECT_SHADE_DISC 10
#define WB_ACTION_MOVE 1
#define WB_ACTION_DRAW 2
#define WB_ACTION_LAYER_MOVE 3

typedef struct
{
	wb_plane_figure_pll *pfigs;
	wb_space_figure_pll *sfigs;
	wb_scene_layer *layers;
	int n_layers;
	int cap_layers;
	int current_layer_id;
	wb_scene_object *objects;
	int n_objects;
	int cap_objects;
	wb_scene_action *actions;
	int n_actions;
	int cap_actions;
	int next_object_id;
	
	float total_duration;
	float current_time;
	int background_type;
	uint32_t background_center_colour;
	uint32_t background_edge_colour;
	
	wb_scene_event_pll *events;
	wb_camera camera;
} wb_scene;

#define WB_BACKGROUND_RADIAL 1
#define WB_LAYER_2D 1
#define WB_LAYER_3D 2

wb_scene *new_scene();
void free_scene(wb_scene *scene);
void wb_scene_set_radial_background(wb_scene *scene, uint32_t center_colour, uint32_t edge_colour);
int wb_scene_add_layer(wb_scene *scene, const char *name, int type, float opacity);
void wb_scene_set_layer_blur(wb_scene *scene, int layer_id, float blur_radius);
void wb_scene_set_layer_jitter(wb_scene *scene, int layer_id, float jitter_strength);
void wb_scene_set_layer_camera(wb_scene *scene, int layer_id, float distance, float scale, float center_x, float center_y);
void wb_scene_set_object_jitter(wb_scene *scene, int object_id, float jitter_strength);
void wb_scene_set_current_layer(wb_scene *scene, int layer_id);
void wb_scene_move_layer(wb_scene *scene, int layer_id, float start_time, float end_time, float x1, float y1, float x2, float y2);
int wb_scene_add_math(wb_scene *scene, const char *src, float x, float y, float size, uint32_t colour);
int wb_scene_add_line(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, uint32_t colour);
int wb_scene_add_dotted_line(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, float gap, uint32_t colour);
int wb_scene_add_arrow(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, float head_size, uint32_t colour);
int wb_scene_add_line3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float thickness, uint32_t colour);
int wb_scene_add_curve3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float thickness, uint32_t colour);
int wb_scene_add_point(wb_scene *scene, float x, float y, float radius, uint32_t colour);
int wb_scene_add_open_point(wb_scene *scene, float x, float y, float radius, float thickness, uint32_t colour);
int wb_scene_add_circle(wb_scene *scene, float x, float y, float radius, float thickness, uint32_t colour);
int wb_scene_add_shade_disc(wb_scene *scene, float x, float y, float radius, uint32_t colour, float opacity);
void wb_scene_move(wb_scene *scene, int object_id, float start_time, float end_time, float x1, float y1, float x2, float y2);
void wb_scene_draw_in(wb_scene *scene, int object_id, float start_time, float end_time);
float wb_ease_grassroots(float t);
void wb_scene_render(wb_scene *scene, float time, int frame, uint8_t *buf);

#endif
