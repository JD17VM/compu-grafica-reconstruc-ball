@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" > nul
set "PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.5\bin;%PATH%"
cl /nologo /O2 /EHsc /std:c++17 bpa_cpu.cpp /Fe:bpa_cpu.exe
nvcc -O2 --fmad=false -arch=sm_86 -Xcompiler "/O2 /EHsc" -std=c++17 bpa_cuda.cu -o bpa_cuda.exe
