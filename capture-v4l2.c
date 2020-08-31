/*
 * Camaglyph 0.1
 * Copyright (C) 2020 Cameron Kaiser.
 * All rights reserved
 * Distributed under the Floodgap Free Software License
 */

#include "camag.h"
#include "capture.h"
#include "capture-v4l2.h"

/* This is the capture driver for V4L2. It is Linux-specific. */

int fdl = -1;
int fdr = -1;
size_t nbufs = 0;
struct vbuffer *lbufs, *rbufs;

/* 32 appears to be the maximum */
#define BUFFERS 4

#ifdef CAMAG_BIG_ENDIAN
#define PIXEL_FORMAT V4L2_PIX_FMT_RGB24
#else
#define PIXEL_FORMAT V4L2_PIX_FMT_BGR24
#endif

static int try_xioctl(int fd, int request, void *arg) {
	int r;

	do {
		r = v4l2_ioctl(fd, request, arg);
	} while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));
	if (r == -1) {
		perror("v4l2_ioctl");
		return 1;
	}
	return 0;
}

int init_capture(void *left, void *right) {
	struct v4l2_format fmt;
	struct v4l2_buffer buf;
	struct v4l2_requestbuffers req;
	enum v4l2_buf_type type;
	int i;

	fdl = -1;
	fdr = -1;
	fdl = v4l2_open((char *)left,  O_RDWR | O_NONBLOCK, 0);
	fdr = v4l2_open((char *)right, O_RDWR | O_NONBLOCK, 0);
	if (fdl < 0 || fdr < 0) {
		perror("v4l2_open");
		return 1;
	}

	BZERO(fmt);
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = WIDTH;
	fmt.fmt.pix.height      = HEIGHT;
	fmt.fmt.pix.pixelformat = PIXEL_FORMAT;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
	if (try_xioctl(fdl, VIDIOC_S_FMT, &fmt)) return 1;
	if ((fmt.fmt.pix.pixelformat != PIXEL_FORMAT) ||
	     (fmt.fmt.pix.width != WIDTH) || (fmt.fmt.pix.height != HEIGHT)) {
		printf("not expected format\n");
		return 1;
	}
	if (try_xioctl(fdr, VIDIOC_S_FMT, &fmt)) return 1;
	if ((fmt.fmt.pix.pixelformat != PIXEL_FORMAT) ||
	     (fmt.fmt.pix.width != WIDTH) || (fmt.fmt.pix.height != HEIGHT)) {
		printf("not expected format\n");
		return 1;
	}

	BZERO(req);
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.count  = BUFFERS;
	req.memory = V4L2_MEMORY_MMAP;
	if (try_xioctl(fdl, VIDIOC_REQBUFS, &req)) return 1;
	if (req.count < BUFFERS) {
		printf("couldn't get buffers\n");
		return 1;
	}
	nbufs = req.count;
	lbufs = calloc(req.count, sizeof(*lbufs));
	if (try_xioctl(fdr, VIDIOC_REQBUFS, &req)) return 1;
	if (req.count < nbufs) {
		printf("couldn't get buffers\n");
		return 1;
	}
	rbufs = calloc(req.count, sizeof(*lbufs));

	for(i=0; i<nbufs; i++) {
		BZERO(buf);
		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;
		if (try_xioctl(fdl, VIDIOC_QUERYBUF, &buf)) return 1;

		lbufs[i].length = buf.length;
		lbufs[i].start  = v4l2_mmap(NULL, buf.length,
		      PROT_READ | PROT_WRITE, MAP_SHARED,
		      fdl, buf.m.offset);

		if (lbufs[i].start == MAP_FAILED) {
			perror("mmap");
			return 1;
		}

		BZERO(buf);
		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;
		if (try_xioctl(fdr, VIDIOC_QUERYBUF, &buf)) return 1;

		rbufs[i].length = buf.length;
		rbufs[i].start  = v4l2_mmap(NULL, buf.length,
		      PROT_READ | PROT_WRITE, MAP_SHARED,
		      fdr, buf.m.offset);

		if (rbufs[i].start == MAP_FAILED) {
			perror("mmap");
			return 1;
		}
	}

	for(i=0; i<nbufs; i++) {
		BZERO(buf);
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (try_xioctl(fdl, VIDIOC_QBUF, &buf)) return 1;

		BZERO(buf);
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (try_xioctl(fdr, VIDIOC_QBUF, &buf)) return 1;
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (try_xioctl(fdl, VIDIOC_STREAMON, &type)) return 1;
	if (try_xioctl(fdr, VIDIOC_STREAMON, &type)) return 1;
	return 0;
}

int capture_frame(enum which camera, void *buffer) {
	int r, sfd = -1;
	struct vbuffer *sbufs;
	fd_set fds;
	struct timeval tv;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	if ((fdl < 0) || (fdr < 0)) {
		fprintf(stderr, "v4l2 not initialized\n");
		return 1;
	}
	sfd = (camera == left) ? fdl : fdr;
	sbufs = (camera == left) ? lbufs : rbufs;

	/* wait for the camera to become ready */
	do {
		FD_ZERO(&fds);
		FD_SET(sfd, &fds);

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		r = select(sfd + 1, &fds, NULL, NULL, &tv);
	} while ((r == -1 && (errno = EINTR)));
	if (r == -1) {
		perror("select");
		return errno;
	}

	/* wait for a filled buffer */
	BZERO(buf);
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (try_xioctl(sfd, VIDIOC_DQBUF, &buf)) return 1;

	/* copy to the workbuffer, return the buffer to the queue */
	memcpy(buffer, sbufs[buf.index].start, buf.bytesused);
	return (try_xioctl(sfd, VIDIOC_QBUF, &buf));
}

/* no sense in a return value since we're quitting anyway */
void uninit_capture() {
	int i;
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (try_xioctl(fdl, VIDIOC_STREAMOFF, &type)) return;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (try_xioctl(fdr, VIDIOC_STREAMOFF, &type)) return;
	for(i=0; i<nbufs; i++) {
		v4l2_munmap(lbufs[i].start, lbufs[i].length);
		v4l2_munmap(rbufs[i].start, rbufs[i].length);
	}
	v4l2_close(fdl); fdl = -1;
	v4l2_close(fdr); fdr = -1;

	nbufs = 0;
	return;
}
