#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <algorithm>
#include <unordered_set>

using namespace std;

// Clase Nodo que representa un nodo en la matriz
class Nodo {
public:
    int x, y;    // Coordenadas del nodo
    double g, h, f;  // g: costo real, h: heurística, f: g + h
    Nodo* padre;  // Puntero al nodo padre

    Nodo(int x, int y, double g = 0, double h = 0, Nodo* padre = nullptr)
        : x(x), y(y), g(g), h(h), f(g + h), padre(padre) {}

    // Sobrecarga del operador < para usar la cola de prioridad
    bool operator<(const Nodo& other) const {
        return f > other.f;  // Prioridad inversa para la cola (menor f tiene mayor prioridad)
    }
};

// Clase AEstrella que implementa el algoritmo
class AEstrella {
private:
    int n;  // Tamaño de la matriz
    vector<vector<int>> grid;  // Matriz que representa el mapa (0 = libre, 1 = obstáculo)

    // Calcula la distancia Manhattan (heurística)
    double distancia_manhattan(const Nodo& a, const Nodo& b) {
        return abs(a.x - b.x) + abs(a.y - b.y);
    }

    // Obtiene los vecinos de un nodo
    vector<Nodo> obtener_vecinos(const Nodo& actual) {
        vector<Nodo> vecinos;
        vector<pair<int, int>> movimientos = { {0, 1}, {1, 0}, {0, -1}, {-1, 0} };  // Derecha, Abajo, Izquierda, Arriba

        for (auto& mov : movimientos) {
            int nuevo_x = actual.x + mov.first;
            int nuevo_y = actual.y + mov.second;
            if (nuevo_x >= 0 && nuevo_x < n && nuevo_y >= 0 && nuevo_y < n && grid[nuevo_x][nuevo_y] == 0) {
                vecinos.push_back(Nodo(nuevo_x, nuevo_y));
            }
        }
        return vecinos;
    }

    // Reconstruye la ruta óptima desde el nodo final
    vector<pair<int, int>> reconstruir_ruta(Nodo* nodo_final) {
        vector<pair<int, int>> ruta;
        Nodo* actual = nodo_final;
        while (actual != nullptr) {
            ruta.push_back({ actual->x, actual->y });
            actual = actual->padre;
        }
        reverse(ruta.begin(), ruta.end());
        return ruta;
    }

public:
    // Constructor que inicializa el tamaño de la matriz
    AEstrella(int tam) : n(tam) {
        grid.resize(n, vector<int>(n, 0));  // Inicializa la matriz n x n sin obstáculos
    }

    // Agregar un obstáculo en la posición (x, y)
    void agregar_obstaculo(int x, int y) {
        grid[x][y] = 1;
    }

    // Implementación del algoritmo A*
    vector<pair<int, int>> encontrar_ruta(pair<int, int> inicio, pair<int, int> fin) {
        Nodo* inicio_nodo = new Nodo(inicio.first, inicio.second);
        Nodo* fin_nodo = new Nodo(fin.first, fin.second);

        priority_queue<Nodo> open_set;  // Cola de prioridad para los nodos abiertos
        open_set.push(*inicio_nodo);

        unordered_set<int> cerrado;  // Conjunto de nodos cerrados
        vector<vector<double>> g_scores(n, vector<double>(n, INFINITY));  // Costos g
        g_scores[inicio_nodo->x][inicio_nodo->y] = 0;

        while (!open_set.empty()) {
            Nodo actual = open_set.top();
            open_set.pop();

            if (actual.x == fin_nodo->x && actual.y == fin_nodo->y) {
                return reconstruir_ruta(&actual);  // Si llegamos al nodo final
            }

            cerrado.insert(actual.x * n + actual.y);  // Marca el nodo como cerrado

            for (Nodo& vecino : obtener_vecinos(actual)) {
                int hash_vecino = vecino.x * n + vecino.y;
                if (cerrado.find(hash_vecino) != cerrado.end()) {
                    continue;  // Si el vecino ya está cerrado, lo saltamos
                }

                double g_nuevo = g_scores[actual.x][actual.y] + 1;  // Costo desde el inicio al vecino

                if (g_nuevo < g_scores[vecino.x][vecino.y]) {
                    vecino.g = g_nuevo;
                    vecino.h = distancia_manhattan(vecino, *fin_nodo);
                    vecino.f = vecino.g + vecino.h;
                    vecino.padre = new Nodo(actual);  // Actual como padre del vecino
                    g_scores[vecino.x][vecino.y] = g_nuevo;

                    open_set.push(vecino);  // Añadimos el vecino al conjunto abierto
                }
            }
        }
        return {};  // No se encontró ruta
    }

    // Imprime la matriz con la ruta
    void imprimir_matriz_con_ruta(const vector<pair<int, int>>& ruta) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (find(ruta.begin(), ruta.end(), make_pair(i, j)) != ruta.end()) {
                    cout << "R ";  // Ruta
                }
                else if (grid[i][j] == 1) {
                    cout << "X ";  // Obstáculo
                }
                else {
                    cout << "0 ";  // Espacio libre
                }
            }
            cout << endl;
        }
    }
};