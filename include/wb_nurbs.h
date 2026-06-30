#ifndef WB_SPLINE_H_
#define WB_SPLINE_H_

typedef struct
{
	int degree;
	int n_control_points;
	int n_knots;
	float *knots;
	float *control_points;
	float *weights;
} wb_nurbs;

typedef struct
{
	wb_nurbs *nx;
	wb_nurbs *ny;
	
	uint32_t colour;
} wb_nurbs_pcurve;

typedef struct
{
	wb_nurbs *nx;
	wb_nurbs *ny;
	wb_nurbs *nz;
	
	uint32_t colour;
} wb_nurbs_scurve;

float nurbs(wb_nurbs *nurbs, float u);
float nurbs_basis_function(wb_nurbs *nurbs, int i, int n, float u);

wb_nurbs *new_nurbs(int degree, int n_points);
wb_nurbs *clone_nurbs(wb_nurbs *nurbs);
void free_nurbs(wb_nurbs *nurbs);

void scale_nurbs(wb_nurbs *nurbs, float factor);
void translate_nurbs(wb_nurbs *nurbs, float x);
void jitter_nurbs(wb_nurbs *nurbs, float jitter);

float nurbs_get_max_control_point_value(wb_nurbs *nurbs);

void scale_nurbs_pcurve(wb_nurbs_pcurve *curve, float factor);
void translate_nurbs_pcurve(wb_nurbs_pcurve *curve, float x, float y);
void jitter_nurbs_pcurve(wb_nurbs_pcurve *curve, float jitter);

float nurbs_pcurve_get_max_x_value(wb_nurbs_pcurve *curve);

wb_nurbs_pcurve *clone_nurbs_pcurve(wb_nurbs_pcurve *curve);
void free_nurbs_pcurve(wb_nurbs_pcurve *curve);

wb_nurbs_pcurve *circle_nurbs_pcurve(float x, float y, float radius, int n_points, float start_angle);

#endif
