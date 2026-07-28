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

/* The implicit top-level patch.  Its horizontal extent follows the render
 * aspect ratio; half_height is the positive world-space y extent. */
typedef struct
{
	wb_vec2 center;
	float half_height;
} wb_scene_viewport;

void init_camera(wb_camera *camera);

typedef struct
{
	int id;
	char name[64];
	int type;
	float opacity;
	float render_opacity;
	float blur_radius;
	float glow_radius;
	float glow_opacity;
	float jitter_strength;
	int jitter_explicit;
	float render_jitter_strength;
	float camera_distance;
	float camera_scale;
	float camera_yaw;
	int camera_projection;
	wb_vec2 camera_center;
	int camera_target_explicit;
	wb_vec3 camera_target;
	float render_camera_distance;
	float render_camera_scale;
	float render_camera_yaw;
	int render_camera_projection;
	wb_vec2 render_camera_center;
	int render_camera_target_explicit;
	wb_vec3 render_camera_target;
	wb_vec2 offset;
	wb_vec2 render_offset;
} wb_render_context;

/* Retained authoring-space node.  Layers remain a transitional compositor
 * implementation detail until recursive patch rendering replaces them. */
typedef struct
{
	int id;
	int parent_id;
	char name[64];
	int dimension;
	int coord_type;
	int z_index;
	int draw_order;
	wb_vec2 origin;
	wb_vec2 scale;
	float rotation;
	wb_vec3 origin3;
	wb_vec3 scale3;
	wb_vec3 rotation3;
	float opacity;
	float render_opacity;
	float blur_radius;
	float glow_radius;
	float glow_opacity;
	float jitter_strength;
	int jitter_explicit;
	float render_jitter_strength;
	wb_vec2 render_translation;
	wb_vec3 render_translation3d;
	int n_render_transforms;
	struct { wb_vec2 pivot; wb_vec2 scale; float rotation; } render_transforms[8];
	int n_render_transforms3d;
	struct { wb_vec3 pivot; wb_vec3 scale; wb_vec3 rotation; } render_transforms3d[8];
	float camera_distance;
	float camera_scale;
	float camera_yaw;
	int camera_projection;
	wb_vec2 camera_center;
	int camera_target_explicit;
	wb_vec3 camera_target;
	float render_camera_distance;
	float render_camera_scale;
	float render_camera_yaw;
	int render_camera_projection;
	wb_vec2 render_camera_center;
	int render_camera_target_explicit;
	wb_vec3 render_camera_target;
} wb_scene_patch;

typedef struct
{
	int id;
	int type;
	int patch_id;
	int draw_order;
	wb_math_formula *math;
	char *text;
	wb_vec3 *points3d;
	int n_points3d;
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
	int jitter_explicit;
	float render_jitter_strength;
	float render_alpha;
	wb_vec2 render_translation;
	wb_vec3 render_translation3d;
	wb_vec2 render_patch_pivot;
	wb_vec2 render_patch_scale;
	float render_patch_rotation;
	wb_vec3 render_patch_pivot3d;
	wb_vec3 render_patch_scale3d;
	wb_vec3 render_patch_rotation3d;
	int n_render_patch_transforms;
	struct
	{
		wb_vec2 pivot;
		wb_vec2 scale;
		float rotation;
	} render_patch_transforms[8];
	int n_render_patch_transforms3d;
	struct
	{
		wb_vec3 pivot;
		wb_vec3 scale;
		wb_vec3 rotation;
	} render_patch_transforms3d[8];
} wb_scene_object;

typedef struct
{
	int object_id;
	int patch_id;
	int type;
	float start_time;
	float end_time;
	wb_vec2 from;
	wb_vec2 to;
	float from_z;
	float to_z;
	float aux0;
	float aux1;
	float aux2;
	float aux3;
	wb_vec3 q0;
	wb_vec3 q1;
	wb_vec3 q2;
	int flags;
} wb_scene_action;

#define WB_OBJECT_MATH 1
#define WB_OBJECT_TEXT 2
#define WB_OBJECT_LINE 3
#define WB_OBJECT_CURVE 4
#define WB_OBJECT_POINT 5
#define WB_OBJECT_OPEN_POINT 6
#define WB_OBJECT_LINE3D 7
#define WB_OBJECT_CURVE3D 8
#define WB_OBJECT_WIRE3D 9
#define WB_OBJECT_SHADE_POLY3D 10
#define WB_OBJECT_CIRCLE 11
#define WB_OBJECT_DOTTED_LINE 12
#define WB_OBJECT_ARROW 13
#define WB_OBJECT_SHADE_DISC 14
#define WB_OBJECT_TRIANGLE 15
#define WB_OBJECT_SHADE_TRIANGLE 16
#define WB_OBJECT_QUAD 17
#define WB_OBJECT_RAY 18
#define WB_OBJECT_ELLIPSE 19
#define WB_OBJECT_POLYGON 20
#define WB_OBJECT_SHADE_POLYGON 21
#define WB_OBJECT_BLOB 22
#define WB_OBJECT_DASHED_LINE 23
#define WB_OBJECT_POINT3D 24
#define WB_OBJECT_OPEN_POINT3D 25
#define WB_OBJECT_TRIANGLE3D 26
#define WB_OBJECT_SHADE_TRIANGLE3D 27
#define WB_OBJECT_SHADE_BLOB 28
#define WB_ACTION_MOVE 1
#define WB_ACTION_DRAW 2
#define WB_ACTION_FADE 6
#define WB_ACTION_TRANSLATE 8
#define WB_ACTION_TRANSFORM 9
#define WB_ACTION_TRANSLATE3D 10
#define WB_ACTION_TRANSFORM3D 11
#define WB_ACTION_PATCH_FADE 12
#define WB_ACTION_PATCH_TRANSLATE 13
#define WB_ACTION_PATCH_TRANSLATE3D 14
#define WB_ACTION_PATCH_TRANSFORM 15
#define WB_ACTION_PATCH_TRANSFORM3D 16
#define WB_ACTION_PATCH_CAMERA_MOVE 17
#define WB_ACTION_PATCH_CAMERA_ORBIT 18

