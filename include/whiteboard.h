#ifndef WHITEBOARD_MAIN_H_
#define WHITEBOARD_MAIN_H_

#define WIDTH 1920
#define HEIGHT 1080
#define FPS 60
#define JITTER_FPS 15
#define FRAMES_PER_SCENE (FPS * 4)
#define N_SCENES 1

#define BIG_NEGATIVE_FLOAT 			(-1e24f)
#define VERY_SMALL_FLOAT 			(1e-24f)
#define BASICALLY_ZERO(x)			(fabsf(x) < VERY_SMALL_FLOAT)
#define BASICALLY_EQUAL(x, y)		(BASICALLY_ZERO(fabsf(x - y)))
#define BASICALLY_ZERO_VEC2(v)		(BASICALLY_ZERO(v.x) && BASICALLY_ZERO(v.y))
#define BASICALLY_EQUAL_VEC2(v, w)	(BASICALLY_ZERO(v.x - w.x) && BASICALLY_ZERO(v.y - w.y))
#define BASICALLY_ZERO_VEC3(v)		(BASICALLY_ZERO(v.x) && BASICALLY_ZERO(v.y) && BASICALLY_ZERO(v.z))
#define BASICALLY_EQUAL_VEC3(v, w)	(BASICALLY_ZERO(v.x - w.x) && BASICALLY_ZERO(v.y - w.y) && BASICALLY_ZERO(v.z - w.z))

#define binary_min(x, y) ((x < y) ? x : y)
#define binary_max(x, y) ((x > y) ? x : y)

#define PI 	3.141592654
#define TAU 6.283185307

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "wb_linked_list.h"

#include "wb_vector.h"
#include "wb_nurbs.h"
#include "wb_figure.h"
#include "wb_letters.h"
#include "wb_symbols.h"
#include "wb_math.h"
#include "wb_scene.h"
#include "wb_spec.h"

#include "wb_draw.h"

#define sqr(x) ((x) * (x))

#endif
