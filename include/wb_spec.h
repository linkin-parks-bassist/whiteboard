#ifndef WB_SPEC_H_
#define WB_SPEC_H_

typedef struct
{
	int type;
	float duration;
} wb_scene_transition;

enum
{
	WB_TRANSITION_NONE = 0,
	WB_TRANSITION_FADE = 1,
	WB_TRANSITION_CROSSFADE = 2,
};

typedef struct
{
	wb_scene *scene;
	wb_scene **scenes;
	float *durations;
	wb_scene_transition *transitions;
	int n_scenes;
	float duration;
	char output_path[256];
	char error[256];
} wb_loaded_video;

wb_loaded_video wb_load_video_spec(const char *path);
void wb_free_loaded_video(wb_loaded_video *video);

#endif
