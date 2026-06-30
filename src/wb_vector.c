#include "whiteboard.h"

wb_vec2 vec2(float x, float y)
{
	return (wb_vec2){.x = x, .y = y};
}

wb_vec3 vec3(float x, float y, float z)
{
	return (wb_vec3){.x = x, .y = y, .z = z};
}

wb_quaternion quat(float x, float y, float z, float w)
{
	return (wb_quaternion){.a = x, .b = y, .c = z, .d = w};
}

float vec2_norm(wb_vec2 v)
{
	return sqrt(v.x * v.x + v.y * v.y);
}

float vec2_norm2(wb_vec2 v)
{
	return v.x * v.x + v.y * v.y;
}

wb_vec2 vec2_scale(float a, wb_vec2 v)
{
	return vec2(a * v.x, a * v.y);
}

float vec2_dist(wb_vec2 u, wb_vec2 v)
{
	return sqrt((u.x - v.x) * (u.x - v.x) + (u.y - v.y) * (u.y - v.y));
}

wb_vec2 vec2_sum(wb_vec2 u, wb_vec2 v)
{
	return vec2(u.x + v.x, u.y + v.y);
}

wb_vec2 vec2_diff(wb_vec2 u, wb_vec2 v)
{
	return vec2(u.x - v.x, u.y - v.y);
}

float vec2_dot(wb_vec2 u, wb_vec2 v)
{
	return u.x * v.x + u.y * v.y;
}

wb_vec2 vec2_normalised(wb_vec2 v)
{
	if (BASICALLY_ZERO(v.x) && BASICALLY_ZERO(v.y))
		return vec2(0, 0);
	
	float norm = vec2_norm(v);
	
	return vec2(v.x / norm, v.y / norm);
}

wb_vec2 vec2_perp(wb_vec2 v)
{
	return vec2(-v.y, v.x);
}

float vec3_norm(wb_vec3 v)
{
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

float vec3_norm2(wb_vec3 v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

wb_vec3 vec3_scale(float a, wb_vec3 v)
{
	return vec3(a * v.x, a * v.y, a * v.z);
}

float vec3_dist(wb_vec3 u, wb_vec3 v)
{
	return sqrt((u.x - v.x) * (u.x - v.x) + (u.y - v.y) * (u.y - v.y) + (u.z - v.z) * (u.z - v.z));
}

wb_vec3 vec3_sum(wb_vec3 u, wb_vec3 v)
{
	return vec3(u.x + v.x, u.y + v.y, u.z + v.z);
}

wb_vec3 vec3_diff(wb_vec3 u, wb_vec3 v)
{
	return vec3(u.x - v.x, u.y - v.y, u.z - v.z);
}

float vec3_dot(wb_vec3 u, wb_vec3 v)
{
	return u.x * v.x + u.y * v.y + u.z * v.z;
}

wb_vec3 vec3_normalised(wb_vec3 v)
{
	if (BASICALLY_ZERO(v.x) && BASICALLY_ZERO(v.y) && BASICALLY_ZERO(v.z))
		return vec3(0, 0, 0);
	
	float norm = vec3_norm(v);
	
	return vec3(v.x / norm, v.y / norm, v.z / norm);
}
