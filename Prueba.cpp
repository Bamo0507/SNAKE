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
#include <queue>
#include <signal.h>

// Dimensiones fijas del terreno
const int ancho_inicial = 8; // Ancho del terreno
const int largo_inicial = 8; // Alto del terreno
const int ancho = ancho_inicial; // Ancho actual del terreno
const int largo = largo_inicial; // Alto actual del terreno

int nivel = 1;  // Nivel actual
int puntaje = 0; // Puntaje del jugador

int colorfondo = 0; // Índice de colores del fondo en un array
int colorsnake = 0; // Índice de colores de la serpiente en un array

// Colores de cada objeto para generación aleatoria de colores
int Fondo[5] = {51, 63, 184, 214, 177};
int Paredes[5] = {80, 97, 136, 130, 92};
int Manzana[3] = {9, 88, 197};
int Snake[3] = {28, 77, 84};
int CabezaSnake[3] = {10, 106, 65};

int largoDefaultContador = 0; // Contador para controlar cuando la serpiente ya haya crecido a 3
int velocidad = 400; // Variable global para controlar la velocidad

// Matriz del terreno
std::vector<std::vector<int>> terreno; // 0 = camino, 1 = manzana, 2 = serpiente, 3 = pared, 5 = entrada, 6 = salida

std::atomic<bool> movimiento_completado(true);

std::mutex terreno_mutex;    // Para proteger el acceso al terreno durante actualizaciones
std::mutex direccion_mutex;  // Para proteger el acceso a 'direccion'

// Estructura para manejar lógica de matriz
struct Coordenada {
    int x, y;
};

// Lógica Posicional Inicial
std::vector<Coordenada> serpiente;
Coordenada direccion = {0, 0}; // La serpiente no se mueve hasta que el jugador ingrese una dirección
std::vector<Coordenada> manzanas; // Lista de manzanas

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

void generarLaberinto(int width, int height);
void tallarLaberinto(int x, int y);
int verificarTerreno();

void aumentarNivel();
void mostrarPuntaje();

void manejarSenal(int senal);

// Configuración de la terminal
void configurarTerminal() {
    struct termios new_settings;
    tcgetattr(0, &new_settings);
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 1; // Espera hasta 0.1 segundos para entrada
    tcsetattr(0, TCSANOW, &new_settings);
}

// Restaurar la configuración estándar de la terminal
void restaurarTerminal() {
    struct termios default_settings;
    tcgetattr(0, &default_settings);
    default_settings.c_lflag |= ICANON;
    default_settings.c_lflag |= ECHO;
    tcsetattr(0, TCSANOW, &default_settings);
}

// Generar múltiples manzanas en posiciones aleatorias
void generarManzanas(int cantidad) {
    manzanas.clear();

    for (int i = 0; i < cantidad; ++i) {
        int x, y;
        do {
            x = rand() % (ancho - 2) + 1; // Evitar bordes
            y = rand() % (largo - 2) + 1; // Evitar bordes
        } while (terreno[y][x] != 0);
        manzanas.push_back({x, y});
        terreno[y][x] = 1;
    }
}

// Generar el laberinto usando el algoritmo de Backtracking recursivo adaptado a dimensiones pares
void generarLaberinto(int width, int height) {
    // Reiniciar el terreno con paredes
    terreno = std::vector<std::vector<int>>(height, std::vector<int>(width, 3));

    // Establecer entrada y salida antes de tallar el laberinto
    terreno[1][0] = 5;                    // Entrada en el borde izquierdo
    terreno[height - 2][width - 1] = 6;   // Salida en el borde derecho

    // Punto inicial para tallar el laberinto (iniciar dentro de los bordes)
    int startX = 1;
    int startY = 1;

    tallarLaberinto(startX, startY);
}

// Función recursiva para tallar el laberinto adaptado a dimensiones pares y mantener los bordes como paredes
void tallarLaberinto(int x, int y) {
    terreno[y][x] = 0; // Marcar como camino

    // Direcciones de movimiento: N, S, E, O
    std::vector<std::pair<int, int>> directions = {
        {0, -1}, // Norte
        {0, 1},  // Sur
        {1, 0},  // Este
        {-1, 0}  // Oeste
    };

    // Aleatorizar las direcciones
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(directions.begin(), directions.end(), g);

    // Intentar tallar en cada dirección
    for (auto dir : directions) {
        int nx = x + dir.first * 2;
        int ny = y + dir.second * 2;

        // Asegurarse de que las nuevas coordenadas no estén en los bordes
        if (nx > 0 && nx < ancho - 1 && ny > 0 && ny < largo - 1 && terreno[ny][nx] == 3) {
            terreno[ny][nx] = 0;
            terreno[y + dir.second][x + dir.first] = 0; // Eliminar pared intermedia
            tallarLaberinto(nx, ny);
        }
    }
}

