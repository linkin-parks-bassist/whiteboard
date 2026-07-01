#include "whiteboard.h"

static int render_width;
static int render_height;
static uint8_t *draw_alpha_buffer;

static inline double sqrf(double x) { return x * x; }
static inline int floor_to_int(float x) { return (int)floorf(x); }
static inline int ceil_to_int(float x) { return (int)ceilf(x); }

void set_render_dimensions(int width, int height)
{
	render_width  = width;
	render_height = height;
}

void set_draw_alpha_buffer(uint8_t *alpha)
{
	draw_alpha_buffer = alpha;
}

wb_plane_polyline *new_plane_polyline(int n_points, float thickness)
{
	wb_plane_polyline *result = malloc(sizeof(wb_plane_polyline));
	
	if (!result)
		return NULL;
	
	result->n_points  = n_points;
	result->thickness = thickness;
	
	result->points = malloc(sizeof(wb_vec2) * n_points);
	
	if (!result->points)
	{
		free(result);
		return NULL;
	}
	
	return result;
}

void free_plane_polyline(wb_plane_polyline *pl)
{
	if (!pl)
		return;
	
	if (pl->points)
		free(pl->points);
	
	free(pl);
}


wb_plane_polyline *nurbs_pcurve_to_ppolyline(wb_nurbs_pcurve *curve, int n_points, float thickness)
{
	if (!curve)
		return NULL;
	
	if (!curve->nx || !curve->ny || n_points < 1)
	{
		printf("nurbs_pcurve_to_ppolyline abort 1\n");
		return NULL;
	}
	
	if (!curve->nx->knots || !curve->ny->knots)
		return NULL;
	
	wb_plane_polyline *result = new_plane_polyline(n_points, thickness);
	
	if (!result)
	{
		printf("nurbs_pcurve_to_ppolyline abort 2\n");
		return NULL;
	}
	
	result->n_points  = n_points;
	result->thickness = thickness;
	
	result->colour = curve->colour;
	
	float u_x = curve->nx->knots[curve->nx->degree];
	float u_y = curve->ny->knots[curve->ny->degree];
	
	float u_step_x = (curve->nx->knots[curve->nx->n_knots - curve->nx->degree - 1] - curve->nx->knots[curve->nx->degree]) / (n_points - 1);
	float u_step_y = (curve->ny->knots[curve->ny->n_knots - curve->nx->degree - 1] - curve->ny->knots[curve->ny->degree]) / (n_points - 1);
	
	for (int i = 0; i < n_points; i++)
	{
		
		result->points[i].x = nurbs(curve->nx, u_x);
		result->points[i].y = nurbs(curve->ny, u_y);
		
		u_x += u_step_x;
		u_y += u_step_y;
	}
	
	return result;
}

float clamp(float x, float a, float b)
{
	float m = binary_min(a, b);
	float M = binary_max(a, b);
	
	return binary_max(binary_min(x, M), m);
}

uint32_t interpolate_colours(uint32_t a, uint32_t b, float t)
{
	float ra = (float)COLOUR_R(a) / 255.0;
	float ga = (float)COLOUR_G(a) / 255.0;
	float ba = (float)COLOUR_B(a) / 255.0;
	
	float rb = (float)COLOUR_R(b) / 255.0;
	float gb = (float)COLOUR_G(b) / 255.0;
	float bb = (float)COLOUR_B(b) / 255.0;
	
	t = clamp(t, 0, 1);
	
	return COLOUR((int)(((1 - t) * ra + t * rb) * 255.0), (int)(((1 - t) * ga + t * gb) * 255.0), (int)(((1 - t) * ba + t * bb) * 255.0));
}

void polyline_set_colour(wb_plane_polyline *pl, uint32_t colour)
{
	if (!pl)
		return;
	
	pl->colour = colour;
}

void draw_pixel(uint8_t *buf, int x, int y, uint32_t colour)
{
	if (!buf)
		return;
	
	if (x < 0 || render_width - 1 < x || y < 0 || render_height - 1 < y)
		return;
	
	int ind = (y * render_width + x) * 3;
	buf[ind++] = COLOUR_B(colour);
	buf[ind++] = COLOUR_G(colour);
	buf[ind++] = COLOUR_R(colour);
	if (draw_alpha_buffer)
		draw_alpha_buffer[y * render_width + x] = 255;
}

