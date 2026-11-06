#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

const float PI = 3.14159265358979323846f;
const float EPS = 1e-12f;

struct Punto
{
    float x, y, z;
};

struct Triangulo
{
    int a, b, c;
};

struct AristaActiva
{
    int a, b;
    Punto centro;
};

struct ResultadoPivot
{
    int punto;
    Punto centro;
    float angulo;
    bool encontrado;
};

struct Resumen
{
    int regiones;
    int cubiertos;
    int triangulos;
    long long paresVecinos;
    bool precalculo;
};

const int MAX_VECINOS = 40;

vector<Punto> puntos;
vector<Triangulo> malla;
vector<int> inicioVecinos;
vector<int> listaVecinos;
vector<int> primerPar;

Punto sumar(const Punto& A, const Punto& B)
{
    return {A.x + B.x, A.y + B.y, A.z + B.z};
}

Punto restar(const Punto& A, const Punto& B)
{
    return {A.x - B.x, A.y - B.y, A.z - B.z};
}

Punto multiplicar(const Punto& A, float k)
{
    return {A.x * k, A.y * k, A.z * k};
}

float productoPunto(const Punto& A, const Punto& B)
{
    return A.x * B.x + A.y * B.y + A.z * B.z;
}

Punto productoCruz(const Punto& A, const Punto& B)
{
    return {A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x};
}

float norma(const Punto& A)
{
    return sqrt(productoPunto(A, A));
}

Punto normalizar(const Punto& A)
{
    float n = norma(A);
    if (n < EPS)
        return {0, 0, 0};
    return {A.x / n, A.y / n, A.z / n};
}

float distancia(const Punto& A, const Punto& B)
{
    return norma(restar(A, B));
}

bool cargarXYZ(const string& nombre)
{
    ifstream archivo(nombre.c_str());
    if (!archivo)
        return false;

    puntos.clear();
    string linea;
    while (getline(archivo, linea))
    {
        if (linea.empty() || linea[0] == '#')
            continue;
        stringstream ss(linea);
        Punto p;
        if (ss >> p.x >> p.y >> p.z)
            puntos.push_back(p);
    }
    return !puntos.empty();
}

bool guardarOBJ(const string& nombre)
{
    ofstream archivo(nombre.c_str());
    if (!archivo)
        return false;

    for (const Punto& p : puntos)
        archivo << "v " << p.x << " " << p.y << " " << p.z << "\n";
    for (const Triangulo& t : malla)
        archivo << "f " << t.a + 1 << " " << t.b + 1 << " " << t.c + 1 << "\n";
    return true;
}

bool centrosEsfera(const Punto& A, const Punto& B, const Punto& C, float radio, Punto& S1, Punto& S2)
{
    Punto u = restar(B, A);
    Punto v = restar(C, A);
    Punto w = productoCruz(u, v);

    float ww = productoPunto(w, w);
    if (ww < EPS)
        return false;

    float uu = productoPunto(u, u);
    float vv = productoPunto(v, v);
    Punto vxw = productoCruz(v, w);
    Punto wxu = productoCruz(w, u);

    Punto O;
    O.x = A.x + (uu * vxw.x + vv * wxu.x) / (2.0f * ww);
    O.y = A.y + (uu * vxw.y + vv * wxu.y) / (2.0f * ww);
    O.z = A.z + (uu * vxw.z + vv * wxu.z) / (2.0f * ww);

    float Rc = distancia(O, A);
    if (Rc > radio + EPS)
        return false;

    float h2 = radio * radio - Rc * Rc;
    if (h2 < 0.0f)
        h2 = 0.0f;
    float h = sqrt(h2);

    Punto n = normalizar(w);
    S1 = sumar(O, multiplicar(n, h));
    S2 = sumar(O, multiplicar(n, -h));
    return true;
}

bool esferaVacia(const Punto& centro, float radio, int ia, int ib, int ic)
{
    float limite = radio - 1e-6f;
    for (int k = inicioVecinos[ia]; k < inicioVecinos[ia + 1]; k++)
    {
        int i = listaVecinos[k];
        if (i == ia || i == ib || i == ic)
            continue;
        if (distancia(centro, puntos[i]) < limite)
            return false;
    }
    return true;
}

