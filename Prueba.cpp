#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <atomic>
#include <termios.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include <random>
#include <mutex>

// Dimensiones fijas del terreno
const int ancho = 8; // Ancho del terreno
const int largo = 8; // Alto del terreno

int nivel = 1;       // Nivel actual
int puntaje = 0;     // Puntaje del jugador

int colorfondo = 0;  // Índice de colores del fondo
int colorsnake = 0;  // Índice de colores de la serpiente

// Colores de cada objeto para generación aleatoria de colores
int Fondo[5] = {51, 63, 184, 214, 177};
int Paredes[5] = {80, 97, 136, 130, 92};
int Manzana[3] = {9, 88, 197};
int Snake[3] = {28, 77, 84};
int CabezaSnake[3] = {10, 106, 65};

int velocidad = 400;          // Variable global para controlar la velocidad

// Matriz del terreno
std::vector<std::vector<int>> terreno; // 0 = camino, 1 = manzana, 2 = serpiente, 3 = pared, 5 = entrada, 6 = salida

std::atomic<bool> movimiento_completado(true);

std::mutex terreno_mutex; // Para proteger el acceso al terreno durante actualizaciones
std::mutex direccion_mutex; // Para proteger el acceso a la variable direccion

// Estructura para manejar lógica de matriz
struct Coordenada {

std::vector<Coordenada> serpiente;
Coordenada direccion = {0, 0};
std::vector<Coordenada> manzanas;

std::atomic<bool> game_over(false);

// Declaración de funciones
void configurarTerminal();
void restaurarTerminal();
void generarManzanas(int cantidad);
void iniciarTerreno();
void imprimirTerreno();
void actualizarTerreno();
void *moverSerpiente(void *arg);
void *manejarInput(void *arg);
void *hiloActualizarTerreno(void *arg);
void generarLaberinto();
void aumentarNivel();
void mostrarPuntaje();

// Configuración de la terminal
void configurarTerminal() {
    struct termios new_settings;
    tcgetattr(0, &new_settings);
    new_settings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &new_settings);
}

// Restaurar la configuración estándar de la terminal
void restaurarTerminal() {
    struct termios default_settings;
    tcgetattr(0, &default_settings);
    default_settings.c_lflag |= (ICANON | ECHO);
    tcsetattr(0, TCSANOW, &default_settings);
}

// Generar múltiples manzanas en posiciones aleatorias
void generarManzanas(int cantidad) {
    std::lock_guard<std::mutex> lock(terreno_mutex);
    manzanas.clear();

    for (int i = 0; i < cantidad; ++i) {
        int x, y;
        do {
            x = rand() % ancho;
            y = rand() % largo;
        } while (terreno[y][x] != 0);
        manzanas.push_back({x, y});
        terreno[y][x] = 1;
    }
}

// Generar el laberinto usando el Algoritmo de Prim Modificado
void generarLaberinto() {
    terreno = std::vector<std::vector<int>>(largo, std::vector<int>(ancho, 3));

    std::vector<std::vector<bool>> en_arbol(largo, std::vector<bool>(ancho, false));
    std::vector<Coordenada> paredes;

    int x = 1;
    int y = 1;
    en_arbol[y][x] = true;
    terreno[y][x] = 0;

    if (y > 0) paredes.push_back({x, y - 1});
    if (y < largo - 1) paredes.push_back({x, y + 1});
    if (x > 0) paredes.push_back({x - 1, y});
    if (x < ancho - 1) paredes.push_back({x + 1, y});

    while (!paredes.empty()) {
        int idx = rand() % paredes.size();
        Coordenada pared = paredes[idx];
        paredes.erase(paredes.begin() + idx);

        int px = pared.x;
        int py = pared.y;

        std::vector<Coordenada> vecinos;

        if (py > 0 && en_arbol[py - 1][px]) vecinos.push_back({px, py - 1});
        if (py < largo - 1 && en_arbol[py + 1][px]) vecinos.push_back({px, py + 1});
        if (px > 0 && en_arbol[py][px - 1]) vecinos.push_back({px - 1, py});
        if (px < ancho - 1 && en_arbol[py][px + 1]) vecinos.push_back({px + 1, py});

        if (vecinos.size() == 1) {
            terreno[py][px] = 0;
            en_arbol[py][px] = true;

            if (py > 0 && !en_arbol[py - 1][px]) paredes.push_back({px, py - 1});
            if (py < largo - 1 && !en_arbol[py + 1][px]) paredes.push_back({px, py + 1});
            if (px > 0 && !en_arbol[py][px - 1]) paredes.push_back({px - 1, py});
            if (px < ancho - 1 && !en_arbol[py][px + 1]) paredes.push_back({px + 1, py});
        }
    }

    terreno[1][0] = 5;                    // Entrada
    terreno[largo - 2][ancho - 1] = 6;    // Salida
    terreno[1][1] = 0;                    // Asegurar camino desde la entrada
    terreno[largo - 2][ancho - 2] = 0;    // Asegurar camino hacia la salida
}

// Inicializar el terreno y la serpiente
void iniciarTerreno() {
    colorfondo = rand() % 5;
    colorsnake = rand() % 3;

    generarLaberinto();

    serpiente.clear();
    serpiente.push_back({1, 0});
    direccion = {0, 0};

    generarManzanas(3);

    actualizarTerreno();
    imprimirTerreno();
}

// Mostrar el puntaje y nivel actual
void mostrarPuntaje() {
    std::cout << "Nivel: " << nivel << "  Puntaje: " << puntaje << std::endl;
}