uint32_t read_pixel(uint8_t *buf, int x, int y)
{
	if (!buf)
		return 0;
	
	if (x < 0 || render_width - 1 < x || y < 0 || render_height - 1 < y)
		return 0;
	
	int ind = (y * render_width + x) * 3;
	
	return COLOUR(buf[ind+2], buf[ind+1], buf[ind]);
}

static inline void blend_pixel(uint8_t *buf, int x, int y, uint32_t colour, float alpha)
{
	if (alpha <= 0.0f)
		return;
	
	if (x < 0 || render_width - 1 < x || y < 0 || render_height - 1 < y)
		return;
	
	if (alpha > 1.0f)
		alpha = 1.0f;
	
	int ind = (y * render_width + x) * 3;
	float inv = 1.0f - alpha;
	buf[ind + 0] = (uint8_t)(buf[ind + 0] * inv + COLOUR_B(colour) * alpha);
	buf[ind + 1] = (uint8_t)(buf[ind + 1] * inv + COLOUR_G(colour) * alpha);
	buf[ind + 2] = (uint8_t)(buf[ind + 2] * inv + COLOUR_R(colour) * alpha);
	if (draw_alpha_buffer)
	{
		int a = (int)(alpha * 255.0f);
		int ai = y * render_width + x;
		if (a > draw_alpha_buffer[ai])
			draw_alpha_buffer[ai] = (uint8_t)a;
	}
}

void fill_with_colour(uint8_t *buf, uint32_t colour)
{
	if (!buf)
		return;
	
	for (int i = 0; i < render_width * render_height; i++)
	{
		buf[3 * i + 0] = COLOUR_B(colour);
		buf[3 * i + 1] = COLOUR_G(colour);
		buf[3 * i + 2] = COLOUR_R(colour);
	}
}

void fill_with_radial_gradient(uint8_t *buf, uint32_t center_colour, uint32_t edge_colour)
{
	if (!buf)
		return;
	
	float cx = ((float)render_width - 1.0f) * 0.5f;
	float cy = ((float)render_height - 1.0f) * 0.5f;
	float max_dist = sqrtf(cx * cx + cy * cy);
	
	if (max_dist <= 0.0f)
	{
		fill_with_colour(buf, center_colour);
		return;
	}
	
	for (int y = 0; y < render_height; y++)
	{
		for (int x = 0; x < render_width; x++)
		{
			float dx = (float)x - cx;
			float dy = (float)y - cy;
			float t = sqrtf(dx * dx + dy * dy) / max_dist;
			uint32_t colour = interpolate_colours(center_colour, edge_colour, t);
			int ind = (y * render_width + x) * 3;
			
			buf[ind + 0] = COLOUR_B(colour);
			buf[ind + 1] = COLOUR_G(colour);
			buf[ind + 2] = COLOUR_R(colour);
		}
	}
}

void draw_disc(uint8_t *buf, float x, float y, float radius, uint32_t colour)
{
	if (!buf || !colour)
		return;
	
	float aa_radius = radius + 1.0f;
	float radius_sq = radius * radius;
	float aa_radius_sq = aa_radius * aa_radius;
	int x_min = binary_max(0, floor_to_int(x - aa_radius));
	int x_max = binary_min(render_width - 1, ceil_to_int(x + aa_radius));
	int y_min = binary_max(0, floor_to_int(y - aa_radius));
	int y_max = binary_min(render_height - 1, ceil_to_int(y + aa_radius));
	
	for (int py = y_min; py <= y_max; py++)
	{
		float dy = py - y;
		float dy_sq = dy * dy;
		for (int px = x_min; px <= x_max; px++)
		{
			float dx = px - x;
			float dist_sq = dx * dx + dy_sq;
			if (dist_sq > aa_radius_sq)
				continue;
			
			float alpha = dist_sq <= radius_sq ? 1.0f : aa_radius - sqrtf(dist_sq);
			blend_pixel(buf, px, py, colour, alpha);
		}
	}
}

