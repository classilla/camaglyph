/*
 * Camaglyph 0.1
 * Copyright (C) 2020 Cameron Kaiser.
 * All rights reserved
 * Distributed under the Floodgap Free Software License
 */

#include "camag.h"
#include "render.h"

/* Utility functions that won't be converted to SIMD at some point. */

const char *rendermode_name(enum rendermode mode) {
	return
		(mode == OPTIMIZED_ANAGLYPH) 	? "Optimized anaglyph" :
		(mode == DUBOIS)		? "Dubois" :
		(mode == INTERLACE_RL)		? "Interlace (R-L)" :
		(mode == INTERLACE_LR)		? "Interlace (L-R)" :
		NULL /* caller go boom */
	;
};

