#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "whiteboard.h"

wb_loaded_video loaded_video;

void render_scene(int scene, int t, uint8_t *buf)
{
	float time = (float)t / FPS;
	int jitter_frame = (int)(time * JITTER_FPS);
	
	fill_with_colour(buf, 0xFFFFFF);
	
	wb_scene_render(loaded_video.scenes[scene], time, scene * 100000 + jitter_frame, buf);
}

int main(int argc, char **argv)
{
	srand(time(0));
    uint8_t *frame = (uint8_t*)malloc(WIDTH * HEIGHT * 3);

	set_render_dimensions(WIDTH, HEIGHT);
	
	if (argc > 1)
	{
		loaded_video = wb_load_video_spec(argv[1]);
		if (!loaded_video.scene)
		{
			fprintf(stderr, "Whiteboard spec error: %s\n", loaded_video.error);
			free(frame);
			return 1;
		}
	}
	else
	{
		loaded_video.scene = new_scene();
		loaded_video.scenes = malloc(sizeof(wb_scene*));
		loaded_video.durations = malloc(sizeof(float));
		if (!loaded_video.scene || !loaded_video.scenes || !loaded_video.durations)
		{
			fprintf(stderr, "could not allocate demo scene\n");
			if (loaded_video.scene)
				free_scene(loaded_video.scene);
			free(loaded_video.scenes);
			free(loaded_video.durations);
			memset(&loaded_video, 0, sizeof(loaded_video));
			free(frame);
			return 1;
		}
		loaded_video.scenes[0] = loaded_video.scene;
		loaded_video.durations[0] = (float)FRAMES_PER_SCENE / FPS;
		loaded_video.n_scenes = 1;
		loaded_video.duration = loaded_video.durations[0];
		int eq = wb_scene_add_math(loaded_video.scene, "$\\frac{1}{2}\\int \\mu(A)^{-1}\\chi_A+\\mu(B)^{-1}\\chi_B d\\mu$", 220, 540, 70, NICE_BLUE);
		wb_scene_move(loaded_video.scene, eq, 1.0f, 3.0f, 220, 540, 420, 420);
	}


    for (int scene = 0; scene < loaded_video.n_scenes; scene++)
    {
		int n_frames = (int)ceilf(loaded_video.durations[scene] * FPS);
		#ifndef DEBUG
		printf("Rendering scene %d (%d frames)\n", scene, n_frames);
		if (system("command -v ffmpeg >/dev/null 2>&1") != 0)
		{
			fprintf(stderr, "ffmpeg not found; cannot write scene_%02d.mp4\n", scene);
			exit(1);
		}
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -y -f rawvideo -pix_fmt rgb24 -s %dx%d -r %d -i - "
                 "-c:v libx264 -preset fast -crf 18 scene_%02d.mp4",
                 WIDTH, HEIGHT, FPS, scene);

        FILE *pipe = popen(cmd, "w");
        if (!pipe) { perror("popen"); exit(1); }
        #endif

        for (int t = 0; t < n_frames; t++)
        {
            render_scene(scene, t, frame);
            #ifndef DEBUG
            fwrite(frame, 1, WIDTH*HEIGHT*3, pipe);
            #endif
        }
        #ifndef DEBUG
        int status = pclose(pipe);
		if (status != 0)
		{
			fprintf(stderr, "ffmpeg failed while writing scene_%02d.mp4, status=%d\n", scene, status);
			exit(1);
		}
        #endif
    }

    free(frame);
	wb_free_loaded_video(&loaded_video);
    return 0;
}