void draw_disc_with_alpha(uint8_t *buf, float x, float y, float radius, uint32_t colour, float opacity)
{
	if (!buf || !colour || opacity <= 0.0f)
		return;
	if (opacity > 1.0f)
		opacity = 1.0f;
	
	float aa_radius = radius + 1.0f;
	float radius_sq = radius * radius;
	float aa_radius_sq = aa_radius * aa_radius;
	int x_min = binary_max(0, floor_to_int(x - aa_radius));
	int x_max = binary_min(render_width - 1, ceil_to_int(x + aa_radius));
	int y_min = binary_max(0, floor_to_int(y - aa_radius));
	int y_max = binary_min(render_height - 1, ceil_to_int(y + aa_radius));
	
	for (int py = y_min; py <= y_max; py++)
	{
		float dy = py - y;
		float dy_sq = dy * dy;
		for (int px = x_min; px <= x_max; px++)
		{
			float dx = px - x;
			float dist_sq = dx * dx + dy_sq;
			if (dist_sq > aa_radius_sq)
				continue;
			
			float coverage = dist_sq <= radius_sq ? 1.0f : aa_radius - sqrtf(dist_sq);
			float alpha = coverage * opacity;
			int ind = (py * render_width + px) * 3;
			
			if (draw_alpha_buffer)
			{
				int ai = py * render_width + px;
				int a = (int)(alpha * 255.0f);
				buf[ind + 0] = COLOUR_B(colour);
				buf[ind + 1] = COLOUR_G(colour);
				buf[ind + 2] = COLOUR_R(colour);
				if (a > draw_alpha_buffer[ai])
					draw_alpha_buffer[ai] = (uint8_t)a;
			}
			else
				blend_pixel(buf, px, py, colour, alpha);
		}
	}
}

static inline float edge_function(wb_vec2 a, wb_vec2 b, float x, float y)
{
	return (x - a.x) * (b.y - a.y) - (y - a.y) * (b.x - a.x);
}

static void write_alpha_fill_pixel(uint8_t *buf, int x, int y, uint32_t colour, float alpha)
{
	if (!buf || alpha <= 0.0f)
		return;
	if (alpha > 1.0f)
		alpha = 1.0f;
	
	int ind = (y * render_width + x) * 3;
	if (draw_alpha_buffer)
	{
		int ai = y * render_width + x;
		int a = (int)(alpha * 255.0f);
		buf[ind + 0] = COLOUR_B(colour);
		buf[ind + 1] = COLOUR_G(colour);
		buf[ind + 2] = COLOUR_R(colour);
		if (a > draw_alpha_buffer[ai])
			draw_alpha_buffer[ai] = (uint8_t)a;
	}
	else
		blend_pixel(buf, x, y, colour, alpha);
}

void draw_triangle_with_alpha(uint8_t *buf, wb_vec2 a, wb_vec2 b, wb_vec2 c, uint32_t colour, float opacity)
{
	if (!buf || !colour || opacity <= 0.0f)
		return;
	if (opacity > 1.0f)
		opacity = 1.0f;
	
	float area = edge_function(a, b, c.x, c.y);
	if (BASICALLY_ZERO(area))
		return;
	
	int x_min = binary_max(0, floor_to_int(binary_min(binary_min(a.x, b.x), c.x) - 1.0f));
	int x_max = binary_min(render_width - 1, ceil_to_int(binary_max(binary_max(a.x, b.x), c.x) + 1.0f));
	int y_min = binary_max(0, floor_to_int(binary_min(binary_min(a.y, b.y), c.y) - 1.0f));
	int y_max = binary_min(render_height - 1, ceil_to_int(binary_max(binary_max(a.y, b.y), c.y) + 1.0f));
	float sign = area < 0.0f ? -1.0f : 1.0f;
	
	for (int py = y_min; py <= y_max; py++)
	{
		for (int px = x_min; px <= x_max; px++)
		{
			float x = (float)px + 0.5f;
			float y = (float)py + 0.5f;
			float e0 = edge_function(a, b, x, y) * sign;
			float e1 = edge_function(b, c, x, y) * sign;
			float e2 = edge_function(c, a, x, y) * sign;
			float min_edge = binary_min(binary_min(e0 / binary_max(1.0f, vec2_norm(vec2_diff(b, a))), e1 / binary_max(1.0f, vec2_norm(vec2_diff(c, b)))), e2 / binary_max(1.0f, vec2_norm(vec2_diff(a, c))));
			float coverage;
			
			if (min_edge < -1.0f)
				continue;
			coverage = min_edge >= 0.0f ? 1.0f : min_edge + 1.0f;
			write_alpha_fill_pixel(buf, px, py, colour, coverage * opacity);
		}
	}
}

