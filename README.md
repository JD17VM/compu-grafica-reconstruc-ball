# Ball Pivoting: CPU vs CUDA

Repositorio: https://github.com/JD17VM/compu-grafica-reconstruc-ball

## Archivos

| Archivo | Contenido |
|---|---|
| `bpa_comun.h` | Algoritmo Ball Pivoting (multi-semilla y una semilla) sin OpenGL, tomado de `03_Stanford_Bunny_BPA_MULTISEMILLA_CENTRADO.cpp`. La vecindad se guarda en formato CSR (`inicioVecinos`, `listaVecinos`). |
| `bpa_cpu.cpp` | Versión CPU. Vecindad O(n²) como en el programa original. Opcionalmente precalcula la semilla válida de cada punto en secuencial. |
| `bpa_cuda.cu` | Versión GPU. La vecindad y el precálculo de semillas se ejecutan en CUDA; el pivoteo de aristas sigue en CPU. |
| `compilar.bat` | Compila las dos versiones (MSVC + nvcc). |
| `medir_50000.bat` | Mediciones con la nube de 50 000 puntos. |
| `analizar_malla.py` | Cuenta aristas de borde y aristas con más de dos triángulos en un OBJ. |
| `graficos.py` | Lee `resultados/` y genera los gráficos de tiempos. |
| `opengl/` | Copias de los programas 05 y 06 con la medición de tiempo (`chrono`) que pide la guía. |
| `resultados/` | Salidas de las mediciones. |

## Compilar

Requiere Visual Studio Build Tools 2022 y CUDA Toolkit (probado con CUDA 13.5, GPU RTX 3050, `sm_86`).

```
.\compilar.bat
```

Programas OpenGL (MSYS2 UCRT64 + freeglut):

```
g++ -O2 opengl/03_Stanford_Bunny_BPA_MULTISEMILLA_CENTRADO_TIEMPO.cpp -o bpa_opengl.exe -lfreeglut -lglu32 -lopengl32
```

## Ejecutar

```
bpa_cpu.exe  Stanford_Bunny_20000.xyz 0.007 multi original
bpa_cpu.exe  Stanford_Bunny_20000.xyz 0.007 multi precalculo
bpa_cuda.exe Stanford_Bunny_20000.xyz 0.007 multi
```

El tercer argumento es `una` (una sola semilla, como el programa 05) o `multi` (multi-semilla, como el programa 06).
Se puede agregar un último argumento `salida.obj` para guardar la malla.

Las tres versiones generan exactamente la misma malla (se verificó comparando los OBJ byte a byte).
