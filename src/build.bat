@echo off
if not defined DevEnvDir (
    call vcvarsall x64
)

set CompileFlags=-MTd -nologo -Z7 -Gm- -GR- -EHa- -Oi -Od -WX- -W4 -FC -wd4100 -wd4189 -wd4201 -wd4244 -wd4505 -DDEBUG=1 -DINTERNAL=1
set LinkFlags=-opt:ref -incremental:no winmm.lib user32.lib gdi32.lib 

if not exist ..\build mkdir ..\build

pushd ..\build

del *.pdb > NUL 2> NUL
cl %CompileFlags% ..\src\sonic_2d_game.cpp -LD -Fms2dg.map /link /pdb:s2dg_%random%.pdb -opt:ref -incremental:no -EXPORT:GameMainRenderAndUpdateLoop
cl %CompileFlags% ..\src\win32_game_engine.cpp -Fmwin32_s2dg.map /link %LinkFlags%

popd
