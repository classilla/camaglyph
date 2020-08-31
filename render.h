/*
 * Camaglyph 0.1
 * Copyright (C) 2020 Cameron Kaiser.
 * All rights reserved
 * Distributed under the Floodgap Free Software License
 */

#include <unistd.h>
#include <string.h>
#include <inttypes.h>

enum rendermode {
	OPTIMIZED_ANAGLYPH,
	DUBOIS,
	INTERLACE_RL,
	INTERLACE_LR,
};

const char *rendermode_name(enum rendermode mode);

/* render() would contain SIMD variants, if implemented. */

void render(void *left,
            void *right,
            void *out,
            size_t blen,
            size_t width,
            int wypos,
            enum rendermode mode);
