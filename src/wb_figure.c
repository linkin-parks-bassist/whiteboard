#include "whiteboard.h"

IMPLEMENT_LINKED_PTR_LIST(wb_plane_figure);
IMPLEMENT_LINKED_PTR_LIST(wb_space_figure);

void scale_plane_figure(wb_plane_figure *fig, float factor)
{
	if (!fig)
		return;
	
	for (int i = 0; i < fig->n_curves; i++)
		scale_nurbs_pcurve(fig->curves[i], factor);
}

void translate_plane_figure(wb_plane_figure *fig, float x, float y)
{
	if (!fig)
		return;
	
	for (int i = 0; i < fig->n_curves; i++)
		translate_nurbs_pcurve(fig->curves[i], x, y);
}

wb_plane_figure *clone_plane_figure(wb_plane_figure *fig)
{
	if (!fig)
		return NULL;
	
	wb_plane_figure *result = malloc(sizeof(wb_plane_figure));
	
	if (!result)
		return NULL;
	
	result->n_curves = fig->n_curves;
	
	if (result->n_curves <= 0 || !fig->curves)
	{
		result->curves = NULL;
		return result;
	}
	
	result->curves = malloc(sizeof(wb_nurbs_pcurve*) * result->n_curves);
	
	if (!result->curves)
	{
		free(result);
		return NULL;
	}
	
	for (int i = 0; i < result->n_curves; i++)
		result->curves[i] = NULL;
	
	for (int i = 0; i < result->n_curves; i++)
	{
		result->curves[i] = clone_nurbs_pcurve(fig->curves[i]);
		
		if (!result->curves[i] && fig->curves[i])
		{
			for (int j = 0; j < i; j++)
				free_nurbs_pcurve(result->curves[j]);
			
			free(result->curves);
			free(result);
			
			return NULL;
		}
	}
	
	return result;
}

void free_plane_figure(wb_plane_figure *fig)
{
	if (!fig)
		return;
	
	if (!fig->curves)
	{
		free(fig);
		return;
	}
	
	for (int i = 0; i < fig->n_curves; i++)
		free_nurbs_pcurve(fig->curves[i]);
	
	free(fig->curves);
	free(fig);
}

void jitter_plane_figure(wb_plane_figure *fig, float jitter)
{
	if (!fig)
		return;
	
	if (!fig->curves)
		return;
	
	for (int i = 0; i < fig->n_curves; i++)
		jitter_nurbs_pcurve(fig->curves[i], jitter);
}

float pfigure_get_max_x_value(wb_plane_figure *fig)
{
	if (!fig)
		return BIG_NEGATIVE_FLOAT;
	
	if (!fig->curves)
		return BIG_NEGATIVE_FLOAT;
	
	float max_x = BIG_NEGATIVE_FLOAT;
	float local_max_x;
	
	for (int i = 0; i < fig->n_curves; i++)
	{
		local_max_x = nurbs_pcurve_get_max_x_value(fig->curves[i]);
		
		max_x = (local_max_x > max_x) ? local_max_x : max_x;
	}
	
	return max_x;
}
