#include "whiteboard.h"

float nurbs_f(wb_nurbs *nurbs, int i, int n, float u)
{
	if (!nurbs)
		return 0.0;
	
	if (i >= nurbs->n_knots || i + n >= nurbs->n_knots)
		return 0.0;
	
	if (fabsf(nurbs->knots[i + n] - nurbs->knots[i]) < 1e-6)
		return 0.0;
	
	return (u - nurbs->knots[i]) / (nurbs->knots[i + n] - nurbs->knots[i]);
}

float nurbs_g(wb_nurbs *nurbs, int i, int n, float u)
{
	return 1.0 - nurbs_f(nurbs, i + 1, n, u);
}

float nurbs_basis_function(wb_nurbs *nurbs, int i, int n, float u)
{
	if (!nurbs || n < 0 || i < 0)
		return 0.0;
	
	if (!nurbs->knots || nurbs->n_knots - 1 < i + 1)
		return 0.0;
	
	if (u < nurbs->knots[0] || nurbs->knots[nurbs->n_knots - 1] < u)
		return 0.0;
	
	if (n == 0)
		return (nurbs->knots[i] <= u && u <= nurbs->knots[i + 1]) ? 1.0 : 0.0;
	else
		return nurbs_f(nurbs, i, n, u) * nurbs_basis_function(nurbs, i, n - 1, u) + nurbs_g(nurbs, i, n, u) * nurbs_basis_function(nurbs, i + 1, n - 1, u);
}

float nurbs(wb_nurbs *nurbs, float u)
{
	float N = 0.0;
	float numer, denom;
	
	denom = 0.0;
	for (int j = 0; j < nurbs->n_control_points; j++)
	{
		denom += nurbs_basis_function(nurbs, j, nurbs->degree, u) * nurbs->weights[j];
	}
	
	for (int i = 0; i < nurbs->n_control_points; i++)
	{
		numer = nurbs_basis_function(nurbs, i, nurbs->degree, u) * nurbs->weights[i];
		
		N += nurbs->control_points[i] * numer / denom;
	}
	
	return N;
}

wb_nurbs *new_nurbs(int degree, int n_points)
{
	wb_nurbs *nurbs = malloc(sizeof(wb_nurbs));
	
	if (!nurbs)
		return NULL;
	
	nurbs->control_points = malloc(n_points * sizeof(float));
	
	if (!nurbs->control_points)
	{
		free(nurbs);
		return NULL;
	}
	
	nurbs->weights = malloc(n_points * sizeof(float));
	
	if (!nurbs->weights)
	{
		free(nurbs->control_points);
		free(nurbs);
		return NULL;
	}
	
	nurbs->knots = malloc((degree + n_points + 1) * sizeof(float));
	
	if (!nurbs->knots)
	{
		free(nurbs->weights);
		free(nurbs->control_points);
		free(nurbs);
		return NULL;
	}
	
	nurbs->degree = degree;
	nurbs->n_control_points = n_points;
	nurbs->n_knots = degree + n_points + 1;
	
	for (int i = 0; i < n_points; i++)
		nurbs->weights[i] = 1.0;
	
	int i = 0;
	
	while (i < degree)
		nurbs->knots[i++] = 0;
	
	while (i < n_points)
	{
		nurbs->knots[i] = i - degree;
		i++;
	}
	
	while (i < nurbs->n_knots)
		nurbs->knots[i++] = n_points - degree;
	
	return nurbs;
}

wb_nurbs *clone_nurbs(wb_nurbs *nurbs)
{
	if (!nurbs)
		return NULL;
	
	wb_nurbs *result = malloc(sizeof(wb_nurbs));
	
	if (!result)
		return NULL;
	
	result->n_control_points = nurbs->n_control_points;
	result->degree = nurbs->degree;
	result->n_knots = nurbs->n_knots;
	
	result->knots 	= NULL;
	result->weights = NULL;
	
	result->control_points = malloc(sizeof(float) * result->n_control_points);
	
	if (!result->control_points)
	{
		free(result);
		return NULL;
	}
	
	memcpy(result->control_points, nurbs->control_points, sizeof(float) * result->n_control_points);
	
	result->knots = malloc(sizeof(float) * result->n_knots);
	
	if (!result->knots)
	{
		free(result->control_points);
		free(result);
		return NULL;
	}
	
	memcpy(result->knots, nurbs->knots, sizeof(float) * result->n_knots);
	
	result->weights = malloc(sizeof(float) * result->n_control_points);
	
	if (!result->weights)
	{
		free(result->control_points);
		free(result->knots);
		free(result);
		return NULL;
	}
	
	memcpy(result->weights, nurbs->weights, sizeof(float) * result->n_control_points);
	
	return result;
}

