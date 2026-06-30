#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "whiteboard.h"

wb_scene *demo_scene;
float video_duration = (float)FRAMES_PER_SCENE / FPS;

void render_scene(int scene, int t, uint8_t *buf)
{
	float time = (float)t / FPS;
	int jitter_frame = (int)(time * JITTER_FPS);
	
	fill_with_colour(buf, 0xFFFFFF);
	
	wb_scene_render(demo_scene, time, scene * 100000 + jitter_frame, buf);
}

int main(int argc, char **argv)
{
	srand(time(0));
    uint8_t *frame = (uint8_t*)malloc(WIDTH * HEIGHT * 3);

	set_render_dimensions(WIDTH, HEIGHT);
	
	if (argc > 1)
	{
		wb_loaded_video loaded = wb_load_video_spec(argv[1]);
		if (!loaded.scene)
		{
			fprintf(stderr, "Whiteboard spec error: %s\n", loaded.error);
			free(frame);
			return 1;
		}
		demo_scene = loaded.scene;
		video_duration = loaded.duration;
	}
	else
	{
		demo_scene = new_scene();
		int eq = wb_scene_add_math(demo_scene, "$\\frac{1}{2}\\int \\mu(A)^{-1}\\chi_A+\\mu(B)^{-1}\\chi_B d\\mu$", 220, 540, 70, NICE_BLUE);
		wb_scene_move(demo_scene, eq, 1.0f, 3.0f, 220, 540, 420, 420);
	}


    for (int scene = 0; scene < N_SCENES; scene++)
    {
		int n_frames = (int)ceilf(video_duration * FPS);
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
	free_scene(demo_scene);
    return 0;
}
