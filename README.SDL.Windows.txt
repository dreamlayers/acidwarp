Readme for SDL / Windows port of Acidwarp
-----------------------------------------

Acidwarp is a program by Noah Spurrier.  It was originally a DOS program.
This is an a port by Boris Gjenero (dreamlayers at yahoo dot ca), which 
uses the SDL library (http://www.libsdl.org) for video output.  It is 
based on the Linux SVGALib port by Steven Wills.  For more information,
see README.

This was tested on Windows, but thanks to SDL it should be possible to
build this for other operating systems.  The Windows specific code is
only used to work around the full screen mode issue noted below, and 
is only compiled if WIN32 is defined.

Quick start
-----------

Run acidwarp.exe

Use the following keys:
Up     causes the pallette to rotate faster.
Down   causes the pallette to rotate slower.
l      (L)ock on the current pattern.
k      switch to the next pallette.
p      (P)ause. Totally stops pallette rotation and pattern changing.
q      (Q)uit. Causes Acidwarp to exit.
n      (N)ext pattern.

Click on the window to toggle full screen mode

The program takes some optional command line arguments.  To see these, run:
acidwarp -h 

Note on full screen mode
------------------------

In Windows 7, it is normally impossible to access all 256 colours.  Merge
Acidwarp-256color.reg into the registry to fix this.

In Windows 2000 and XP, it is possible to use all 256 colours using the
windib video driver.

In earlier versions of Windows, all 256 colours may not be available via
the windib driver, and the directx driver may provide better results.
You can select it by setting the SDL_VIDEODRIVER environment variable to
"directx".