void free_nurbs(wb_nurbs *nurbs)
{
	if (!nurbs)
		return;
	
	if (nurbs->knots)
		free(nurbs->knots);
	
	if (nurbs->control_points)
		free(nurbs->control_points);
	
	if (nurbs->weights)
		free(nurbs->weights);
	
	free(nurbs);
}

void scale_nurbs(wb_nurbs *nurbs, float factor)
{
	if (!nurbs)
		return;
	
	if (!nurbs->control_points)
		return;
	
	for (int i = 0; i < nurbs->n_control_points; i++)
		nurbs->control_points[i] *= factor;
}

void translate_nurbs(wb_nurbs *nurbs, float x)
{
	if (!nurbs)
		return;
	
	if (!nurbs->control_points)
		return;
	
	for (int i = 0; i < nurbs->n_control_points; i++)
		nurbs->control_points[i] += x;
}

void jitter_nurbs(wb_nurbs *nurbs, float jitter)
{
	if (!nurbs)
		return;
	
	if (!nurbs->control_points)
		return;
	
	float j;
	for (int i = 0; i < nurbs->n_control_points; i++)
	{
		j = (((float)(rand() % 2000) - 1000) / 1000.0) * jitter;
		nurbs->control_points[i] += j;
	}
}

float nurbs_get_max_control_point_value(wb_nurbs *nurbs)
{
	if (!nurbs)
		return BIG_NEGATIVE_FLOAT;
	
	float max_x = BIG_NEGATIVE_FLOAT;
	
	for (int i = 0; i < nurbs->n_control_points; i++)
		max_x = (nurbs->control_points[i] > max_x) ? nurbs->control_points[i] : max_x;
	
	return max_x;
}

void scale_nurbs_pcurve(wb_nurbs_pcurve *curve, float factor)
{
	if (!curve)
		return;
	
	scale_nurbs(curve->nx, factor);
	scale_nurbs(curve->ny, factor);
}

void translate_nurbs_pcurve(wb_nurbs_pcurve *curve, float x, float y)
{
	if (!curve)
		return;
	
	translate_nurbs(curve->nx, x);
	translate_nurbs(curve->ny, y);
}

wb_nurbs_pcurve *clone_nurbs_pcurve(wb_nurbs_pcurve *curve)
{
	if (!curve)
		return NULL;
	
	wb_nurbs_pcurve *result = malloc(sizeof(wb_nurbs_pcurve));
	
	if (!result)
		return NULL;
	
	result->nx = clone_nurbs(curve->nx);
	
	if (!result->nx)
	{
		free(result);
		return NULL;
	}
	
	result->ny = clone_nurbs(curve->ny);
	
	if (!result->ny)
	{
		free_nurbs(result->nx);
		free(result);
		return NULL;
	}
	
	return result;
}

void free_nurbs_pcurve(wb_nurbs_pcurve *curve)
{
	if (!curve)
		return;
	
	if (curve->nx)
		free_nurbs(curve->nx);
	
	if (curve->ny)
		free_nurbs(curve->ny);
	
	free(curve);
}


void jitter_nurbs_pcurve(wb_nurbs_pcurve *curve, float jitter)
{
	if (!curve)
		return;
	
	jitter_nurbs(curve->nx, jitter);
	jitter_nurbs(curve->ny, jitter);
}

// Actually returns the max control point value. Close enough lol
float nurbs_pcurve_get_max_x_value(wb_nurbs_pcurve *curve)
{
	if (!curve)
		return BIG_NEGATIVE_FLOAT;
	
	return nurbs_get_max_control_point_value(curve->nx);
}

wb_nurbs_pcurve *circle_nurbs_pcurve(float x, float y, float radius, int n_points, float start_angle)
{
	wb_nurbs_pcurve *result = malloc(sizeof(wb_nurbs_pcurve));
	
	if (!result || n_points <= 0)
		return NULL;
	
	result->nx = new_nurbs(3, n_points + 1);
	
	if (!result->nx)
	{
		free(result);
		return NULL;
	}
	
	result->ny = new_nurbs(3, n_points + 1);
	
	if (!result->ny)
	{
		free_nurbs(result->nx);
		free(result);
		return NULL;
	}
	
	float angle = start_angle;
	for (int i = 0; i < n_points + 1; i++)
	{
		result->nx->control_points[i] = x + radius * cos(angle);
		result->ny->control_points[i] = y + radius * sin(angle);
		
		angle += TAU / n_points;
	}
	
	return result;
}