// Imprimir el terreno en la terminal
void imprimirTerreno() {
    std::cout << "\033[2J\033[H";

    mostrarPuntaje();

    for (int y = 0; y < largo; y++) {
        for (int x = 0; x < ancho; x++) {

            if (serpiente[0].x == x && serpiente[0].y == y) {
                std::cout << "\033[48;5;" << CabezaSnake[colorsnake] << "m  \033[0m";
            } else if (terreno[y][x] == 2) {
                std::cout << "\033[48;5;" << Snake[colorsnake] << "m  \033[0m";
            } else if (terreno[y][x] == 1) {
                std::cout << "\033[48;5;" << Manzana[colorsnake] << "m  \033[0m";
            } else if (terreno[y][x] == 3) {
                std::cout << "\033[48;5;" << Paredes[colorfondo] << "m  \033[0m";
            } else if (terreno[y][x] == 5) {
                std::cout << "\033[48;5;220m  \033[0m";
            } else if (terreno[y][x] == 6) {
                std::cout << "\033[48;5;46m  \033[0m";
            } else {
                std::cout << "\033[48;5;" << Fondo[colorfondo] << "m  \033[0m";
            }
        }
        std::cout << std::endl;
    }
    std::cout.flush();
}

// Actualizar el terreno con la posición de la serpiente y las manzanas
void actualizarTerreno() {
    std::lock_guard<std::mutex> lock(terreno_mutex);

    for (int y = 0; y < largo; y++) {
        for (int x = 0; x < ancho; x++) {
            if (terreno[y][x] == 2) {
                terreno[y][x] = 0;
            }
        }
    }

    for (auto &segmento : serpiente) {
        terreno[segmento.y][segmento.x] = 2;
    }

    for (auto &manzana : manzanas) {
        terreno[manzana.y][manzana.x] = 1;
    }
}

// Mover la serpiente
void *moverSerpiente(void *arg) {
    while (!game_over) {
        if (direccion.x == 0 && direccion.y == 0) {
            movimiento_completado = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(velocidad));
            continue;
        }

        Coordenada nueva_cabeza;
        {
            std::lock_guard<std::mutex> lock(direccion_mutex);
            nueva_cabeza = {serpiente[0].x + direccion.x, serpiente[0].y + direccion.y};
        }

        if (nueva_cabeza.x < 0 || nueva_cabeza.x >= ancho || nueva_cabeza.y < 0 || nueva_cabeza.y >= largo) {
            game_over = true;
            pthread_exit(0);
        }

        if (terreno[nueva_cabeza.y][nueva_cabeza.x] == 3 || terreno[nueva_cabeza.y][nueva_cabeza.x] == 2) {
            game_over = true;
            pthread_exit(0);
        }

        if (terreno[nueva_cabeza.y][nueva_cabeza.x] == 6) {
            puntaje += 10;
            if (velocidad > 100) {
                velocidad -= 5;
            }
            nivel++;
            aumentarNivel();
            continue;
        }

        bool comioManzana = false;
        for (auto it = manzanas.begin(); it != manzanas.end(); ++it) {
            if (nueva_cabeza.x == it->x && nueva_cabeza.y == it->y) {
                comioManzana = true;
                manzanas.erase(it);
                puntaje += 1;
                break;
            }
        }

        if (comioManzana) {
            serpiente.push_back({-1, -1});
        }

        for (int i = serpiente.size() - 1; i > 0; i--) {
            serpiente[i] = serpiente[i - 1];
        }
        serpiente[0] = nueva_cabeza;

        movimiento_completado = true;

        actualizarTerreno();
        imprimirTerreno();
        std::this_thread::sleep_for(std::chrono::milliseconds(velocidad));
    }

    pthread_exit(0);
}

// Manejar el input del usuario
void *manejarInput(void *arg) {
    char tecla;
    while (!game_over) {
        ssize_t bytes_read = read(0, &tecla, 1);
        if (bytes_read > 0) {
            std::lock_guard<std::mutex> lock(direccion_mutex);
            if (movimiento_completado.load()) {
                movimiento_completado = false;

                switch (tecla) {
                case 'w':
                    if (direccion.y != 1) direccion = {0, -1};
                    break;
                case 's':
                    if (direccion.y != -1) direccion = {0, 1};
                    break;
                case 'a':
                    if (direccion.x != 1) direccion = {-1, 0};
                    break;
                case 'd':
                    if (direccion.x != -1) direccion = {1, 0};
                    break;
                }
            }
        }
    }
    pthread_exit(0);
}

// Actualizar el terreno (hilo separado)
void *hiloActualizarTerreno(void *arg) {
    while (!game_over) {
        actualizarTerreno();
        imprimirTerreno();
        std::this_thread::sleep_for(std::chrono::milliseconds(275));
    }
    pthread_exit(0);
}

// Función para aumentar el nivel y generar un nuevo laberinto
void aumentarNivel() {
    std::cout << "\033[2J\033[H";
    iniciarTerreno();
}

int main() {
    srand(time(0));

    std::cout << "\033[2J\033[H";

    iniciarTerreno();

    configurarTerminal();

    pthread_t hilo_movimiento, hilo_input, hilo_actualizacion;

    pthread_create(&hilo_movimiento, NULL, moverSerpiente, NULL);
    pthread_create(&hilo_input, NULL, manejarInput, NULL);
    pthread_create(&hilo_actualizacion, NULL, hiloActualizarTerreno, NULL);

    pthread_join(hilo_movimiento, NULL);
    pthread_join(hilo_input, NULL);
    pthread_join(hilo_actualizacion, NULL);

    restaurarTerminal();

    std::cout << "Juego terminado. Puntaje final: " << puntaje << std::endl;

    return 0;
}
