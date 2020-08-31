/*
 * Camaglyph 0.1
 * Copyright (C) 2020 Cameron Kaiser.
 * All rights reserved
 * Distributed under the Floodgap Free Software License
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "camag.h"
#include "render.h"
#include "capture.h"

/* XXX: allow this to also build with SDL 2 */
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
SDL_TimerID up;
SDL_Surface *screen;
SDL_SysWMinfo wminfo;

#ifdef __linux__
#include <X11/Xlib.h>
#endif

uint8_t *pixels;
int mode = 0;
size_t pixsize = HEIGHT * WIDTH * 3;
size_t maxtix  = 1000000 / FPS;
void *lefti, *righti;

void snapshot() {
	int fd;
	size_t i = 0;
	char n[16];
	struct stat statbuf;
	FILE *f;

	for(;;) {
		(void)sprintf(n, "out%05d.ppm", i++);
		fd = open(n, O_WRONLY | O_CREAT | O_EXCL, 432); // 0660
		if (fd > 0) {
			break;
		} else if (errno != EEXIST || i > 99999) {
			perror("while saving snapshot: open");
			return;
		}
	}
	/* not everything has dprintf() */
	f = fdopen(fd, "wb");
	if (!f) {
		perror("while saving snapshot: fdopen");
		return;
	}

	fprintf(f, "P6\n%d %d 255\n", WIDTH, HEIGHT);

#ifdef CAMAG_BIG_ENDIAN
	if (!fwrite((const void *)pixels, pixsize, 1, f)) {
		/* ppm is rgb and the frame buffer is rgb, so just write */
		perror("while saving snapshot: fwrite");
		(void)fclose(f);
		return;
	}
#else
	{
		/* ppm must be rgb, but the SDL frame buffer is bgr.
                   use lefti as a temporary buffer to flip */
		size_t i;
		uint8_t *nbuf = (uint8_t *)lefti;
		uint8_t *sbuf = (uint8_t *)pixels;

		/* XXX: SIMD */
		for (i=0; i<pixsize; i+=3) {
			nbuf[i]   = sbuf[i+2];
			nbuf[i+1] = sbuf[i+1];
			nbuf[i+2] = sbuf[i];
		}
		if (!fwrite((const void *)nbuf, pixsize, 1, f)) {
			perror("while saving snapshot: fwrite");
			(void)fclose(f);
			return;
		}
	}
#endif	

	if (fclose(f)) {
		perror("while saving snapshot: fclose");
		return;
	}

	/* flash the screen */
	SDL_LockSurface(screen);
	memset((void *)pixels, 255, pixsize);
	SDL_UnlockSurface(screen);
	SDL_Flip(screen);

	fprintf(stderr, "snapshot written to %s\n", n);
}

int main(int argc, char **argv) {
	char leftdev[] = "/dev/video2";
	char rightdev[] = "/dev/video0";
	char window_title[256];
	size_t fc = 0;
	SDL_Event event;
	int quit = 0;
	struct timeval tv1, tv2;
	long ticks;
	int ofd = -1;
	enum rendermode algo = OPTIMIZED_ANAGLYPH;
	int wxpos, wypos;
#ifdef __linux__
	Window w, junk;
	XWindowAttributes wa;
#endif

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Couldn't load SDL.\n");
		exit(1);
	}
	atexit(SDL_Quit);

	screen = SDL_SetVideoMode(WIDTH, HEIGHT, 24,
		/* SDL_NOFRAME | */
		SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (!screen) {
		fprintf(stderr, "couldn't open SDL window\n");
		exit(1);
	}
	(void)sprintf(window_title, "Camaglyph (%s)", rendermode_name(algo));
	SDL_WM_SetCaption(window_title, window_title);
	pixels = screen->pixels;
	if ((uintptr_t)pixels & 15) {
		printf("pixels is not SIMD aligned\n");
		exit(1);
	}
	lefti = malloc(pixsize);
	righti = malloc(pixsize);
	if (!lefti || !righti) {
		fprintf(stderr, "failed to malloc backbuffers\n");
		exit(1);
	}

	if (argc == 1) {
		fprintf(stderr, "using %s %s\n", leftdev, rightdev);
		if (init_capture((void *)leftdev, (void *)rightdev)) {
			fprintf(stderr, "failed to initialize cameras\n");
			exit(1);
		}
	} else if (argc == 2 || argc > 4) {
		fprintf(stderr, "usage: %s left right [output]\n", argv[0]);
		exit(1);
	} else {
		if (argc == 4) {
			/* open virtual video device */
			ofd = open(argv[3], O_WRONLY, 432);
			if (ofd == -1) {
				perror("open");
				exit(1);
			}
		}
		if (init_capture((void *)argv[1], (void *)argv[2])) {
			fprintf(stderr, "failed to initialize cameras\n");
			exit(1);
		}
	}

	atexit(uninit_capture);

	do {
		gettimeofday(&tv1, NULL);

		if(capture_frame(left, lefti) || capture_frame(right, righti)) {
			fprintf(stderr, "capture failed\n");
			exit(1);
		}

		SDL_LockSurface(screen);

		/* compute present absolute screen location of render buffer.
		   unfortunately, SDL 1.2.15 doesn't tell us this with
		   window move events. */
		SDL_GetWMInfo(&wminfo);
#ifdef __linux__
		/* screw Wayland */
		if (wminfo.info.x11.window) {
			Display *d = wminfo.info.x11.display;
			w = wminfo.info.x11.window;

			if (XGetWindowAttributes(d, w, &wa)) {
				(void)XTranslateCoordinates(d, w, wa.root,
					-wa.border_width, -wa.border_width,
					&wxpos, &wypos, &junk);
			} else {
				fprintf(stderr, "unable to interrogate X11 window\n");
				exit(1);
			}
		} else {
			fprintf(stderr, "warning: no X11 window (Wayland?)\n");
		}
#else
/* SDL_VIDEO_DRIVER_COCOA: NSWindow *window;
   SDL_VIDEO_DRIVER_WINDOW: HWND window; */
#warning no OS support for getting content position, interlacing may be wrong
		wypos = 0;
#endif
		
		render(lefti, righti, pixels, pixsize, WIDTH, wypos, algo);
		SDL_UnlockSurface(screen);
		SDL_Flip(screen);

		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT: {
					quit = 1;
					break;
				}
				case SDL_KEYDOWN: {
					if (event.key.keysym.sym == SDLK_q) {
						/* quit */
						quit = 1;
					} else
					if (event.key.keysym.sym == SDLK_a) {
						/* cycle through algorithms */
						algo =
			(algo == OPTIMIZED_ANAGLYPH)	? DUBOIS :
			(algo == DUBOIS)		? INTERLACE_RL :
			(algo == INTERLACE_RL)		? INTERLACE_LR :
						OPTIMIZED_ANAGLYPH;
						(void)sprintf(window_title,
			"Camaglyph (%s)", rendermode_name(algo));
						SDL_WM_SetCaption(window_title,
							window_title);
					} else
					if (event.key.keysym.sym == SDLK_s) {
						/* take a snapshot */
						snapshot();
					}
					break;
				}
			}
		}

		if (ofd != -1)
			(void)write(ofd, (const void *)pixels, pixsize);

		gettimeofday(&tv2, NULL);
		ticks = maxtix - (
			((tv2.tv_sec * 1000000) + tv2.tv_usec) -
			((tv1.tv_sec * 1000000) + tv1.tv_usec)
		);
		if (ticks > 0)
			usleep(ticks);
	} while(!quit);

	if (ofd != -1)
		close(ofd);
	free(lefti);
	free(righti);
	exit(0);
}
