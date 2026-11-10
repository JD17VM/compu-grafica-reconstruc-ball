import sys
from collections import Counter


def leer_obj(nombre):
    vertices = 0
    caras = []
    for linea in open(nombre):
        if linea.startswith("v "):
            vertices += 1
        elif linea.startswith("f "):
            caras.append(tuple(int(x.split("/")[0]) - 1 for x in linea.split()[1:4]))
    return vertices, caras


def analizar(nombre):
    vertices, caras = leer_obj(nombre)
    aristas = Counter()
    for a, b, c in caras:
        for u, v in ((a, b), (b, c), (c, a)):
            aristas[(min(u, v), max(u, v))] += 1

    usados = len({v for cara in caras for v in cara})
    borde = sum(1 for n in aristas.values() if n == 1)
    variedad = sum(1 for n in aristas.values() if n == 2)
    no_variedad = sum(1 for n in aristas.values() if n > 2)

    print(f"Archivo                  : {nombre}")
    print(f"Triangulos               : {len(caras)}")
    print(f"Vertices usados          : {usados} / {vertices}")
    print(f"Aristas                  : {len(aristas)}")
    print(f"Aristas de borde (1 tri) : {borde}")
    print(f"Aristas con 2 triangulos : {variedad}")
    print(f"Aristas con >2 triangulos: {no_variedad}")
    print()


for archivo in sys.argv[1:]:
    analizar(archivo)
