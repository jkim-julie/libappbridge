This project contains:

libappbridge: A helper library to build applications with libhomescreen and libwindowmanager C++ shared library

AGL repo for source code:
https://github.com/jkim-julie/libappbridge.git

AGL repo for bitbake recipe:
//TODO: need to set the location.
//Thinking about the location, meta-agl-devel/meta-hmi-framework/recipes-demo-hmi/, same as runxdg.
//Now testing with the location above.

Quickstart:

Instructions for building libappbridge
---------------------------------------

The libappbridge is part of the
packagegroup-hmi-framework
packagegroup.

To build all the above, follow the instrucions on the AGL
source meta-agl/scripts/aglsetup.sh -m m3ulcb -b build agl-devel agl-netboot agl-appfw-smack agl-demo agl-localdev agl-audio-4a-framework agl-hmi-framework agl-demo-wam

bitbake agl-demo-platform-wam
