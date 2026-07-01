#ifndef WB_RASTERIZE_H_
#define WB_RASTERIZE_H_

#define LAZY_KERNING_PAD 0.2

#define COLOUR(r, g, b) ((uint32_t)(r | (g << 8) | (b << 16)))

#define COLOUR_R(x) (uint8_t)(((uint32_t)(x) & 0x0000FF) >> 0)
#define COLOUR_G(x) (uint8_t)(((uint32_t)(x) & 0x00FF00) >> 8)
#define COLOUR_B(x) (uint8_t)(((uint32_t)(x) & 0xFF0000) >> 16)

#define N_SAMPLE_POINTS 100

#define NICE_BLUE 0x4d80e4

typedef struct
{
	int n_points;
	float thickness;
	wb_vec2 *points;
	uint32_t colour;
} wb_plane_polyline;

void set_render_dimensions(int width, int height);
void set_draw_alpha_buffer(uint8_t *alpha);

wb_plane_polyline *new_plane_polyline(int n_points, float thickness);
void free_plane_polyline(wb_plane_polyline *pl);
wb_plane_polyline *nurbs_pcurve_to_ppolyline(wb_nurbs_pcurve *curve, int n_points, float thickness);

void polyline_set_colour(wb_plane_polyline *pl, uint32_t colour);

void draw_pixel(uint8_t *buf, int x, int y, uint32_t colour);
void fill_with_colour(uint8_t *buf, uint32_t colour);
void fill_with_radial_gradient(uint8_t *buf, uint32_t center_colour, uint32_t edge_colour);
void draw_disc(uint8_t *buf, float x, float y, float radius, uint32_t colour);
void draw_disc_with_alpha(uint8_t *buf, float x, float y, float radius, uint32_t colour, float opacity);
void draw_triangle_with_alpha(uint8_t *buf, wb_vec2 a, wb_vec2 b, wb_vec2 c, uint32_t colour, float opacity);

void draw_nurbs_pcurve(uint8_t *buf, wb_nurbs_pcurve *curve);

void draw_ppolyline(uint8_t *buf, wb_plane_polyline *pl);
void draw_ppolyline_in_colour(uint8_t *buf, wb_plane_polyline *pl, uint32_t colour);

void draw_plane_figure(uint8_t *buf, wb_plane_figure *fig);
void draw_plane_figure_in_colour(uint8_t *buf, wb_plane_figure *fig, uint32_t colour);

float draw_char(uint8_t *buf, char c, int x, int y, int height, uint32_t colour);
void draw_string(uint8_t *buf, char *str, int x, int y, int height, uint32_t colour);

void draw_sausage(uint8_t *buf, wb_vec2 s, wb_vec2 d, float thickness, uint32_t colour);

#endif
