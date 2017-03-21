# Acidwarp

## What is Acidwarp?

Acidwarp is an eye candy program which displays various patterns and
animates them by changing the palette. Originally it was an MS-DOS
program by Noah Spurrier. This is a port by Boris Gjenero using the
SDL 1.2 library. It is based on the Linux SVGALib port by Steven Wills.
This port can be built for Windows, Linux and other platforms.
Using [Emscripten](http://emscripten.org/), it can be built to run in
web browsers.

## Using the program

Use the following keys to control the program:

| Key      | Action |
|----------|--------|
| **Up**   | Rotate palette faster |
| **Down** | Rotate palette slower |
| **l**    | (L)ock: stay on current pattern, but keep changing palette |
| **k**    | switch to the next pallette |
| **p**    | (P)ause: totally stops pallette rotation and pattern changing. |
| **q**    | (Q)uit: causes Acidwarp to exit |
| **n**    | (N)ext pattern |

Double click on the window to toggle full screen mode.

The program takes some optional command line arguments.  To see all of
these, run: `acidwarp -h`.

Acidwarp originally worked in 320x200 256 colour VGA mode and generated
patterns using lookup tables to avoid slow floating point calculations.
This port defaults to using floating point for image generation. This
allows many patterns to be scaled up to high resolution. If you want
the original image generator, add the `-o` command line argument.

## Building the program

Build the program by running `make`. Version 1.2.x of the SDL library
is required, and detected via `sdl-config`. The icon requires
[ImageMagick](https://www.imagemagick.org) `convert` for resizing and
`xxd` for incorporating it in the program. Adding the icon to the
Windows executable also requires `icotool` from
[icoutils](http://www.nongnu.org/icoutils/) and
[`windres`](https://sourceware.org/binutils/docs/binutils/windres.html).

You can build for Windows from Cygwin. There `CC` defaults to
`i686-w64-mingw32-gcc`.

For building with Emscripten, use: `emmake make`

Acidwarp can now be built with SDL 2 using `make SDL=2`. Experimental hardware
accelerated palette cycling using SDL 2 and OpenGL ES 2.0 can be built with
`make GL=1`. This can also be built with Emscripten for use with WebGL 1.0
using `emmake make GL=1`.

## Further resources

For more information, see the [original README file](README).

Text for `acidwarp -h` and Warper projector instructions are
found in [warp_text.c](warp_text.c).

The original author's site about Acidwarp:
http://www.noah.org/acidwarp/
