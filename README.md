webdraft-xyl
============

Webdraft is a tool that lets you assign point values to pokemon and draft 
pokemon teams. The code isn't really ready for prime-time yet, but it works
well enough. I have several upgrades in the pipeline, but this is a quick port
that adds sprites for X/Y and fills the available mons with the Kalos regional
dex. The name "webdraft" comes from the main version (for people drafting at 
home and not building point lists) being emscripten-ready, only depending on
source libraries and SDL3.


### Building

Windows:
- Run make.bat with clang and devenv on path, and SDL installed at w:/usr/local

Other systems:
- Not provided right now
- Should be a very straightforward unity build, just link to SDL3 and build main.c


