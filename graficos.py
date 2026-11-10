import glob
import os
import re
import statistics
import sys

import matplotlib.pyplot as plt

CARPETA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "resultados")
SALIDA = sys.argv[1] if len(sys.argv) > 1 else CARPETA
TAMANOS = [2503, 20000, 50000]
VERSIONES = [("cpu_original", "CPU original"), ("cpu_precalculo", "CPU con precálculo"), ("gpu", "GPU (CUDA)")]
COLORES = {"cpu_original": "#4C72B0", "cpu_precalculo": "#55A868", "gpu": "#C44E52"}


def leer(archivo):
    datos = {}
    for linea in open(archivo, encoding="utf-8", errors="replace"):
        if ":" not in linea:
            continue
        clave, valor = [x.strip() for x in linea.split(":", 1)]
        numero = re.match(r"-?[\d.]+", valor)
        datos[clave] = float(numero.group()) if numero else valor
    return datos


def mediciones():
    tabla = {}
    for n in TAMANOS:
        for version, _ in VERSIONES:
            archivos = sorted(glob.glob(os.path.join(CARPETA, f"Stanford_Bunny_{n}_{version}_*.txt")))
            ejecuciones = [leer(a) for a in archivos]
            ejecuciones = [e for e in ejecuciones if "Tiempo total" in e]
            if not ejecuciones:
                continue
            ejecuciones.sort(key=lambda e: e["Tiempo total"])
            mediana = ejecuciones[len(ejecuciones) // 2]
            mediana["repeticiones"] = len(ejecuciones)
            tabla[(n, version)] = mediana
    return tabla


def segundos_interrumpidos():
    archivos = glob.glob(os.path.join(CARPETA, "*cpu_original_INTERRUMPIDA.txt"))
    if not archivos:
        return None
    m = re.search(r"(\d+) min (\d+) s", open(archivos[0], encoding="utf-8").read())
    return int(m.group(1)) * 60 + int(m.group(2)) if m else None


def grafico_tiempos(tabla):
    plt.figure(figsize=(8, 4.8))
    for version, nombre in VERSIONES:
        xs = [n for n in TAMANOS if (n, version) in tabla]
        ys = [tabla[(n, version)]["Tiempo total"] / 1000 for n in xs]
        plt.plot(xs, ys, "o-", label=nombre, color=COLORES[version], linewidth=2)
        for x, y in zip(xs, ys):
            plt.annotate(f"{y:.2f} s", (x, y), textcoords="offset points", xytext=(6, 6), fontsize=8)
    cota = segundos_interrumpidos()
    if cota and (50000, "cpu_original") not in tabla:
        plt.plot([50000], [cota], "v", color=COLORES["cpu_original"], markersize=10)
        plt.annotate(f"> {cota:.0f} s (interrumpida)", (50000, cota), textcoords="offset points",
                     xytext=(-95, 8), fontsize=8, color=COLORES["cpu_original"])
    plt.xscale("log")
    plt.yscale("log")
    plt.xticks(TAMANOS, [f"{n:,}".replace(",", " ") for n in TAMANOS])
    plt.xlabel("Número de puntos")
    plt.ylabel("Tiempo total (s)")
    plt.title("Ball Pivoting multi-semilla, r = 0.007")
    plt.grid(True, which="both", alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(SALIDA, "grafico_tiempos.png"), dpi=150)
    plt.close()


def grafico_etapas(tabla):
    etapas = [("Tiempo vecindad", "Vecindad", "#8172B2"),
              ("Tiempo precalculo", "Precálculo semillas", "#CCB974"),
              ("Tiempo pivoteo", "Semillas + pivoteo", "#64B5CD")]
    fig, ejes = plt.subplots(1, 2, figsize=(11, 4.5))
    for eje, n in zip(ejes, [20000, 50000]):
        nombres = [nombre for version, nombre in VERSIONES if (n, version) in tabla]
        base = [0.0] * len(nombres)
        for clave, etiqueta, color in etapas:
            valores = [tabla[(n, version)][clave] / 1000 for version, _ in VERSIONES if (n, version) in tabla]
            eje.barh(nombres, valores, left=base, label=etiqueta, color=color)
            base = [b + v for b, v in zip(base, valores)]
        for i, total in enumerate(base):
            eje.text(total, i, f" {total:.1f} s", va="center", fontsize=9)
        titulo = f"{n:,} puntos".replace(",", " ")
        if (n, "cpu_original") not in tabla:
            titulo += "\n(CPU original no terminó: > 484 s, no se muestra)"
        eje.set_title(titulo)
        eje.set_xlabel("Tiempo (s)")
        eje.set_xlim(0, max(base) * 1.18)
        eje.invert_yaxis()
    ejes[0].legend(loc="lower right", fontsize=8)
    plt.tight_layout()
    plt.savefig(os.path.join(SALIDA, "grafico_etapas.png"), dpi=150)
    plt.close()


if __name__ == "__main__":
    tabla = mediciones()
    for (n, version), datos in sorted(tabla.items()):
        print(f"{n:>6} {version:<15} vecindad {datos['Tiempo vecindad']:>9.1f}  precalculo {datos['Tiempo precalculo']:>9.1f}"
              f"  pivoteo {datos['Tiempo pivoteo']:>9.1f}  total {datos['Tiempo total']:>9.1f}  triangulos {int(datos['Triangulos'])}"
              f"  repeticiones {datos['repeticiones']}")
    grafico_tiempos(tabla)
    grafico_etapas(tabla)
