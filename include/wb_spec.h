#ifndef WB_SPEC_H_
#define WB_SPEC_H_

typedef struct
{
	wb_scene *scene;
	float duration;
	char error[256];
} wb_loaded_video;

wb_loaded_video wb_load_video_spec(const char *path);

#endif
