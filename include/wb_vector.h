#ifndef WB_VECTOR_H_
#define WB_VECTOR_H_

typedef struct
{
	float x, y;
} wb_vec2;

typedef struct
{
	float x, y, z;
} wb_vec3;

typedef struct
{
	float m11, m12, m13;
	float m21, m22, m23;
	float m31, m32, m33;
} wb_mat3;

typedef struct
{
	float m11, m12, m13, m14;
	float m21, m22, m23, m24;
	float m31, m32, m33, m34;
	float m41, m42, m43, m44;
} wb_mat4;

typedef struct
{
	float a, b, c, d;
} wb_quaternion;

wb_vec2 vec2(float x, float y);
wb_vec3 vec3(float x, float y, float z);
wb_quaternion quat(float x, float y, float z, float w);

float vec2_norm(wb_vec2 v);
float vec2_norm2(wb_vec2 v);
float vec2_dist(wb_vec2 u, wb_vec2 v);
float vec2_dot(wb_vec2 u, wb_vec2 v);
wb_vec2 vec2_scale(float a, wb_vec2 v);
wb_vec2 vec2_sum(wb_vec2 u, wb_vec2 v);
wb_vec2 vec2_diff(wb_vec2 u, wb_vec2 v);
wb_vec2 vec2_normalised(wb_vec2 v);
wb_vec2 vec2_perp(wb_vec2 v);

float vec3_norm(wb_vec3 v);
float vec3_dist(wb_vec3 u, wb_vec3 v);
float vec3_dot(wb_vec3 u, wb_vec3 v);
wb_vec3 vec3_scale(float a, wb_vec3 v);
wb_vec3 vec3_sum(wb_vec3 u, wb_vec3 v);
wb_vec3 vec3_diff(wb_vec3 u, wb_vec3 v);
wb_vec3 vec3_normalised(wb_vec3 v);

#endif