// Verificar si el laberinto es válido (si hay un camino desde la entrada hasta la salida)
int verificarTerreno() {
    std::queue<std::pair<int, int>> cola;
    std::vector<std::vector<bool>> visitado(largo, std::vector<bool>(ancho, false));

    cola.push({1, 0}); // Coordenadas de la entrada
    visitado[1][0] = true;

    // Definir las direcciones de movimiento
    std::vector<std::pair<int, int>> direcciones = {
        {0, -1}, // Norte
        {0, 1},  // Sur
        {1, 0},  // Este
        {-1, 0}  // Oeste
    };

    while (!cola.empty()) {
        auto [x, y] = cola.front();
        cola.pop();

        for (auto &dir : direcciones) {
            int nx = x + dir.first;
            int ny = y + dir.second;

            if (nx >= 0 && nx < ancho && ny >= 0 && ny < largo &&
                !visitado[ny][nx] && terreno[ny][nx] != 3) {
                visitado[ny][nx] = true;
                cola.push({nx, ny});
            }
        }
    }

    return visitado[largo - 2][ancho - 1];
}

// Inicializar el terreno y la serpiente
void iniciarTerreno() {
    // Cambiar de color el terreno para cada nivel
    colorfondo = rand() % 4;
    colorsnake = rand() % 2;

    // Generar el laberinto y verificar su validez
    do {
        generarLaberinto(ancho, largo);
    } while (!verificarTerreno());

    // Reiniciar la serpiente
    serpiente.clear();
    serpiente.push_back({1, 1});
    direccion = {0, 0}; // La serpiente no se mueve hasta que el jugador ingrese una dirección

    // Generar manzanas (por ejemplo, 3 manzanas por nivel)
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
    std::lock_guard<std::mutex> lock(terreno_mutex);

    std::cout << "\033[2J\033[H"; // Limpiar la pantalla y mover el cursor a la esquina superior izquierda

    mostrarPuntaje();

    for (int y = 0; y < largo; y++) {
        for (int x = 0; x < ancho; x++) {

            if (serpiente[0].x == x && serpiente[0].y == y) {
                std::cout << "\033[48;5;" << CabezaSnake[colorsnake] << "m  " << "\033[0m"; // Cabeza de la serpiente
            } else if (terreno[y][x] == 2) {
                std::cout << "\033[48;5;" << Snake[colorsnake] << "m  " << "\033[0m"; // Cuerpo de la serpiente
            } else if (terreno[y][x] == 1) {
                std::cout << "\033[48;5;" << Manzana[colorsnake] << "m  " << "\033[0m"; // Manzana
            } else if (terreno[y][x] == 3) {
                std::cout << "\033[48;5;" << Paredes[colorfondo] << "m  " << "\033[0m"; // Pared
            } else if (terreno[y][x] == 5) {
                std::cout << "\033[48;5;220m  \033[0m"; // Entrada
            } else if (terreno[y][x] == 6) {
                std::cout << "\033[48;5;46m  \033[0m"; // Salida
            } else {
                std::cout << "\033[48;5;" << Fondo[colorfondo] << "m  " << "\033[0m"; // Camino libre
            }
        }
        std::cout << std::endl;
    }
    std::cout.flush();
}

// Actualizar el terreno con la posición de la serpiente y las manzanas
void actualizarTerreno() {
    std::lock_guard<std::mutex> lock(terreno_mutex);

    // Limpiar el terreno (excepto paredes, entrada y salida)
    for (int y = 0; y < largo; y++) {
        for (int x = 0; x < ancho; x++) {
            if (terreno[y][x] == 2 || terreno[y][x] == 1) {
                terreno[y][x] = 0;
            }
        }
    }
    // Colocar la serpiente en el terreno
    for (auto &segmento : serpiente) {
        if (segmento.x >= 0 && segmento.x < ancho && segmento.y >= 0 && segmento.y < largo) {
            // Evitar sobrescribir las paredes de los bordes
            if (segmento.x == 0 || segmento.x == ancho - 1 || segmento.y == 0 || segmento.y == largo - 1) {
                continue;
            }
            terreno[segmento.y][segmento.x] = 2;
        }
    }
    // Colocar las manzanas
    for (auto &manzana : manzanas) {
        if (manzana.x >= 0 && manzana.x < ancho && manzana.y >= 0 && manzana.y < largo) {
            // Evitar colocar manzanas en las paredes de los bordes
            if (manzana.x == 0 || manzana.x == ancho - 1 || manzana.y == 0 || manzana.y == largo - 1) {
                continue;
            }
            terreno[manzana.y][manzana.x] = 1;
        }
    }
}