typedef struct
{
	wb_plane_figure_pll *pfigs;
	wb_space_figure_pll *sfigs;
	wb_render_context render_context;
	wb_scene_patch *patches;
	int n_patches;
	int cap_patches;
	int root_patch_id;
	int current_patch_id;
	wb_scene_object *objects;
	int n_objects;
	int cap_objects;
	wb_scene_action *actions;
	int n_actions;
	int cap_actions;
	int next_object_id;
	int next_patch_id;
	int next_draw_order;
	
	float total_duration;
	float current_time;
	wb_scene_viewport root_viewport;
	int background_type;
	uint32_t background_center_colour;
	uint32_t background_edge_colour;
	uint8_t *render_layer_buf;
	uint8_t *render_scratch_buf;
	uint8_t *render_glow_buf;
	uint8_t *render_layer_alpha;
	uint8_t *render_scratch_alpha;
	uint8_t *render_glow_alpha;
	
	wb_scene_event_pll *events;
	wb_camera camera;
} wb_scene;

#define WB_BACKGROUND_RADIAL 1
#define WB_BACKGROUND_PAPER 2
#define WB_LAYER_2D 1
#define WB_LAYER_3D 2

#define WB_CAMERA_PROJECTION_PERSPECTIVE 1
#define WB_CAMERA_PROJECTION_ORTHOGRAPHIC 2

