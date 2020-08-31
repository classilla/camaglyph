# Camaglyph

Copyright (C) 2020 Cameron Kaiser. All rights reserved.  
Distributed under the Floodgap Free Software License.  
ckaiser@floodgap.com


Camaglyph is a tool for generating anaglyph images from twinned webcams,
like the Minoru 3D device. It supports generating optimized anaglyphs and
Dubois anaglyphs for standard red-cyan glasses, and can emit interlaced
images (L-R or R-L) for passive 3D monitors using alternating polarization.
It can also save PPM images to disk and can emit frames to a V4L2-compliant
virtual camera system like `akvcam` (a sample `akvcam.ini` is included) so that
you can use your webcam as, you know, a webcam.

SDL 1.2 and V4L2 are required for the Linux build (the only supported version
right now). It is tested on `ppc64le`, but should work on most platforms,
including 32-bit and big-endian. It runs entirely within userspace.

This tool is preconfigured to support the Minoru 3D webcam, which is two
640x480 UVC webcams in one unit. By default these appear at `/dev/video2`
(left) and `/dev/video0` (right) on Linux systems with V4L2.

## Build and usage instructions

1. Edit `camag.h` if you want different resolutions or frame rates. Virtually
   every recent webcam should offer 640x480 @30fps, which is how Camaglyph is
   configured out of the tin, so verify this works first. If you are not sure
   what your camera(s) support, use a command like

   `v4l2-ctl --list-formats-ext -d /dev/videoX`

   (where `/dev/videoX` is the device in question) to show configurations
   available. Camaglyph expects both cameras to use the same resolution and
   frame rate, whatever is specified.

2. Adjust the Makefile if necessary for your desired compiler and flags.
   `gcc` 4.0.1+ is supported.

3. `make`

4. Connect your camera(s). If your cameras appear at `/dev/video2` (left eye)
   and `/dev/video0` (right eye), which is the default for a Minoru 3D, you can
   just type

   `./camag`

   Otherwise, specify left and right, e.g.:

   `./camag /dev/video2 /dev/video0`

5. The monitor window will appear showing the anaglyph. While video is being
   displayed, you can press:

   A - change display algorithm (defaults to Optimized Anaglyph [faster];
       cycles through Optimized Anaglyph, Dubois [poorer reds but higher
       quality], Interlaced (R-L) and Interlaced (L-R)). The interlaced modes
       are intended for passive LCD 3D monitors. My Mitsubishi Diamondcrysta
       RDT233WX-3D is R-L interlaced.

   S - take a snapshot (to `outXXXXX.ppm` in the current directory; `XXXXX`
       is automatically incremented).
       Note that interlaced snapshots may need to be corrected manually or
       moved to the correct Y-position onscreen to match the 3D display's
       polarization order.

   Q - quit (or close the window)

6. If you have a V4L2-compliant virtual camera system like `akvcam` enabled,
   pass a third parameter for where the output should be emitted, e.g.:

   `./camag /dev/video2 /dev/video0 /dev/video4`

   If you use the provided `akvcam.ini`, then VLC, Firefox, etc., can view
   the provided capture stream (on my system, `/dev/video4` is the output and
   `/dev/video5` is what consumer applications connect to), which can be used
   for video chat and other features. Note that on slower systems this may
   reduce the frame rate somewhat, and interlaced modes may not display
   properly if they are not moved to the correct Y-position onscreen to match
   the 3D display's polarization order.

## To-do

1. Support SDL 2.0.

2. Support Mac OS X (probably PowerPC systems first because I'm that difficult,
   hence SDL 1.2 support) using QuickTime or QTKit. I've attempted to be
   modular so that other video backends can be swapped in relatively easily.

3. Automatic camera resolution detection (or specify it on the command line).

Send comments to ckaiser@floodgap.com.
