/*
 * Camaglyph 0.1
 * Copyright (C) 2020 Cameron Kaiser.
 * All rights reserved
 * Distributed under the Floodgap Free Software License
 */

#include "camag.h"
#include "render.h"

/* These are scalar routines. However, we try to use memcpy() a lot since
   most implementations are SIMD. Eventually we will convert the leftover
   parts to AltiVec, etc. */

#if CAMAG_BIG_ENDIAN
#define _R	(i)
#define _G	(i+1)
#define _B	(i+2)
#else
#define _R	(i+2)
#define _G	(i+1)
#define _B	(i)
#endif

void render(void *left, void *right, void *out,
            size_t blen, size_t width, int wypos, enum rendermode mode) {
	uint8_t *l8 = (uint8_t *)left;
	uint8_t *r8 = (uint8_t *)right;
	uint8_t *o8 = (uint8_t *)out;
	size_t i;

	if (mode == OPTIMIZED_ANAGLYPH) {
		/* pass blue and green from right, filter red channel
			and reconstruct from b+g. easiest done by just
			copying, and then fixing up R */
		memcpy(o8, r8, blen);
		for(i=0; i<blen; i+=3) {
			o8[_R] = (uint8_t)((l8[_B]*0.3)+(l8[_G]*0.7));
		}
	} else if (mode == DUBOIS) {
		float k;
		for(i=0; i<blen; i+=3) {
			/* saturated math required, blah */
			k = 
			((l8[_R]*-0.015)+(l8[_G]*-0.021)+(l8[_B]*-0.005)+
			 (r8[_R]*-0.072)+(r8[_G]*-0.113)+(r8[_B]* 1.226));
			o8[_B] = (k > 255.0) ? 255 : (k < 0.0) ? 0 : (uint8_t)k;
			k = 
			((l8[_R]*-0.040)+(l8[_G]*-0.038)+(l8[_B]*-0.016)+
			 (r8[_R]* 0.378)+(r8[_G]* 0.734)+(r8[_B]*-0.018));
			o8[_G] = (k > 255.0) ? 255 : (k < 0.0) ? 0 : (uint8_t)k;
			k = 
			((l8[_R]* 0.456)+(l8[_G]* 0.500)+(l8[_B]* 0.176)+
			 (r8[_R]*-0.043)+(r8[_G]*-0.088)+(r8[_B]*-0.002));
			o8[_R] = (k > 255.0) ? 255 : (k < 0.0) ? 0 : (uint8_t)k;
		}
	} else if (mode == INTERLACE_LR || mode == INTERLACE_RL) {
		size_t k = 3*width;
		int side = wypos & 1; /* 0 = L, 1 = R */
		if (mode == INTERLACE_RL)
			side ^= 1;
		/* copy the dominant side, then line in the rest */
		if (side) {
			memcpy(o8, r8, blen);
			for(i=k; i<blen; i+=(k+k)) {
				memcpy(&(o8[i]), &(l8[i]), k);
			}
		} else {
			memcpy(o8, l8, blen);
			for(i=k; i<blen; i+=(k+k)) {
				memcpy(&(o8[i]), &(r8[i]), k);
			}
		}
	} else {
		/* ??? profit! */
	}
}
