#include "bpa_comun.h"
#include <chrono>
#include <cuda_runtime.h>

const int HILOS = 256;
#define EPS_GPU 1e-12f

#define VERIFICAR(llamada)                                                        \
    {                                                                             \
        cudaError_t error = llamada;                                              \
        if (error != cudaSuccess)                                                 \
        {                                                                         \
            printf("Error CUDA: %s (linea %d)\n", cudaGetErrorString(error), __LINE__); \
            exit(1);                                                              \
        }                                                                         \
    }

__device__ float distanciaGPU(Punto A, Punto B)
{
    float dx = A.x - B.x;
    float dy = A.y - B.y;
    float dz = A.z - B.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

__device__ Punto cruzGPU(Punto A, Punto B)
{
    return {A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x};
}

__device__ float puntoGPU(Punto A, Punto B)
{
    return A.x * B.x + A.y * B.y + A.z * B.z;
}

__device__ bool centrosEsferaGPU(Punto A, Punto B, Punto C, float radio, Punto& S1, Punto& S2)
{
    Punto u = {B.x - A.x, B.y - A.y, B.z - A.z};
    Punto v = {C.x - A.x, C.y - A.y, C.z - A.z};
    Punto w = cruzGPU(u, v);

    float ww = puntoGPU(w, w);
    if (ww < EPS_GPU)
        return false;

    float uu = puntoGPU(u, u);
    float vv = puntoGPU(v, v);
    Punto vxw = cruzGPU(v, w);
    Punto wxu = cruzGPU(w, u);

    Punto O;
    O.x = A.x + (uu * vxw.x + vv * wxu.x) / (2.0f * ww);
    O.y = A.y + (uu * vxw.y + vv * wxu.y) / (2.0f * ww);
    O.z = A.z + (uu * vxw.z + vv * wxu.z) / (2.0f * ww);

    float Rc = distanciaGPU(O, A);
    if (Rc > radio + EPS_GPU)
        return false;

    float h2 = radio * radio - Rc * Rc;
    if (h2 < 0.0f)
        h2 = 0.0f;
    float h = sqrtf(h2);

    float nw = sqrtf(ww);
    Punto n = nw < EPS_GPU ? Punto{0, 0, 0} : Punto{w.x / nw, w.y / nw, w.z / nw};
    S1 = {O.x + n.x * h, O.y + n.y * h, O.z + n.z * h};
    S2 = {O.x + n.x * -h, O.y + n.y * -h, O.z + n.z * -h};
    return true;
}

__device__ bool esferaVaciaGPU(Punto centro, float radio, int ia, int ib, int ic,
                               const Punto* puntosGPU, const int* inicioGPU, const int* listaGPU)
{
    float limite = radio - 1e-6f;
    for (int k = inicioGPU[ia]; k < inicioGPU[ia + 1]; k++)
    {
        int i = listaGPU[k];
        if (i == ia || i == ib || i == ic)
            continue;
        if (distanciaGPU(centro, puntosGPU[i]) < limite)
            return false;
    }
    return true;
}

__global__ void contarVecinos(const Punto* puntosGPU, int n, float limite, int* cantidad)
{
    __shared__ Punto bloque[HILOS];
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    Punto p = i < n ? puntosGPU[i] : Punto{0, 0, 0};
    int total = 0;

    for (int base = 0; base < n; base += HILOS)
    {
        if (base + threadIdx.x < n)
            bloque[threadIdx.x] = puntosGPU[base + threadIdx.x];
        __syncthreads();

        int tope = min(HILOS, n - base);
        for (int t = 0; t < tope; t++)
        {
            int j = base + t;
            if (i < n && j != i && distanciaGPU(p, bloque[t]) <= limite)
                total++;
        }
        __syncthreads();
    }

    if (i < n)
        cantidad[i] = total;
}

__global__ void llenarVecinos(const Punto* puntosGPU, int n, float limite, const int* inicioGPU, int* listaGPU)
{
    __shared__ Punto bloque[HILOS];
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    Punto p = i < n ? puntosGPU[i] : Punto{0, 0, 0};
    int posicion = i < n ? inicioGPU[i] : 0;

    for (int base = 0; base < n; base += HILOS)
    {
        if (base + threadIdx.x < n)
            bloque[threadIdx.x] = puntosGPU[base + threadIdx.x];
        __syncthreads();

        int tope = min(HILOS, n - base);
        for (int t = 0; t < tope; t++)
        {
            int j = base + t;
            if (i < n && j != i && distanciaGPU(p, bloque[t]) <= limite)
                listaGPU[posicion++] = j;
        }
        __syncthreads();
    }
}

__global__ void precalcularSemillas(const Punto* puntosGPU, int n, float radio,
                                    const int* inicioGPU, const int* listaGPU, int* primerParGPU)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n)
        return;

    float distancias[MAX_VECINOS];
    int indices[MAX_VECINOS];
    int cantidad = 0;
    Punto P = puntosGPU[i];

    for (int k = inicioGPU[i]; k < inicioGPU[i + 1]; k++)
    {
        int j = listaGPU[k];
        float d = distanciaGPU(P, puntosGPU[j]);

        if (cantidad == MAX_VECINOS &&
            (d > distancias[cantidad - 1] || (d == distancias[cantidad - 1] && j > indices[cantidad - 1])))
            continue;

        int pos = cantidad < MAX_VECINOS ? cantidad++ : MAX_VECINOS - 1;
        while (pos > 0 && (distancias[pos - 1] > d || (distancias[pos - 1] == d && indices[pos - 1] > j)))
        {
            distancias[pos] = distancias[pos - 1];
            indices[pos] = indices[pos - 1];
            pos--;
        }
        distancias[pos] = d;
        indices[pos] = j;
    }

    for (int a = 0; a < cantidad; a++)
    {
        for (int b = a + 1; b < cantidad; b++)
        {
            int j = indices[a];
            int k = indices[b];
            Punto S1, S2;
            if (!centrosEsferaGPU(P, puntosGPU[j], puntosGPU[k], radio, S1, S2))
                continue;
            if (esferaVaciaGPU(S1, radio, i, j, k, puntosGPU, inicioGPU, listaGPU) ||
                esferaVaciaGPU(S2, radio, i, j, k, puntosGPU, inicioGPU, listaGPU))
            {
                primerParGPU[i] = a * MAX_VECINOS + b;
                return;
            }
        }
    }
    primerParGPU[i] = -1;
}