void draw_polygon_with_alpha(uint8_t *buf, const wb_vec2 *points, int n_points, uint32_t colour, float opacity)
{
	float area = 0.0f;
	int x_min;
	int x_max;
	int y_min;
	int y_max;
	
	if (!buf || !points || n_points < 3 || !colour || opacity <= 0.0f)
		return;
	if (opacity > 1.0f)
		opacity = 1.0f;
	
	x_min = render_width - 1;
	y_min = render_height - 1;
	x_max = 0;
	y_max = 0;
	for (int i = 0; i < n_points; i++)
	{
		wb_vec2 a = points[i];
		wb_vec2 b = points[(i + 1) % n_points];
		area += a.x * b.y - b.x * a.y;
		x_min = binary_min(x_min, floor_to_int(a.x - 1.0f));
		x_max = binary_max(x_max, ceil_to_int(a.x + 1.0f));
		y_min = binary_min(y_min, floor_to_int(a.y - 1.0f));
		y_max = binary_max(y_max, ceil_to_int(a.y + 1.0f));
	}
	if (BASICALLY_ZERO(area))
		return;
	x_min = binary_max(0, x_min);
	y_min = binary_max(0, y_min);
	x_max = binary_min(render_width - 1, x_max);
	y_max = binary_min(render_height - 1, y_max);
	
	for (int py = y_min; py <= y_max; py++)
	{
		for (int px = x_min; px <= x_max; px++)
		{
			float x = (float)px + 0.5f;
			float y = (float)py + 0.5f;
			int inside = 0;
			float min_edge = 1e24f;
			
			for (int i = 0, j = n_points - 1; i < n_points; j = i++)
			{
				wb_vec2 a = points[j];
				wb_vec2 b = points[i];
				float edge_dist = fabsf(edge_function(a, b, x, y)) / binary_max(1.0f, vec2_norm(vec2_diff(b, a)));
				if (edge_dist < min_edge)
					min_edge = edge_dist;
				if (((a.y > y) != (b.y > y)) && (x < (b.x - a.x) * (y - a.y) / binary_max(1e-6f, b.y - a.y) + a.x))
					inside = !inside;
			}
			if (!inside && min_edge > 1.0f)
				continue;
			write_alpha_fill_pixel(buf, px, py, colour, (inside ? 1.0f : (1.0f - min_edge)) * opacity);
		}
	}
}

void draw_sausage(uint8_t *buf, wb_vec2 s, wb_vec2 d, float thickness, uint32_t colour)
{
	if (!buf)
		return;
	
	if (BASICALLY_EQUAL_VEC2(s, d))
	{
		draw_disc(buf, s.x, s.y, thickness, colour);
		return;
	}
	
	float dx = d.x - s.x;
	float dy = d.y - s.y;
	float len_sq = dx * dx + dy * dy;
	float aa_thickness = thickness + 1.0f;
	float thickness_sq = thickness * thickness;
	float aa_thickness_sq = aa_thickness * aa_thickness;
	int x_min = binary_max(0, floor_to_int(binary_min(s.x, d.x) - aa_thickness));
	int x_max = binary_min(render_width - 1, ceil_to_int(binary_max(s.x, d.x) + aa_thickness));
	int y_min = binary_max(0, floor_to_int(binary_min(s.y, d.y) - aa_thickness));
	int y_max = binary_min(render_height - 1, ceil_to_int(binary_max(s.y, d.y) + aa_thickness));
	
	for (int y = y_min; y <= y_max; y++)
	{
		float py = y - s.y;
		for (int x = x_min; x <= x_max; x++)
		{
			float px = x - s.x;
			float t = (px * dx + py * dy) / len_sq;
			t = clamp(t, 0.0f, 1.0f);
			float qx = px - t * dx;
			float qy = py - t * dy;
			float dist_sq = qx * qx + qy * qy;
			
			if (dist_sq > aa_thickness_sq)
				continue;
			
			float alpha = dist_sq <= thickness_sq ? 1.0f : aa_thickness - sqrtf(dist_sq);
			blend_pixel(buf, x, y, colour, alpha);
		}
	}
}

