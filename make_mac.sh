#@echo off

#taskkill /IM webdraft.exe >nul 2>&1

# clang-cl -Xclang "-std=c23" -msse4.2 -Iw:/usr/local/include -g main.c /Fe"webdraft.exe" /link /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /LIBPATH:w:/usr/local/lib SDL3.lib
#clang-cl -fuse-ld=lld -Xclang "-std=c23" /MD /O2 /W3 -msse4.2 /Iw:\usr\local\include src/main.c %defines% /Fe"bin/webdraft-xyl.exe" /link /NOLOGO /INCREMENTAL:NO /LIBPATH:w:\usr\local\lib %ldprofile% /SUBSYSTEM:WINDOWS kernel32.lib user32.lib shell32.lib gdi32.lib winmm.lib imm32.lib advapi32.lib ole32.lib oleaut32.lib setupapi.lib version.lib SDL3-static.lib

source ~/.zshrc

clang -std=c23 -I/opt/homebrew/include -g src/main.c -o bin/webdraft -L/opt/homebrew/lib -lSDL3
