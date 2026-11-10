@echo off
set XYZ=D:\Universidad\Computacion Grafica\Laboratorio_Ball_Pivoting_Completo\Laboratorio_Ball_Pivoting_Completo\06_CPU_vs_CUDA\Stanford_Bunny_50000.xyz
for %%r in (1 2 3) do (
    .\bpa_cuda.exe "%XYZ%" 0.007 multi > resultados\Stanford_Bunny_50000_gpu_%%r.txt
    .\bpa_cpu.exe "%XYZ%" 0.007 multi precalculo > resultados\Stanford_Bunny_50000_cpu_precalculo_%%r.txt
)
.\bpa_cpu.exe "%XYZ%" 0.007 multi original > resultados\Stanford_Bunny_50000_cpu_original_1.txt
echo listo > resultados\fin_50000.txt