wb_scene *new_scene();
void free_scene(wb_scene *scene);
void wb_scene_set_radial_background(wb_scene *scene, uint32_t center_colour, uint32_t edge_colour);
void wb_scene_set_paper_background(wb_scene *scene, uint32_t center_colour, uint32_t edge_colour);
void wb_scene_set_root_viewport(wb_scene *scene, float center_x, float center_y, float half_height);
int wb_scene_add_layer(wb_scene *scene, const char *name, int type, float opacity);
int wb_scene_add_patch(wb_scene *scene, const char *name, int parent_id, int dimension, int coord_type);
void wb_scene_set_current_patch(wb_scene *scene, int patch_id);
wb_scene_patch *wb_scene_find_patch(wb_scene *scene, int patch_id);
void wb_scene_set_patch_transform(wb_scene *scene, int patch_id, wb_vec2 origin, wb_vec2 scale, float rotation, wb_vec3 origin3, wb_vec3 scale3, wb_vec3 rotation3);
void wb_scene_set_layer_blur(wb_scene *scene, int layer_id, float blur_radius);
void wb_scene_set_layer_glow(wb_scene *scene, int layer_id, float glow_radius, float glow_opacity);
void wb_scene_set_layer_jitter(wb_scene *scene, int layer_id, float jitter_strength);
void wb_scene_set_layer_camera(wb_scene *scene, int layer_id, float distance, float scale, float yaw, int projection, float center_x, float center_y);
void wb_scene_set_layer_camera_target(wb_scene *scene, int layer_id, int explicit_target, float x, float y, float z);
void wb_scene_set_object_jitter(wb_scene *scene, int object_id, float jitter_strength);
void wb_scene_set_current_layer(wb_scene *scene, int layer_id);
void wb_scene_fade_patch(wb_scene *scene, int patch_id, float start_time, float end_time, float opacity1, float opacity2);
void wb_scene_translate_patch(wb_scene *scene, int patch_id, float start_time, float end_time, wb_vec2 from, wb_vec2 to);
void wb_scene_translate_patch3d(wb_scene *scene, int patch_id, float start_time, float end_time, wb_vec3 from, wb_vec3 to);
void wb_scene_transform_patch(wb_scene *scene, int patch_id, float start_time, float end_time, wb_vec2 pivot, wb_vec2 scale1, float rotation1, wb_vec2 scale2, float rotation2);
void wb_scene_transform_patch3d(wb_scene *scene, int patch_id, float start_time, float end_time, wb_vec3 pivot, wb_vec3 scale1, wb_vec3 rotation1, wb_vec3 scale2, wb_vec3 rotation2);
void wb_scene_set_patch_camera(wb_scene *scene, int patch_id, float distance, float scale, float yaw, int projection, wb_vec2 center, int target_explicit, wb_vec3 target);
void wb_scene_move_patch_camera(wb_scene *scene, int patch_id, float start_time, float end_time, float distance1, float scale1, float yaw1, wb_vec2 center1, int target1_explicit, wb_vec3 target1, float distance2, float scale2, float yaw2, wb_vec2 center2, int target2_explicit, wb_vec3 target2);
void wb_scene_orbit_patch_camera(wb_scene *scene, int patch_id, float start_time, float end_time, float yaw1, float yaw2);
void wb_scene_fade_object(wb_scene *scene, int object_id, float start_time, float end_time, float opacity1, float opacity2);
void wb_scene_translate3d(wb_scene *scene, int object_id, float start_time, float end_time, float x1, float y1, float z1, float x2, float y2, float z2);
void wb_scene_transform3d(wb_scene *scene, int object_id, float start_time, float end_time, float pivot_x, float pivot_y, float pivot_z, float scale_x1, float scale_y1, float scale_z1, float yaw1, float pitch1, float roll1, float scale_x2, float scale_y2, float scale_z2, float yaw2, float pitch2, float roll2);
int wb_scene_add_math(wb_scene *scene, const char *src, float x, float y, float size, uint32_t colour);
int wb_scene_add_text(wb_scene *scene, const char *src, float x, float y, float size, uint32_t colour);
int wb_scene_add_curve(wb_scene *scene, float x0, float y0, float x1, float y1, float x2, float y2, float thickness, uint32_t colour);
int wb_scene_add_line(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, uint32_t colour);
int wb_scene_add_ray(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, uint32_t colour);
int wb_scene_add_dotted_line(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, float gap, uint32_t colour);
int wb_scene_add_dashed_line(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, float gap, uint32_t colour);
int wb_scene_add_arrow(wb_scene *scene, float x0, float y0, float x1, float y1, float thickness, float head_size, uint32_t colour);
int wb_scene_add_triangle(wb_scene *scene, float x0, float y0, float x1, float y1, float x2, float y2, float thickness, uint32_t colour);
int wb_scene_add_shade_triangle(wb_scene *scene, float x0, float y0, float x1, float y1, float x2, float y2, uint32_t colour, float opacity);
int wb_scene_add_quad(wb_scene *scene, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float thickness, uint32_t colour);
int wb_scene_add_polygon(wb_scene *scene, const wb_vec2 *points, int n_points, float thickness, uint32_t colour);
int wb_scene_add_shade_polygon(wb_scene *scene, const wb_vec2 *points, int n_points, uint32_t colour, float opacity);
int wb_scene_add_blob(wb_scene *scene, const wb_vec2 *points, int n_points, float thickness, uint32_t colour);
int wb_scene_add_shade_blob(wb_scene *scene, const wb_vec2 *points, int n_points, uint32_t colour, float opacity);
int wb_scene_add_wire3d(wb_scene *scene, const wb_vec3 *points, int n_points, float thickness, uint32_t colour);
int wb_scene_add_shade_poly3d(wb_scene *scene, const wb_vec3 *points, int n_points, uint32_t colour, float opacity);
int wb_scene_add_point3d(wb_scene *scene, float x, float y, float z, float radius, uint32_t colour);
int wb_scene_add_open_point3d(wb_scene *scene, float x, float y, float z, float radius, float thickness, uint32_t colour);
int wb_scene_add_triangle3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float thickness, uint32_t colour);
int wb_scene_add_shade_triangle3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, uint32_t colour, float opacity);
int wb_scene_add_line3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float thickness, uint32_t colour);
int wb_scene_add_curve3d(wb_scene *scene, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float thickness, uint32_t colour);
int wb_scene_add_point(wb_scene *scene, float x, float y, float radius, uint32_t colour);
int wb_scene_add_open_point(wb_scene *scene, float x, float y, float radius, float thickness, uint32_t colour);
int wb_scene_add_circle(wb_scene *scene, float x, float y, float radius, float thickness, uint32_t colour);
int wb_scene_add_ellipse(wb_scene *scene, float x, float y, float radius_x, float radius_y, float thickness, uint32_t colour);
int wb_scene_add_shade_disc(wb_scene *scene, float x, float y, float radius, uint32_t colour, float opacity);
void wb_scene_move(wb_scene *scene, int object_id, float start_time, float end_time, float x1, float y1, float x2, float y2);
void wb_scene_translate(wb_scene *scene, int object_id, float start_time, float end_time, float x1, float y1, float x2, float y2);
void wb_scene_transform(wb_scene *scene, int object_id, float start_time, float end_time, float pivot_x, float pivot_y, float scale_x1, float scale_y1, float rotation1, float scale_x2, float scale_y2, float rotation2);
void wb_scene_draw_in(wb_scene *scene, int object_id, float start_time, float end_time);
float wb_ease_grassroots(float t);
void wb_scene_render(wb_scene *scene, float time, int frame, uint8_t *buf);

#endif
