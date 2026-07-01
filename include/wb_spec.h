#ifndef WB_SPEC_H_
#define WB_SPEC_H_

typedef struct
{
	wb_scene *scene;
	wb_scene **scenes;
	float *durations;
	int n_scenes;
	float duration;
	char error[256];
} wb_loaded_video;

wb_loaded_video wb_load_video_spec(const char *path);
void wb_free_loaded_video(wb_loaded_video *video);

#endif