void draw_sequenced_sausage(uint8_t *buf, wb_vec2 s, wb_vec2 d, float thickness, uint32_t colour, int initial)
{
	if (!buf)
		return;
	
	if (BASICALLY_EQUAL_VEC2(s, d))
	{
		draw_disc(buf, s.x, s.y, thickness, colour);
		return;
	}
	
	float dx = d.x - s.x;
	float dy = d.y - s.y;
	float len_sq = dx * dx + dy * dy;
	float aa_thickness = thickness + 1.0f;
	float thickness_sq = thickness * thickness;
	float aa_thickness_sq = aa_thickness * aa_thickness;
	int x_min = binary_max(0, floor_to_int(binary_min(s.x, d.x) - aa_thickness));
	int x_max = binary_min(render_width - 1, ceil_to_int(binary_max(s.x, d.x) + aa_thickness));
	int y_min = binary_max(0, floor_to_int(binary_min(s.y, d.y) - aa_thickness));
	int y_max = binary_min(render_height - 1, ceil_to_int(binary_max(s.y, d.y) + aa_thickness));
	
	for (int y = y_min; y <= y_max; y++)
	{
		float py = y - s.y;
		for (int x = x_min; x <= x_max; x++)
		{
			float px = x - s.x;
			float t = (px * dx + py * dy) / len_sq;
			t = clamp(t, 0.0f, 1.0f);
			float qx = px - t * dx;
			float qy = py - t * dy;
			float dist_sq = qx * qx + qy * qy;
			
			if (dist_sq > aa_thickness_sq)
				continue;
			
			float alpha = dist_sq <= thickness_sq ? 1.0f : aa_thickness - sqrtf(dist_sq);
			if (!initial && t <= 0.0f && alpha < 1.0f)
				alpha = 0.0f;
			if (initial && t >= 1.0f && alpha < 1.0f)
				alpha = 0.0f;
			blend_pixel(buf, x, y, colour, alpha);
		}
	}
}


void draw_ppolyline(uint8_t *buf, wb_plane_polyline *pl)
{
	if (!buf || !pl)
		return;
	
	if (!pl->points || pl->n_points < 0)
		return;
	
	float length;
	wb_vec2 s = vec2(pl->points[0].x, pl->points[0].y);
	wb_vec2 d;
	
	float pos[2];
	float dir[2];
	
	for (int i = 0; i < pl->n_points - 1; i++)
	{
		d = vec2(pl->points[i + 1].x, pl->points[i + 1].y);
		draw_sausage(buf, s, d, pl->thickness, pl->colour);
		s = d;
	}
}

void draw_ppolyline_in_colour(uint8_t *buf, wb_plane_polyline *pl, uint32_t colour)
{
	if (!buf || !pl)
		return;
	
	uint32_t saved_colour = pl->colour;
	pl->colour = colour;
	draw_ppolyline(buf, pl);
	pl->colour = saved_colour;
}

void draw_plane_figure(uint8_t *buf, wb_plane_figure *fig)
{
	if (!buf || !fig)
		return;
	
	wb_plane_polyline *pl;
	
	for (int i = 0; i < fig->n_curves; i++)
	{
		if (!fig->curves[i]->nx || !fig->curves[i]->ny)
			continue;
		
		pl = nurbs_pcurve_to_ppolyline(fig->curves[i], N_SAMPLE_POINTS, wb_marker_thickness());
	
		if (!pl)
			continue;
		
		draw_ppolyline(buf, pl);
		
		free_plane_polyline(pl);
	}
}

void draw_plane_figure_in_colour(uint8_t *buf, wb_plane_figure *fig, uint32_t colour)
{
	if (!buf || !fig)
		return;
	
	wb_plane_polyline *pl;
	
	for (int i = 0; i < fig->n_curves; i++)
	{
		if (!fig->curves[i]->nx || !fig->curves[i]->ny)
			continue;
		
		pl = nurbs_pcurve_to_ppolyline(fig->curves[i], N_SAMPLE_POINTS, wb_marker_thickness());
	
		if (!pl)
			continue;
		
		draw_ppolyline_in_colour(buf, pl, colour);
		
		free_plane_polyline(pl);
	}
}

// Returns the largest x-value drawn, for kerning purposes
float draw_char(uint8_t *buf, char c, int x, int y, int height, uint32_t colour)
{
	if (!buf)
		return BIG_NEGATIVE_FLOAT;
	
	wb_plane_figure *fig = clone_plane_figure(get_letter(c));
	
	if (!fig)
		return BIG_NEGATIVE_FLOAT;
	
	scale_plane_figure(fig, height);
	translate_plane_figure(fig, x, y);
	jitter_plane_figure(fig, 1.5);
	
	draw_plane_figure_in_colour(buf, fig, colour);
	
	float max_x = pfigure_get_max_x_value(fig);
	
	free_plane_figure(fig);
	
	return max_x;
}

void draw_string(uint8_t *buf, char *str, int x, int y, int height, uint32_t colour)
{
	if (!buf || !str)
		return;
	
	float max_x;
	while (*str)
	{
		max_x = pfigure_get_max_x_value(get_letter(*str));
		
		draw_char(buf, *str, x, y, height, colour);
		
		if (max_x > 0)
			x += (max_x + LAZY_KERNING_PAD) * height;
		
		str++;
	}
}