// Mover la serpiente
void *moverSerpiente(void *arg) {
    while (!game_over) {
        Coordenada dir_actual;
        {
            std::lock_guard<std::mutex> lock(direccion_mutex);
            dir_actual = direccion;
        }

        if (dir_actual.x == 0 && dir_actual.y == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        Coordenada nueva_cabeza = {serpiente[0].x + dir_actual.x, serpiente[0].y + dir_actual.y};

        {
            std::lock_guard<std::mutex> lock(terreno_mutex);
            // Verificar límites y colisiones
            if (nueva_cabeza.x < 0 || nueva_cabeza.x >= ancho || nueva_cabeza.y < 0 || nueva_cabeza.y >= largo) {
                std::cout << "¡Te has salido del área de juego!" << std::endl;
                game_over = true;
                break;
            }

            if (terreno[nueva_cabeza.y][nueva_cabeza.x] == 3) {
                std::cout << "¡Has chocado contra una pared!" << std::endl;
                game_over = true;
                break;
            }

            if (terreno[nueva_cabeza.y][nueva_cabeza.x] == 2) {
                std::cout << "¡Has chocado contra tu propio cuerpo!" << std::endl;
                game_over = true;
                break;
            }

            if (terreno[nueva_cabeza.y][nueva_cabeza.x] == 6) {
                // Has completado el nivel
                puntaje += 10;
                if (velocidad > 100) {
                    velocidad -= 5;
                    if (velocidad < 100) {
                        velocidad = 100;
                    }
                }
                nivel++;
                aumentarNivel();
                continue; // Reiniciar el ciclo con el nuevo nivel
            }
        }

        // Comer manzana
        bool comioManzana = false;
        {
            std::lock_guard<std::mutex> lock(terreno_mutex);
            if (terreno[nueva_cabeza.y][nueva_cabeza.x] == 1) {
                comioManzana = true;
                // Eliminar la manzana de la lista
                manzanas.erase(std::remove_if(manzanas.begin(), manzanas.end(),
                    [&](const Coordenada& m) { return m.x == nueva_cabeza.x && m.y == nueva_cabeza.y; }),
                    manzanas.end());
                puntaje += 1;
            }
        }

        if (comioManzana) {
            Coordenada cola = serpiente.back();
            serpiente.push_back({cola.x, cola.y}); // Agregar segmento en la posición de la cola
        }

        // Mover la serpiente
        for (int i = serpiente.size() - 1; i > 0; i--) {
            serpiente[i] = serpiente[i - 1];
        }
        serpiente[0] = nueva_cabeza;

        movimiento_completado = true;

        if (movimiento_completado && largoDefaultContador < 3) {
            largoDefaultContador++;
            Coordenada cola = serpiente.back();
            serpiente.push_back({cola.x, cola.y});
        }

        actualizarTerreno();
        imprimirTerreno();
        std::this_thread::sleep_for(std::chrono::milliseconds(velocidad));
    }

    restaurarTerminal();
    pthread_exit(0);
}

// Manejar el input del usuario
void *manejarInput(void *arg) {
    char tecla;
    while (!game_over) {
        ssize_t bytes_read = read(0, &tecla, 1);
        if (bytes_read > 0) {
            Coordenada nueva_direccion;

            // Determinar la nueva dirección basada en la tecla
            switch (tecla) {
                case 'w':
                    nueva_direccion = {0, -1};
                    break;
                case 's':
                    nueva_direccion = {0, 1};
                    break;
                case 'a':
                    nueva_direccion = {-1, 0};
                    break;
                case 'd':
                    nueva_direccion = {1, 0};
                    break;
                default:
                    continue; // Ignorar otras teclas
            }

            // Evitar que la serpiente se mueva en la dirección opuesta directamente
            {
                std::lock_guard<std::mutex> lock(direccion_mutex);
                if ((nueva_direccion.x != -direccion.x || nueva_direccion.y != -direccion.y) &&
                    (nueva_direccion.x != 0 || nueva_direccion.y != 0)) {
                    direccion = nueva_direccion;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Pausa para reducir uso de CPU
    }
    pthread_exit(0);
}

// Función para aumentar el nivel y generar un nuevo laberinto
void aumentarNivel() {
    // Limpiar la pantalla usando códigos ANSI
    std::cout << "\033[2J\033[H";
    largoDefaultContador = 0;
    // Resetear el terreno y generar un nuevo laberinto
    iniciarTerreno();
}

// Función para manejar señales y restaurar la terminal
void manejarSenal(int senal) {
    restaurarTerminal();
    exit(0);
}

int main() {
    srand(time(0));

    // Limpiar la pantalla
    std::cout << "\033[2J\033[H";

    // Configurar manejo de señales
    signal(SIGINT, manejarSenal);
    signal(SIGTERM, manejarSenal);

    configurarTerminal();

    iniciarTerreno();

    pthread_t hilo_movimiento, hilo_input;

    pthread_create(&hilo_movimiento, NULL, moverSerpiente, NULL);
    pthread_create(&hilo_input, NULL, manejarInput, NULL);

    pthread_join(hilo_movimiento, NULL);
    pthread_join(hilo_input, NULL);

    restaurarTerminal();

    // Mostrar puntaje final
    std::cout << "Juego terminado. Puntaje final: " << puntaje << std::endl;

    return 0;
}
