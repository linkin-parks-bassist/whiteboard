#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "whiteboard.h"

wb_loaded_video loaded_video;

static void scene_output_path(char *dst, size_t dst_size, const char *base_path, int scene, int n_scenes)
{
	char stem[256];
	char ext[64];
	const char *dot;
	
	if (!dst || dst_size == 0)
		return;
	
	if (!base_path || !*base_path)
	{
		snprintf(dst, dst_size, "scene_%02d.mp4", scene);
		return;
	}
	
	if (n_scenes <= 1)
	{
		snprintf(dst, dst_size, "%s", base_path);
		return;
	}
	
	dot = strrchr(base_path, '.');
	if (dot && dot > base_path)
	{
		size_t stem_len = (size_t)(dot - base_path);
		if (stem_len >= sizeof(stem))
			stem_len = sizeof(stem) - 1;
		memcpy(stem, base_path, stem_len);
		stem[stem_len] = 0;
		snprintf(ext, sizeof(ext), "%s", dot);
	}
	else
	{
		snprintf(stem, sizeof(stem), "%s", base_path);
		snprintf(ext, sizeof(ext), ".mp4");
	}
	
	snprintf(dst, dst_size, "%s_%02d%s", stem, scene, ext);
}

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
		char output_path[512];
		scene_output_path(output_path, sizeof(output_path), loaded_video.output_path, scene, loaded_video.n_scenes);
		#ifndef DEBUG
		printf("Rendering scene %d (%d frames) to %s\n", scene, n_frames, output_path);
		if (system("command -v ffmpeg >/dev/null 2>&1") != 0)
		{
			fprintf(stderr, "ffmpeg not found; cannot write %s\n", output_path);
			exit(1);
		}
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -y -f rawvideo -pix_fmt rgb24 -s %dx%d -r %d -i - "
                 "-c:v libx264 -preset fast -crf 18 %s",
                 WIDTH, HEIGHT, FPS, output_path);

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
			fprintf(stderr, "ffmpeg failed while writing %s, status=%d\n", output_path, status);
			exit(1);
		}
        #endif
    }

    free(frame);
	wb_free_loaded_video(&loaded_video);
    return 0;
}
