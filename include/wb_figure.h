#ifndef WB_FIGURE_H_
#define WB_FIGURE_H_

typedef struct
{
	int n_curves;
	wb_nurbs_pcurve **curves;
} wb_plane_figure;

typedef struct
{
	int n_curves;
	wb_nurbs_scurve **curves;
} wb_space_figure;

DECLARE_LINKED_PTR_LIST(wb_plane_figure);
DECLARE_LINKED_PTR_LIST(wb_space_figure);

void scale_plane_figure(wb_plane_figure *fig, float factor);
void translate_plane_figure(wb_plane_figure *fig, float x, float y);
void free_plane_figure(wb_plane_figure *fig);

wb_plane_figure *clone_plane_figure(wb_plane_figure *fig);
void free_plane_figure(wb_plane_figure *fig);

void jitter_plane_figure(wb_plane_figure *fig, float jitter);

float pfigure_get_max_x_value(wb_plane_figure *fig);

#endif
