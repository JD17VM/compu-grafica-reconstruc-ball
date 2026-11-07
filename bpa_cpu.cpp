#include "bpa_comun.h"
#include <chrono>

void construirVecindadCPU(float radio)
{
    int n = (int)puntos.size();
    vector<vector<int> > vecinos(n);
    float limite = 2.0f * radio;

    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (distancia(puntos[i], puntos[j]) <= limite)
            {
                vecinos[i].push_back(j);
                vecinos[j].push_back(i);
            }
        }
    }

    inicioVecinos.assign(n + 1, 0);
    for (int i = 0; i < n; i++)
        inicioVecinos[i + 1] = inicioVecinos[i] + (int)vecinos[i].size();

    listaVecinos.resize(inicioVecinos[n]);
    for (int i = 0; i < n; i++)
        copy(vecinos[i].begin(), vecinos[i].end(), listaVecinos.begin() + inicioVecinos[i]);
}

double milisegundos(chrono::high_resolution_clock::time_point desde, chrono::high_resolution_clock::time_point hasta)
{
    return chrono::duration<double, milli>(hasta - desde).count();
}

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        cout << "Uso: bpa_cpu archivo.xyz radio una|multi original|precalculo [salida.obj]" << endl;
        return 1;
    }

    string archivo = argv[1];
    float radio = (float)atof(argv[2]);
    bool multisemilla = string(argv[3]) == "multi";
    bool conPrecalculo = string(argv[4]) == "precalculo";

    if (!cargarXYZ(archivo))
    {
        cout << "No se pudo abrir: " << archivo << endl;
        return 1;
    }

    auto t0 = chrono::high_resolution_clock::now();
    construirVecindadCPU(radio);
    auto t1 = chrono::high_resolution_clock::now();
    if (conPrecalculo)
        precalcularSemillasCPU(radio);
    auto t2 = chrono::high_resolution_clock::now();
    Resumen resumen = pivotarMalla(radio, multisemilla);
    auto t3 = chrono::high_resolution_clock::now();

    imprimirResultado(conPrecalculo ? "CPU con precalculo" : "CPU original", archivo, radio, multisemilla,
                      resumen, milisegundos(t0, t1), milisegundos(t1, t2), milisegundos(t2, t3));

    if (argc > 5)
        guardarOBJ(argv[5]);

    return 0;
}
