#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "whiteboard.h"

wb_loaded_video loaded_video;

typedef struct
{
	int enabled;
	int scene;
	float time_seconds;
	float video_time_seconds;
	char path[512];
} wb_snapshot_options;

typedef struct
{
	int scene_index;
	int scene_frame;
	int total_frame;
} wb_timeline_location;

static double wall_time_seconds(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

static void format_seconds_brief(double seconds, char *dst, size_t dst_size)
{
	int total_seconds;
	int hours;
	int minutes;
	int secs;
	
	if (!dst || dst_size == 0)
		return;
	
	if (seconds < 0.0)
		seconds = 0.0;
	total_seconds = (int)round(seconds);
	hours = total_seconds / 3600;
	minutes = (total_seconds % 3600) / 60;
	secs = total_seconds % 60;
	
	if (hours > 0)
		snprintf(dst, dst_size, "%dh%02dm%02ds", hours, minutes, secs);
	else if (minutes > 0)
		snprintf(dst, dst_size, "%dm%02ds", minutes, secs);
	else
		snprintf(dst, dst_size, "%ds", secs);
}

static void render_progress_line(int scene_index, int n_scenes, int scene_frame, int scene_total_frames, int total_frame, int total_frames, double started_at)
{
	char bar[33];
	char elapsed_buf[32];
	char eta_buf[32];
	double elapsed = wall_time_seconds() - started_at;
	double overall = total_frames > 0 ? (double)total_frame / (double)total_frames : 1.0;
	double eta = 0.0;
	int filled = (int)(overall * 32.0);
	
	if (overall > 0.0 && total_frame < total_frames)
		eta = elapsed * ((1.0 / overall) - 1.0);
	format_seconds_brief(elapsed, elapsed_buf, sizeof(elapsed_buf));
	format_seconds_brief(eta, eta_buf, sizeof(eta_buf));
	
	if (filled < 0)
		filled = 0;
	if (filled > 32)
		filled = 32;
	for (int i = 0; i < 32; i++)
		bar[i] = i < filled ? '#' : '-';
	bar[32] = 0;
	fprintf(stderr,
		"\r[%s] %5.1f%% scene %d/%d frame %d/%d total %d/%d elapsed %s eta %s",
		bar,
		overall * 100.0,
		scene_index + 1,
		n_scenes,
		scene_frame,
		scene_total_frames,
		total_frame,
		total_frames,
		elapsed_buf,
		eta_buf);
	fflush(stderr);
}

static int transition_overlap_frames(int scene)
{
	float duration;
	
	if (scene < 0 || scene + 1 >= loaded_video.n_scenes || !loaded_video.transitions)
		return 0;
	duration = loaded_video.transitions[scene].duration;
	if (duration <= 0.0f || loaded_video.transitions[scene].type == WB_TRANSITION_NONE)
		return 0;
	return binary_max(0, (int)roundf(binary_min(duration, binary_min(loaded_video.durations[scene], loaded_video.durations[scene + 1])) * FPS));
}

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

static void blend_frames(uint8_t *dst, const uint8_t *a, const uint8_t *b, float t)
{
	int n_bytes = WIDTH * HEIGHT * 3;
	
	if (!dst || !a || !b)
		return;
	if (t < 0.0f)
		t = 0.0f;
	if (t > 1.0f)
		t = 1.0f;
	
	for (int i = 0; i < n_bytes; i++)
	{
		float av = (float)a[i];
		float bv = (float)b[i];
		dst[i] = (uint8_t)(av + (bv - av) * t);
	}
}

static int total_output_frames(void)
{
	int total_frames = 0;
	
	for (int scene = 0; scene < loaded_video.n_scenes; scene++)
	{
		total_frames += (int)ceilf(loaded_video.durations[scene] * FPS);
		if (scene + 1 < loaded_video.n_scenes)
			total_frames -= transition_overlap_frames(scene);
	}
	
	return total_frames;
}

void render_scene(int scene, int t, uint8_t *buf)
{
	float time = (float)t / FPS;
	int jitter_frame = (int)(time * JITTER_FPS);
	
	fill_with_colour(buf, WB_DEFAULT_BACKGROUND_CENTER_COLOUR);
	
	wb_scene_render(loaded_video.scenes[scene], time, scene * 100000 + jitter_frame, buf);
}

static wb_timeline_location render_timeline_frame(int total_frame, uint8_t *frame, uint8_t *transition_frame, uint8_t *background_frame)
{
	wb_timeline_location loc;
	int total_frames = total_output_frames();
	int cursor = 0;
	
	memset(&loc, 0, sizeof(loc));
	loc.total_frame = total_frame;
	if (!frame)
		return loc;
	if (total_frames <= 0)
	{
		fill_with_colour(frame, WB_DEFAULT_BACKGROUND_CENTER_COLOUR);
		return loc;
	}
	if (total_frame < 0)
		total_frame = 0;
	if (total_frame >= total_frames)
		total_frame = total_frames - 1;
	loc.total_frame = total_frame;
	
	for (int scene = 0; scene < loaded_video.n_scenes; scene++)
	{
		int n_frames = (int)ceilf(loaded_video.durations[scene] * FPS);
		int incoming_overlap_frames = scene > 0 ? transition_overlap_frames(scene - 1) : 0;
		int overlap_frames = transition_overlap_frames(scene);
		int steady_start = incoming_overlap_frames;
		int steady_frames = n_frames - incoming_overlap_frames - overlap_frames;
		
		if (total_frame < cursor + steady_frames)
		{
			int scene_frame = steady_start + (total_frame - cursor);
			render_scene(scene, scene_frame, frame);
			loc.scene_index = scene;
			loc.scene_frame = scene_frame;
			return loc;
		}
		cursor += steady_frames;
		
		if (total_frame < cursor + overlap_frames)
		{
			int overlap_t = total_frame - cursor;
			float a = overlap_frames > 1 ? (float)(overlap_t + 1) / (float)(overlap_frames) : 1.0f;
			int scene_frame = n_frames - overlap_frames + overlap_t;
			
			render_scene(scene, scene_frame, frame);
			if (scene + 1 < loaded_video.n_scenes && transition_frame)
			{
				render_scene(scene + 1, overlap_t, transition_frame);
				if (loaded_video.transitions[scene].type == WB_TRANSITION_FADE)
				{
					if (a < 0.5f)
					{
						blend_frames(transition_frame, frame, background_frame, a * 2.0f);
						memcpy(frame, transition_frame, WIDTH * HEIGHT * 3);
					}
					else
						blend_frames(frame, background_frame, transition_frame, (a - 0.5f) * 2.0f);
				}
				else
					blend_frames(frame, frame, transition_frame, a);
			}
			loc.scene_index = scene;
			loc.scene_frame = scene_frame;
			return loc;
		}
		cursor += overlap_frames;
	}
	
	render_scene(loaded_video.n_scenes - 1, binary_max(0, (int)ceilf(loaded_video.durations[loaded_video.n_scenes - 1] * FPS) - 1), frame);
	loc.scene_index = loaded_video.n_scenes - 1;
	loc.scene_frame = binary_max(0, (int)ceilf(loaded_video.durations[loaded_video.n_scenes - 1] * FPS) - 1);
	return loc;
}

static int write_snapshot_ppm(const char *path, const uint8_t *buf)
{
	FILE *f;
	
	if (!path || !*path || !buf)
		return 0;
	
	f = fopen(path, "wb");
	if (!f)
		return 0;
	
	fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
	fwrite(buf, 1, WIDTH * HEIGHT * 3, f);
	fclose(f);
	return 1;
}

static void default_snapshot_path(char *dst, size_t dst_size, const char *base_path, int scene, int n_scenes)
{
	char scene_path[512];
	char stem[512];
	const char *dot;
	
	scene_output_path(scene_path, sizeof(scene_path), base_path, scene, n_scenes);
	dot = strrchr(scene_path, '.');
	if (dot && dot > scene_path)
	{
		size_t stem_len = (size_t)(dot - scene_path);
		if (stem_len >= sizeof(stem))
			stem_len = sizeof(stem) - 1;
		memcpy(stem, scene_path, stem_len);
		stem[stem_len] = 0;
		snprintf(dst, dst_size, "%s_snapshot.ppm", stem);
		return;
	}
	snprintf(dst, dst_size, "%s_snapshot.ppm", scene_path);
}

static void default_video_snapshot_path(char *dst, size_t dst_size, const char *base_path)
{
	char stem[512];
	const char *dot;
	
	if (!dst || dst_size == 0)
		return;
	if (!base_path || !*base_path)
	{
		snprintf(dst, dst_size, "video_snapshot.ppm");
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
		snprintf(dst, dst_size, "%s_snapshot.ppm", stem);
		return;
	}
	snprintf(dst, dst_size, "%s_snapshot.ppm", base_path);
}

int main(int argc, char **argv)
{
	wb_snapshot_options snapshot;
	const char *spec_path = NULL;
	int can_encode_video = 1;
	int total_frames = 0;
	int rendered_total_frames = 0;
	double render_started_at = 0.0;
	FILE *pipe = NULL;
	int argi;
	char output_path[512];

	srand(time(0));
    uint8_t *frame = (uint8_t*)malloc(WIDTH * HEIGHT * 3);
	uint8_t *transition_frame = (uint8_t*)malloc(WIDTH * HEIGHT * 3);
	uint8_t *background_frame = (uint8_t*)malloc(WIDTH * HEIGHT * 3);
	memset(&snapshot, 0, sizeof(snapshot));
	snapshot.scene = 0;
	snapshot.time_seconds = -1.0f;
	snapshot.video_time_seconds = -1.0f;

	set_render_dimensions(WIDTH, HEIGHT);

	if (!frame || !transition_frame || !background_frame)
	{
		fprintf(stderr, "could not allocate frame buffers\n");
		free(background_frame);
		free(transition_frame);
		free(frame);
		return 1;
	}
	fill_with_colour(background_frame, WB_DEFAULT_BACKGROUND_CENTER_COLOUR);
	
	for (argi = 1; argi < argc; argi++)
	{
		if (strcmp(argv[argi], "--snapshot") == 0)
		{
			snapshot.enabled = 1;
			if (argi + 1 < argc && argv[argi + 1][0] != '-')
				snprintf(snapshot.path, sizeof(snapshot.path), "%s", argv[++argi]);
		}
		else if (strcmp(argv[argi], "--snapshot-scene") == 0 && argi + 1 < argc)
			snapshot.scene = atoi(argv[++argi]);
		else if (strcmp(argv[argi], "--snapshot-time") == 0 && argi + 1 < argc)
			snapshot.time_seconds = strtof(argv[++argi], NULL);
		else if (strcmp(argv[argi], "--snapshot-video-time") == 0 && argi + 1 < argc)
			snapshot.video_time_seconds = strtof(argv[++argi], NULL);
		else if (!spec_path)
			spec_path = argv[argi];
	}
	
	if (spec_path)
	{
		loaded_video = wb_load_video_spec(spec_path);
		if (!loaded_video.scene)
		{
			fprintf(stderr, "Whiteboard spec error: %s\n", loaded_video.error);
			free(background_frame);
			free(transition_frame);
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
			free(background_frame);
			free(transition_frame);
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

	if (snapshot.enabled)
	{
		int snapshot_scene = snapshot.scene;
		int snapshot_frame;
		int snapshot_total_frame = 0;
		char snapshot_path[512];
		wb_timeline_location snapshot_loc;
		
		if (snapshot.video_time_seconds >= 0.0f)
		{
			int total_frames = total_output_frames();
			snapshot_total_frame = (int)roundf(snapshot.video_time_seconds * FPS);
			snapshot_total_frame = binary_max(0, binary_min(snapshot_total_frame, binary_max(0, total_frames - 1)));
			snapshot_loc = render_timeline_frame(snapshot_total_frame, frame, transition_frame, background_frame);
			snapshot_scene = snapshot_loc.scene_index;
			snapshot_frame = snapshot_loc.scene_frame;
		}
		else
		{
			if (snapshot_scene < 0)
				snapshot_scene = 0;
			if (snapshot_scene >= loaded_video.n_scenes)
				snapshot_scene = loaded_video.n_scenes - 1;
			if (snapshot.time_seconds < 0.0f)
				snapshot_frame = binary_max(0, (int)ceilf(loaded_video.durations[snapshot_scene] * FPS) - 1);
			else
				snapshot_frame = (int)roundf(snapshot.time_seconds * FPS);
			snapshot_frame = binary_max(0, binary_min(snapshot_frame, binary_max(0, (int)ceilf(loaded_video.durations[snapshot_scene] * FPS) - 1)));
			render_scene(snapshot_scene, snapshot_frame, frame);
			snapshot_total_frame = snapshot_frame;
		}
		if (snapshot.path[0])
			snprintf(snapshot_path, sizeof(snapshot_path), "%s", snapshot.path);
		else if (snapshot.video_time_seconds >= 0.0f)
			default_video_snapshot_path(snapshot_path, sizeof(snapshot_path), loaded_video.output_path);
		else
			default_snapshot_path(snapshot_path, sizeof(snapshot_path), loaded_video.output_path, snapshot_scene, loaded_video.n_scenes);
		if (!write_snapshot_ppm(snapshot_path, frame))
		{
			fprintf(stderr, "could not write snapshot %s\n", snapshot_path);
			free(background_frame);
			free(transition_frame);
			free(frame);
			wb_free_loaded_video(&loaded_video);
			return 1;
		}
		if (snapshot.video_time_seconds >= 0.0f)
			printf("Saved snapshot for video frame %d (scene %d frame %d) to %s\n", snapshot_total_frame, snapshot_scene, snapshot_frame, snapshot_path);
		else
			printf("Saved snapshot for scene %d frame %d to %s\n", snapshot_scene, snapshot_frame, snapshot_path);
	}

	#ifndef DEBUG
	can_encode_video = (system("command -v ffmpeg >/dev/null 2>&1") == 0);
	if (!can_encode_video)
	{
		if (snapshot.enabled)
		{
			fprintf(stderr, "ffmpeg not found; wrote snapshot and skipped video encoding\n");
			free(background_frame);
			free(transition_frame);
			free(frame);
			wb_free_loaded_video(&loaded_video);
			return 0;
		}
		else
		{
			fprintf(stderr, "ffmpeg not found; cannot write video output\n");
			free(background_frame);
			free(transition_frame);
			free(frame);
			wb_free_loaded_video(&loaded_video);
			return 1;
		}
	}
	#endif

	total_frames = total_output_frames();

	render_started_at = wall_time_seconds();

	scene_output_path(output_path, sizeof(output_path), loaded_video.output_path, 0, 1);
	#ifndef DEBUG
	if (can_encode_video)
	{
		char cmd[512];
		printf("Rendering video (%d scenes, %d frames) to %s\n", loaded_video.n_scenes, total_frames, output_path);
		snprintf(cmd, sizeof(cmd),
			"ffmpeg -hide_banner -loglevel error -nostats -y "
			"-f rawvideo -pix_fmt rgb24 -s %dx%d -r %d -i - "
			"-c:v libx264 -preset fast -crf 18 \"%s\"",
			WIDTH, HEIGHT, FPS, output_path);
		pipe = popen(cmd, "w");
		if (!pipe)
		{
			perror("popen");
			free(background_frame);
			free(transition_frame);
			free(frame);
			wb_free_loaded_video(&loaded_video);
			return 1;
		}
	}
	#endif

	for (int scene = 0; scene < loaded_video.n_scenes; scene++)
	{
		int n_frames = (int)ceilf(loaded_video.durations[scene] * FPS);
		int incoming_overlap_frames = scene > 0 ? transition_overlap_frames(scene - 1) : 0;
		int overlap_frames = transition_overlap_frames(scene);
		int steady_start = incoming_overlap_frames;
		int steady_frames = n_frames - incoming_overlap_frames - overlap_frames;
		
		for (int t = 0; t < steady_frames; t++)
		{
			int scene_frame = steady_start + t;
			render_scene(scene, scene_frame, frame);
			rendered_total_frames++;
			render_progress_line(scene, loaded_video.n_scenes, scene_frame + 1, n_frames, rendered_total_frames, total_frames, render_started_at);
			#ifndef DEBUG
			if (pipe)
				fwrite(frame, 1, WIDTH * HEIGHT * 3, pipe);
			#endif
		}
		
		for (int t = 0; t < overlap_frames; t++)
		{
			float a = overlap_frames > 1 ? (float)(t + 1) / (float)(overlap_frames) : 1.0f;
			int scene_frame = n_frames - overlap_frames + t;
			render_scene(scene, scene_frame, frame);
			render_scene(scene + 1, t, transition_frame);
			if (loaded_video.transitions[scene].type == WB_TRANSITION_FADE)
			{
				if (a < 0.5f)
				{
					blend_frames(transition_frame, frame, background_frame, a * 2.0f);
					memcpy(frame, transition_frame, WIDTH * HEIGHT * 3);
				}
				else
					blend_frames(frame, background_frame, transition_frame, (a - 0.5f) * 2.0f);
			}
			else
				blend_frames(frame, frame, transition_frame, a);
			rendered_total_frames++;
			render_progress_line(scene, loaded_video.n_scenes, scene_frame + 1, n_frames, rendered_total_frames, total_frames, render_started_at);
			#ifndef DEBUG
			if (pipe)
				fwrite(frame, 1, WIDTH * HEIGHT * 3, pipe);
			#endif
		}
	}
	#ifndef DEBUG
	if (pipe)
	{
		int status = pclose(pipe);
		if (status != 0)
		{
			fprintf(stderr, "ffmpeg failed while writing %s, status=%d\n", output_path, status);
			free(background_frame);
			free(transition_frame);
			free(frame);
			wb_free_loaded_video(&loaded_video);
			return 1;
		}
	}
	#endif

	if (total_frames > 0)
		fprintf(stderr, "\n");

	free(background_frame);
	free(transition_frame);
    free(frame);
	wb_free_loaded_video(&loaded_video);
    return 0;
}
