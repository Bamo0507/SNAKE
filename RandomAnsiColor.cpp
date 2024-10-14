#include <iostream>
#include <cstdlib>  // Librería para generar números aleatorios
#include <ctime>    // Semilla para el random
#include <vector>   // Manejar la matriz con vectores
#include <thread>   // Para dormir el hilo y simular el avance del juego
#include <chrono>   // Para controlar la duración del sleep
#include <iomanip>  // Para formatear la impresión

// Dimensiones iniciales
int ancho = 15;
int largo = 10;

// Cantidad inicial de manzanas
int appleCount = 50;
int score = 0; // Puntaje inicial (se mantiene en 0)

// Colores para terreno y manzanas
int colorTerreno;
int colorManzana;

// Generación de matriz (terreno)
std::vector<std::vector<int>> terreno;

// Inicializar terreno
void iniciar_Terreno(int valor_inicial) {
    terreno.resize(largo, std::vector<int>(ancho, valor_inicial));
    // Se genera el terreno y se llena cada casilla con el valor pasado por parámetro
}

// Generación de manzanas aleatorias
void generarManzanas(int valorBase, int valorManzana) {
    int count = 0; // Contador para manzanas generadas en el terreno
    while (count < appleCount) {
        int x = rand() % ancho; // Genera un número aleatorio entre 0 y el ancho
        int y = rand() % largo; // Genera un número aleatorio entre 0 y el largo
        // Si la celda está vacía (valorBase), se coloca la manzana (valorManzana)
        if (terreno[y][x] == valorBase) {
            terreno[y][x] = valorManzana;
            count++;  // Incrementar el contador de manzanas colocadas
        }
    }
}

// Impresión de la matriz del terreno con ANSI y colores
void imprimir_Terreno(int nivel) {
    // Mueve el cursor a la esquina superior izquierda
    std::cout << "\033[H"; // ANSI escape para mover el cursor al principio

    // Imprimir Nivel en la esquina superior izquierda y Score en la esquina superior derecha
    std::cout << "Nivel: " << nivel 
              << std::setw(ancho * 2 - 10) // Ajusta la posición del "Score"
              << "Score: " << score << std::endl;

    for (int y = 0; y < largo; y++) {
        for (int x = 0; x < ancho; x++) {
            if (terreno[y][x] == 1) {
                // Imprimir manzana con color
                std::cout << "\033[48;5;" << colorManzana << "m  " << "\033[0m";
            } else {
                // Imprimir celda vacía con color
                std::cout << "\033[48;5;" << colorTerreno << "m  " << "\033[0m";
            }
        }
        std::cout << std::endl;
    }
    std::cout.flush(); // Asegura que la salida se imprima inmediatamente
}

// Reducir el número de manzanas cuando se completa un nivel
void disminuirManzanas(int valor_inicial, int valorManzana) {
    if (appleCount > 1) {
        appleCount -= 5; // Reducir la cantidad de manzanas en 5
    } else {
        appleCount = 1; // Si el número de manzanas es 1, mantenerlo en 1
    }
    terreno.clear(); // Limpiar el terreno anterior
    iniciar_Terreno(valor_inicial); // Reiniciar el terreno con el valor base
    generarManzanas(valor_inicial, valorManzana); // Generar las nuevas manzanas

    // Generar colores aleatorios para cada nivel
    colorTerreno = rand() % 256;  // Color aleatorio para el terreno (0-255)
    colorManzana = rand() % 256;  // Color aleatorio para las manzanas (0-255)
}

int main() {
    srand(time(0)); // Inicia la semilla para los números aleatorios

    // Generar colores iniciales para el nivel 1
    colorTerreno = rand() % 256;  // Color aleatorio para el terreno
    colorManzana = rand() % 256;  // Color aleatorio para las manzanas

    // Iniciar el terreno con un valor inicial de 0 (terreno vacío)
    iniciar_Terreno(0);
    // Generar manzanas aleatorias en el terreno
    generarManzanas(0, 1);

    // Limpia la terminal y oculta el cursor para hacer la simulación más fluida
    std::cout << "\033[2J"; // ANSI escape para limpiar la pantalla
    std::cout << "\033[?25l"; // Oculta el cursor (opcional, para mejor visualización)

    // Simulación del nivel 1
    imprimir_Terreno(1);

    // Simular el avance de nivel y la reducción de manzanas
    for (int nivel = 2; nivel <= 10; nivel++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Simular tiempo de espera entre niveles
        disminuirManzanas(0, 1);
        imprimir_Terreno(nivel);
    }

    // Restablecer la visibilidad del cursor antes de terminar
    std::cout << "\033[?25h"; // Muestra el cursor de nuevo
    return 0;
}