double milisegundos(chrono::high_resolution_clock::time_point desde, chrono::high_resolution_clock::time_point hasta)
{
    return chrono::duration<double, milli>(hasta - desde).count();
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        cout << "Uso: bpa_cuda archivo.xyz radio una|multi [salida.obj]" << endl;
        return 1;
    }

    string archivo = argv[1];
    float radio = (float)atof(argv[2]);
    bool multisemilla = string(argv[3]) == "multi";

    if (!cargarXYZ(archivo))
    {
        cout << "No se pudo abrir: " << archivo << endl;
        return 1;
    }

    cudaDeviceProp propiedades;
    VERIFICAR(cudaGetDeviceProperties(&propiedades, 0));
    VERIFICAR(cudaFree(0));

    int n = (int)puntos.size();
    int bloques = (n + HILOS - 1) / HILOS;
    float limite = 2.0f * radio;

    auto t0 = chrono::high_resolution_clock::now();

    Punto* puntosGPU;
    int* cantidadGPU;
    int* inicioGPU;
    int* listaGPU;
    int* primerParGPU;

    VERIFICAR(cudaMalloc(&puntosGPU, n * sizeof(Punto)));
    VERIFICAR(cudaMalloc(&cantidadGPU, n * sizeof(int)));
    VERIFICAR(cudaMalloc(&inicioGPU, (n + 1) * sizeof(int)));
    VERIFICAR(cudaMemcpy(puntosGPU, puntos.data(), n * sizeof(Punto), cudaMemcpyHostToDevice));

    contarVecinos<<<bloques, HILOS>>>(puntosGPU, n, limite, cantidadGPU);
    VERIFICAR(cudaGetLastError());

    vector<int> cantidad(n);
    VERIFICAR(cudaMemcpy(cantidad.data(), cantidadGPU, n * sizeof(int), cudaMemcpyDeviceToHost));

    inicioVecinos.assign(n + 1, 0);
    for (int i = 0; i < n; i++)
        inicioVecinos[i + 1] = inicioVecinos[i] + cantidad[i];

    listaVecinos.resize(inicioVecinos[n]);
    VERIFICAR(cudaMalloc(&listaGPU, max(1, inicioVecinos[n]) * sizeof(int)));
    VERIFICAR(cudaMemcpy(inicioGPU, inicioVecinos.data(), (n + 1) * sizeof(int), cudaMemcpyHostToDevice));

    llenarVecinos<<<bloques, HILOS>>>(puntosGPU, n, limite, inicioGPU, listaGPU);
    VERIFICAR(cudaGetLastError());
    VERIFICAR(cudaMemcpy(listaVecinos.data(), listaGPU, inicioVecinos[n] * sizeof(int), cudaMemcpyDeviceToHost));

    auto t1 = chrono::high_resolution_clock::now();

    VERIFICAR(cudaMalloc(&primerParGPU, n * sizeof(int)));
    precalcularSemillas<<<bloques, HILOS>>>(puntosGPU, n, radio, inicioGPU, listaGPU, primerParGPU);
    VERIFICAR(cudaGetLastError());

    primerPar.resize(n);
    VERIFICAR(cudaMemcpy(primerPar.data(), primerParGPU, n * sizeof(int), cudaMemcpyDeviceToHost));

    auto t2 = chrono::high_resolution_clock::now();

    Resumen resumen = pivotarMalla(radio, multisemilla);

    auto t3 = chrono::high_resolution_clock::now();

    cudaFree(puntosGPU);
    cudaFree(cantidadGPU);
    cudaFree(inicioGPU);
    cudaFree(listaGPU);
    cudaFree(primerParGPU);

    printf("GPU                 : %s\n", propiedades.name);
    imprimirResultado("GPU (CUDA)", archivo, radio, multisemilla, resumen,
                      milisegundos(t0, t1), milisegundos(t1, t2), milisegundos(t2, t3));

    if (argc > 4)
        guardarOBJ(argv[4]);

    return 0;
}
