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

// Dimensiones iniciales del terreno
int ancho_inicial = 8; // Ancho inicial del terreno (debe ser impar para el algoritmo)
int largo_inicial = 6; // Alto inicial del terreno (debe ser impar para el algoritmo)
int ancho = ancho_inicial; // Ancho actual del terreno
int largo = largo_inicial; // Alto actual del terreno

int nivel = 1;  // Nivel actual
int puntaje = 0; // Puntaje del jugador

int colorfondo = 0; //Index de colores del fondo de en un array
int colorsnake = 0; //Index de colores de la serpiente de un array

//Colores de cada objeto para generacion aleatoria de colores
int Fondo[5] = {51, 63, 184, 214, 177};
int Paredes[5] = {80, 97, 136, 130, 92};
int Manzana[3] = {9, 88, 197};
int Snake[3] = {28, 77, 84};
int CabezaSnake[3] = {10, 106, 65};

int largoDefaultContador = 0; //Contador para controlar cuando la serpiente ya haya crecido a 3
int velocidad = 400; //Variable global para controlar la velocidad

#define Num_hilos 4
pthread_barrier_t barrera;

// Matriz del terreno
std::vector<std::vector<int>> terreno; // 0 = camino, 1 = manzana, 2 = serpiente, 3 = pared, 5 = entrada, 6 = salida

std::atomic<bool> movimiento_completado(true);

std::mutex terreno_mutex; // Para proteger el acceso al terreno durante actualizaciones

// Estructura para manejar lógica de matriz
struct Coordenada {
    int x, y;
};

// LÓGICA POSICIONAL INICIAL
std::vector<Coordenada> serpiente;
Coordenada direccion = {0, 0};
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
void *hiloActualizarTerreno(void *arg);

void generarLaberinto(int width, int height);
void* tallarLaberinto(int x, int y);

void aumentarNivel();
void mostrarPuntaje();

// Configuración de la terminal
void configurarTerminal() {
    struct termios new_settings;
    tcgetattr(0, &new_settings);
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
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
            x = rand() % (ancho - 2) + 1;
            y = rand() % (largo - 2) + 1;
        } while (terreno[y][x] != 0);
        manzanas.push_back({x, y});
        terreno[y][x] = 1;
    }
}

// Generar el laberinto usando el algoritmo de Backtracking recursivo
void generarLaberinto(int width, int height) {
    pthread_t threads[Num_hilos];

    pthread_barrier_init(&barrera, NULL, Num_hilos);

    // Reiniciar el terreno con paredes
    terreno = std::vector<std::vector<int>>(height, std::vector<int>(width, 3));

    // Asegurar que las dimensiones sean impares
    if (width % 2 == 0) width--;
    if (height % 2 == 0) height--;

    // Punto inicial para tallar el laberinto
    int startX = 1;
    int startY = 1;

    for(int i = 0; i < Num_hilos; i++){
        // Tallar el laberinto desde el punto inicial
        pthread_create(&threads[i], NULL, [](void* arg) -> void* {
            auto coords = static_cast<std::pair<int, int>*>(arg);
            tallarLaberinto(coords->first, coords->second);
            delete coords;
            return nullptr;
        }, new std::pair<int, int>(startX, startY));
    }
    
    terreno[height - 2][width - 3] = 0; 
    terreno[height - 2][width - 2] = 0;   
    terreno[height - 3][width - 2] = 0;   


    // Establecer entrada y salida
    terreno[1][0] = 5;                    // Entrada
    terreno[height - 2][width - 1] = 6;   // Salida
}

// Función recursiva para tallar el laberinto
void* tallarLaberinto(int x, int y) {
    terreno[y][x] = 0; // Marcar como camino

    // Direcciones de movimiento: N, S, E, O
    std::vector<std::pair<int, int>> directions = {
        {0, -2}, // Norte
        {0, 2},  // Sur
        {2, 0},  // Este
        {-2, 0}  // Oeste
    };

    // Aleatorizar las direcciones
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(directions.begin(), directions.end(), g);

    // Intentar tallar en cada dirección
    for (auto dir : directions) {
        int nx = x + dir.first;
        int ny = y + dir.second;

        if (nx > 0 && nx < ancho - 1 && ny > 0 && ny < largo - 1 && terreno[ny][nx] == 3) {
            terreno[ny][nx] = 0;
            terreno[y + dir.second / 2][x + dir.first / 2] = 0; // Eliminar pared intermedia
            tallarLaberinto(nx, ny);
        }
    }
    pthread_barrier_wait(&barrera);

    return NULL;
}

// Inicializar el terreno y la serpiente
void iniciarTerreno() {
    //Cambiar de color el terreno para cada nivel
    colorfondo = rand() % 4;
    colorsnake = rand() % 2;

    // Ajustar dimensiones del laberinto para cada nivel
    ancho = ancho_inicial + (nivel - 1) * 2; // Incrementar el tamaño del laberinto en cada nivel
    largo = largo_inicial + (nivel - 1) * 2;

    // Asegurar que las dimensiones sean impares
    if (ancho % 2 == 0) ancho++;
    if (largo % 2 == 0) largo++;

    // Regenerar el laberinto
    generarLaberinto(ancho, largo);

    // Reiniciar la serpiente
    serpiente.clear();
    serpiente.push_back({1, 1});
    direccion = {0, 0}; // Reiniciar dirección

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
        terreno[segmento.y][segmento.x] = 2;
    }
    // Colocar las manzanas
    for (auto &manzana : manzanas) {
        terreno[manzana.y][manzana.x] = 1;
    }
}

