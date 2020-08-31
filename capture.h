/*
 * Camaglyph 0.1
 * Copyright (C) 2020 Cameron Kaiser.
 * All rights reserved
 * Distributed under the Floodgap Free Software License
 */

enum which {
	left = 0,
	right,
};

int init_capture(void *left, void *right);
int capture_frame(enum which camera, void *buffer);
void uninit_capture();