float anguloPivote(const Punto& A, const Punto& B, const Punto& centroActual, const Punto& centroNuevo)
{
    Punto eje = normalizar(restar(B, A));
    Punto M = multiplicar(sumar(A, B), 0.5f);

    Punto u = restar(centroActual, M);
    Punto v = restar(centroNuevo, M);
    u = restar(u, multiplicar(eje, productoPunto(u, eje)));
    v = restar(v, multiplicar(eje, productoPunto(v, eje)));

    float nu = norma(u);
    float nv = norma(v);
    if (nu < EPS || nv < EPS)
        return 1e30f;

    u = multiplicar(u, 1.0f / nu);
    v = multiplicar(v, 1.0f / nv);

    float angulo = atan2(productoPunto(eje, productoCruz(u, v)), productoPunto(u, v));
    if (angulo <= EPS)
        angulo += 2.0f * PI;
    return angulo;
}

pair<int, int> claveArista(int a, int b)
{
    if (a > b)
        swap(a, b);
    return {a, b};
}

tuple<int, int, int> claveTriangulo(int a, int b, int c)
{
    int v[3] = {a, b, c};
    sort(v, v + 3);
    return make_tuple(v[0], v[1], v[2]);
}

vector<pair<float, int> > vecinosCercanos(int i)
{
    vector<pair<float, int> > vecinos;
    for (int k = inicioVecinos[i]; k < inicioVecinos[i + 1]; k++)
    {
        int j = listaVecinos[k];
        vecinos.push_back({distancia(puntos[i], puntos[j]), j});
    }
    sort(vecinos.begin(), vecinos.end());
    if ((int)vecinos.size() > MAX_VECINOS)
        vecinos.resize(MAX_VECINOS);
    return vecinos;
}

void precalcularSemillasCPU(float radio)
{
    primerPar.assign(puntos.size(), -1);

    for (int i = 0; i < (int)puntos.size(); i++)
    {
        vector<pair<float, int> > vecinos = vecinosCercanos(i);
        bool encontrado = false;

        for (int a = 0; a < (int)vecinos.size() && !encontrado; a++)
        {
            for (int b = a + 1; b < (int)vecinos.size() && !encontrado; b++)
            {
                int j = vecinos[a].second;
                int k = vecinos[b].second;
                Punto S1, S2;
                if (!centrosEsfera(puntos[i], puntos[j], puntos[k], radio, S1, S2))
                    continue;
                if (esferaVacia(S1, radio, i, j, k) || esferaVacia(S2, radio, i, j, k))
                {
                    primerPar[i] = a * MAX_VECINOS + b;
                    encontrado = true;
                }
            }
        }
    }
}

bool buscarSemilla(float radio, const set<tuple<int, int, int> >& triangulosUsados,
                   const vector<bool>& puntoCubierto, Triangulo& semilla, Punto& centro)
{
    bool conPrecalculo = !primerPar.empty();

    for (int i = 0; i < (int)puntos.size(); i++)
    {
        if (puntoCubierto[i])
            continue;
        if (conPrecalculo && primerPar[i] < 0)
            continue;

        vector<pair<float, int> > vecinos = vecinosCercanos(i);

        for (int a = 0; a < (int)vecinos.size(); a++)
        {
            for (int b = a + 1; b < (int)vecinos.size(); b++)
            {
                if (conPrecalculo && a * MAX_VECINOS + b < primerPar[i])
                    continue;

                int j = vecinos[a].second;
                int k = vecinos[b].second;

                if (triangulosUsados.count(claveTriangulo(i, j, k)))
                    continue;

                Punto S1, S2;
                if (!centrosEsfera(puntos[i], puntos[j], puntos[k], radio, S1, S2))
                    continue;

                if (esferaVacia(S1, radio, i, j, k))
                {
                    semilla = {i, j, k};
                    centro = S1;
                    return true;
                }
                if (esferaVacia(S2, radio, i, j, k))
                {
                    semilla = {i, j, k};
                    centro = S2;
                    return true;
                }
            }
        }
    }
    return false;
}

ResultadoPivot buscarPuntoBPA(int a, int b, const Punto& centroActual, float radio,
                              const set<tuple<int, int, int> >& triangulosUsados)
{
    ResultadoPivot mejor;
    mejor.punto = -1;
    mejor.angulo = 1e30f;
    mejor.encontrado = false;

    const Punto& A = puntos[a];
    const Punto& B = puntos[b];

    for (int k = inicioVecinos[a]; k < inicioVecinos[a + 1]; k++)
    {
        int i = listaVecinos[k];
        if (i == a || i == b)
            continue;
        if (distancia(B, puntos[i]) > 2.0f * radio)
            continue;
        if (triangulosUsados.count(claveTriangulo(a, b, i)))
            continue;

        Punto S1, S2;
        if (!centrosEsfera(A, B, puntos[i], radio, S1, S2))
            continue;

        Punto centros[2] = {S1, S2};
        for (int c = 0; c < 2; c++)
        {
            if (!esferaVacia(centros[c], radio, a, b, i))
                continue;

            float angulo = anguloPivote(A, B, centroActual, centros[c]);
            if (angulo < mejor.angulo)
            {
                mejor.punto = i;
                mejor.centro = centros[c];
                mejor.angulo = angulo;
                mejor.encontrado = true;
            }
        }
    }
    return mejor;
}

