/*
 * Camaglyph 0.1
 * Copyright (C) 2020 Cameron Kaiser.
 * All rights reserved
 * Distributed under the Floodgap Free Software License
 */

/* You can adjust these options if you are not using the Minoru3D camera. */
#define WIDTH  640
#define HEIGHT 480
#define FPS    30

/* Detect endianness.
   Works on gcc 4.0.1+. Clang? who knows? who cares? */
#if defined(__ORDER_BIG_ENDIAN__) && defined(__BYTE_ORDER__)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define CAMAG_BIG_ENDIAN 1
#endif
#else
#if (_BIG_ENDIAN)
#define CAMAG_BIG_ENDIAN 1
#elif (__BIG_ENDIAN__)
#define CAMAG_BIG_ENDIAN 1
#endif
#endif

#if CAMAG_BIG_ENDIAN
#warning big endian mode not tested, but should work
#endif