// Mover la serpiente
void *moverSerpiente(void *arg) {
    while (!game_over) {
        Coordenada nueva_cabeza = {serpiente[0].x + direccion.x, serpiente[0].y + direccion.y};

        // Verificar límites y colisiones
        if (nueva_cabeza.x < 0 || nueva_cabeza.x >= ancho || nueva_cabeza.y < 0 || nueva_cabeza.y >= largo) {
            std::cout << "¡Te has salido del área de juego!" << std::endl;
            game_over = true;
            pthread_exit(0);
        }

        if (terreno[nueva_cabeza.y][nueva_cabeza.x] == 3) {
            std::cout << "┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "███▀▀▀██┼███▀▀▀███┼███▀█▄█▀███┼██▀▀▀" << std::endl;
            std::cout << "██┼┼┼┼██┼██┼┼┼┼┼██┼██┼┼┼█┼┼┼██┼██┼┼┼" << std::endl;
            std::cout << "██┼┼┼▄▄▄┼██▄▄▄▄▄██┼██┼┼┼▀┼┼┼██┼██▀▀▀" << std::endl;
            std::cout << "███▄▄▄██┼██┼┼┼┼┼██┼██┼┼┼┼┼┼┼██┼██▄▄▄" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "███▀▀▀███┼▀███┼┼██▀┼██▀▀▀┼██▀▀▀▀██▄┼" << std::endl;
            std::cout << "██┼┼┼┼┼██┼┼┼██┼┼██┼┼██┼┼┼┼██┼┼┼┼┼██┼" << std::endl;
            std::cout << "██┼┼┼┼┼██┼┼┼██┼┼██┼┼██▀▀▀┼██▄▄▄▄▄▀▀┼" << std::endl;
            std::cout << "██┼┼┼┼┼██┼┼┼██┼┼█▀┼┼██┼┼┼┼██┼┼┼┼┼██┼" << std::endl;
            std::cout << "███▄▄▄███┼┼┼─▀█▀┼┼─┼██▄▄▄┼██┼┼┼┼┼██▄" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼██┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼██┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼████▄┼┼┼▄▄▄▄▄▄▄┼┼┼▄████┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼▀▀█▄█████████▄█▀▀┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼┼┼█████████████┼┼┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼┼┼██▀▀▀███▀▀▀██┼┼┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼┼┼██┼┼┼███┼┼┼██┼┼┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼┼┼█████▀▄▀█████┼┼┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼┼┼┼███████████┼┼┼┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼▄▄▄██┼┼█▀█▀█┼┼██▄▄▄┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼▀▀██┼┼┼┼┼┼┼┼┼┼┼██▀▀┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼┼▀▀┼┼┼┼┼┼┼┼┼┼┼▀▀┼┼┼┼┼┼┼┼┼┼┼" << std::endl;
            std::cout << "┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼" << std::endl;  

            game_over = true;
            pthread_exit(0);
        }

        if (terreno[nueva_cabeza.y][nueva_cabeza.x] == 6) {
            // Has completado el nivel
            puntaje += 10;
            if (velocidad > 100){
                velocidad -= 5;
            }
            nivel++;
            aumentarNivel();
            continue; // Reiniciar el ciclo con el nuevo nivel
        }

        // Comer manzana
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
            serpiente.push_back({-1, -1}); // Agregar segmento
        }

        // Mover la serpiente
        for (int i = serpiente.size() - 1; i > 0; i--) {
            serpiente[i] = serpiente[i - 1];
        }
        serpiente[0] = nueva_cabeza;

        movimiento_completado = true;

        if(movimiento_completado && largoDefaultContador < 3){
            largoDefaultContador++;
            serpiente.push_back({serpiente.back().x, serpiente.back().y});
        }
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
            if (movimiento_completado.load()) {
                movimiento_completado = false;

                if (direccion.x != 0 || direccion.y != 0) {
                    if (direccion.x != 0) {
                        if (tecla == 'w') {
                            direccion = {0, -1};
                        } else if (tecla == 's') {
                            direccion = {0, 1};
                        }
                    } else if (direccion.y != 0) {
                        if (tecla == 'a') {
                            direccion = {-1, 0};
                        } else if (tecla == 'd') {
                            direccion = {1, 0};
                        }
                    }
                } else {
                    switch (tecla) {
                    case 'w':
                        direccion = {0, -1};
                        break;
                    case 's':
                        direccion = {0, 1};
                        break;
                    case 'a':
                        direccion = {-1, 0};
                        break;
                    case 'd':
                        direccion = {1, 0};
                        break;
                    }
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
    // Limpiar la pantalla usando códigos ANSI
    std::cout << "\033[2J\033[H";
    largoDefaultContador = 0;
    // Resetear el terreno y generar un nuevo laberinto
    iniciarTerreno();
}

int main() {
    srand(time(0));

    // Limpiar la pantalla
    std::cout << "\033[2J\033[H";

    colorfondo = rand() % 4;
    colorsnake = rand() % 2;


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

    // Mostrar puntaje final
    std::cout << "Juego terminado. Puntaje final: " << puntaje << std::endl;

    return 0;
}
