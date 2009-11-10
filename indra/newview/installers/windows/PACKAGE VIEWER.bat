copy /B /Y runtimes\dbghelp.dll ..\..\..\build-VC90\newview\dbghelp.dll
mkdir ..\..\..\build-VC90\newview\Universal
copy /B /Y runtimes\msvcr80.dll ..\..\..\build-VC90\newview\Universal\msvcr80.dll
copy /B /Y runtimes\msvcp80.dll ..\..\..\build-VC90\newview\Universal\msvcp80.dll
copy /B /Y runtimes\Microsoft.VC80.CRT.manifest ..\..\..\build-VC90\newview\Universal
cd packaged
python ../../../viewer_manifest.py --build ../../../../build-VC90/newview --dest .