Resumen pivotarMalla(float radio, bool multisemilla)
{
    malla.clear();

    set<tuple<int, int, int> > triangulosUsados;
    vector<bool> puntoCubierto(puntos.size(), false);
    int regiones = 0;

    const int MAX_TRIANGULOS = multisemilla ? max(15000, (int)puntos.size() * 3) : 15000;

    while ((int)malla.size() < MAX_TRIANGULOS)
    {
        if (!multisemilla && regiones == 1)
            break;

        Triangulo semilla;
        Punto centroSemilla;
        if (!buscarSemilla(radio, triangulosUsados, puntoCubierto, semilla, centroSemilla))
            break;

        regiones++;

        queue<AristaActiva> frontera;
        set<pair<int, int> > aristasCerradas;

        tuple<int, int, int> ks = claveTriangulo(semilla.a, semilla.b, semilla.c);
        if (triangulosUsados.count(ks))
            continue;

        malla.push_back(semilla);
        triangulosUsados.insert(ks);
        puntoCubierto[semilla.a] = true;
        puntoCubierto[semilla.b] = true;
        puntoCubierto[semilla.c] = true;

        frontera.push({semilla.a, semilla.b, centroSemilla});
        frontera.push({semilla.b, semilla.c, centroSemilla});
        frontera.push({semilla.c, semilla.a, centroSemilla});

        while (!frontera.empty() && (int)malla.size() < MAX_TRIANGULOS)
        {
            AristaActiva e = frontera.front();
            frontera.pop();

            pair<int, int> clave = claveArista(e.a, e.b);
            if (aristasCerradas.count(clave))
                continue;

            ResultadoPivot r = buscarPuntoBPA(e.a, e.b, e.centro, radio, triangulosUsados);
            if (!r.encontrado)
            {
                aristasCerradas.insert(clave);
                continue;
            }

            Triangulo t = {e.a, e.b, r.punto};
            tuple<int, int, int> kt = claveTriangulo(t.a, t.b, t.c);
            if (triangulosUsados.count(kt))
            {
                aristasCerradas.insert(clave);
                continue;
            }

            malla.push_back(t);
            triangulosUsados.insert(kt);
            puntoCubierto[t.a] = true;
            puntoCubierto[t.b] = true;
            puntoCubierto[t.c] = true;
            aristasCerradas.insert(clave);

            if (!aristasCerradas.count(claveArista(t.b, t.c)))
                frontera.push({t.b, t.c, r.centro});
            if (!aristasCerradas.count(claveArista(t.c, t.a)))
                frontera.push({t.c, t.a, r.centro});
        }
    }

    Resumen resumen;
    resumen.regiones = regiones;
    resumen.precalculo = !primerPar.empty();
    resumen.cubiertos = (int)count(puntoCubierto.begin(), puntoCubierto.end(), true);
    resumen.triangulos = (int)malla.size();
    resumen.paresVecinos = (long long)listaVecinos.size() / 2;
    return resumen;
}

void imprimirResultado(const string& version, const string& archivo, float radio, bool multisemilla,
                       const Resumen& resumen, double msVecindad, double msPrecalculo, double msPivoteo)
{
    printf("Version             : %s\n", version.c_str());
    printf("Archivo             : %s\n", archivo.substr(archivo.find_last_of("/\\") + 1).c_str());
    printf("Puntos              : %d\n", (int)puntos.size());
    printf("Radio               : %.3f\n", radio);
    printf("Modo                : %s\n", multisemilla ? "multi-semilla" : "una semilla");
    printf("Pares vecinos       : %lld\n", resumen.paresVecinos);
    printf("Regiones            : %d\n", resumen.regiones);
    printf("Puntos cubiertos    : %d / %d\n", resumen.cubiertos, (int)puntos.size());
    printf("Triangulos          : %d\n", resumen.triangulos);
    printf("Tiempo vecindad     : %10.1f ms\n", msVecindad);
    printf("Tiempo precalculo   : %10.1f ms\n", msPrecalculo);
    printf("Tiempo pivoteo      : %10.1f ms\n", msPivoteo);
    printf("Tiempo total        : %10.1f ms\n", msVecindad + msPrecalculo + msPivoteo);
}
