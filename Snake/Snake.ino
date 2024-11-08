/*
 * Este código ESP32 se crea por esp32io.com
 *
 * Este código ESP32 se libera en el dominio público
 *
 * Para más detalles (instrucciones y diagrama de cableado), visita https://esp32io.com/tutorials/esp32-neopixel-led-strip
 */

#include <Adafruit_NeoPixel.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include "esp_system.h"  // para esp_random()

#define PIN_NEO_PIXEL 16      // El pin GPIO16 del ESP32 conectado a NeoPixel
#define PIN_LIGHT_SENSOR 36   // El pin GPIO36 del ESP32 conectado al sensor de luz
#define NUM_PIXELS 64         // El número de LEDs (pixels) en la tira NeoPixel LED

// Dimensiones iniciales del terreno
int ancho = 8; // Ancho inicial del terreno (puede ser par o impar)
int largo = 8; // Alto inicial del terreno (puede ser par o impar)
int nivel = 1;  // Nivel actual
int puntaje = 0; // Puntaje del jugador

// Matriz del terreno
std::vector<std::vector<int>> terreno; // 0 = camino, 1 = manzana, 2 = serpiente, 3 = pared, 5 = entrada, 6 = salida

// Estructura para manejar lógica de matriz
struct Coordenada {
    int x, y;
};

// LÓGICA POSICIONAL INICIAL
std::vector<Coordenada> serpiente;
Coordenada direccion = {0, 0};
std::vector<Coordenada> manzanas; // Lista de manzanas

// Declaración de funciones
void generarManzanas(int cantidad);
void iniciarTerreno();
void imprimirTerreno();
void actualizarTerreno();
void generarLaberinto(int width, int height);
void tallarLaberinto(int x, int y);
std::vector<Coordenada> obtenerCeldasBorde();
Coordenada seleccionarCeldaAleatoria(const std::vector<Coordenada>& celdasBorde);
std::vector<std::pair<int, int>> obtenerDireccionesAleatorias();

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

void setup() {
  NeoPixel.begin();  // inicializar el objeto NeoPixel (REQUERIDO)
  Serial.begin(9600); // abrir el puerto serie a 9600 bps:
  pinMode(PIN_LIGHT_SENSOR, INPUT);
}

void loop() {
  int Light_value = analogRead(PIN_LIGHT_SENSOR); // valor entre 0 a 4095
  int Brightness = std::floor((Light_value / 16));
  if(Brightness < 20){
    Brightness = 10;
  }

  NeoPixel.setBrightness(Brightness); // un valor de 0 a 255
  Serial.println(Brightness);

  iniciarTerreno();
  delay(5000); 

  // Puedes reactivar el código comentado para efectos de los LEDs según necesites
}

/*
 * Función para imprimir el terreno en los LEDs
 */
void imprimirTerreno() {
    //NeoPixel.clear();  // apaga todos los LEDs

    for (int y = 0; y < largo; y++) {
        for (int x = 0; x < ancho; x++) {
            int pixel = (y * ancho) + x;
            if (serpiente.size() > 0 && serpiente[0].x == x && serpiente[0].y == y) { // Cabeza de la serpiente
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(40, 120, 40));
            } else if (terreno[y][x] == 2) { // Cuerpo de la serpiente
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(0, 255, 0));
            } else if (terreno[y][x] == 1) { // Manzana
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 0, 0));
            } else if (terreno[y][x] == 3) { // Pared
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(0, 255, 255));
            } else if (terreno[y][x] == 5) { // Entrada
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 255, 0));
            } else if (terreno[y][x] == 6) { // Salida
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 0, 255));
            } else { // Fondo
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(0, 0, 255));
            }
        }
    }
    NeoPixel.show();
}

/*
 * Función para generar múltiples manzanas en posiciones aleatorias
 */
void generarManzanas(int cantidad) {
    manzanas.clear();

    for (int i = 0; i < cantidad; ++i) {
        int x, y;
        do {
            x = rand() % (ancho - 2) + 1;
            y = rand() % (largo - 2) + 1;
        } while (terreno[y][x] != CAMINO);
        manzanas.push_back({x, y});
        terreno[y][x] = 1;
    }
}

/*
 * Función para generar el laberinto usando el algoritmo de Backtracking recursivo
 */
void generarLaberinto(int width, int height) {
    // Reiniciar el terreno con paredes
    terreno = std::vector<std::vector<int>>(height, std::vector<int>(width, PARED));

    // Obtener todas las celdas de borde
    std::vector<Coordenada> celdasBorde = obtenerCeldasBorde();

    // Seleccionar aleatoriamente la entrada y la salida
    Coordenada entrada = seleccionarCeldaAleatoria(celdasBorde);
    Coordenada salida;
    do {
        salida = seleccionarCeldaAleatoria(celdasBorde);
    } while ((entrada.x == salida.x) && (entrada.y == salida.y));

    // Ajustar la entrada si está en una posición par
    if (entrada.x % 2 == 0 && entrada.y % 2 == 0) {
        if (entrada.x == 0) entrada.x += 1;
        if (entrada.x == ancho - 1) entrada.x -= 1;
        if (entrada.y == 0) entrada.y += 1;
        if (entrada.y == largo - 1) entrada.y -= 1;
    }

    // Tallar el laberinto comenzando desde la entrada
    tallarLaberinto(entrada.x, entrada.y);

    // Establecer entrada y salida en el laberinto
    terreno[entrada.y][entrada.x] = ENTRADA;
    terreno[salida.y][salida.x] = SALIDA;
}

/*
 * Función para obtener todas las celdas de borde
 */
std::vector<Coordenada> obtenerCeldasBorde() {
    std::vector<Coordenada> celdasBorde;
    for (int x = 0; x < ancho; x++) {
        celdasBorde.push_back({x, 0});           // Borde superior
        celdasBorde.push_back({x, largo - 1});   // Borde inferior
    }
    for (int y = 1; y < largo - 1; y++) {
        celdasBorde.push_back({0, y});           // Borde izquierdo
        celdasBorde.push_back({ancho - 1, y});   // Borde derecho
    }
    return celdasBorde;
}

/*
 * Función para seleccionar aleatoriamente una celda de borde
 */
Coordenada seleccionarCeldaAleatoria(const std::vector<Coordenada>& celdasBorde) {
    int index = esp_random() % celdasBorde.size();
    return celdasBorde[index];
}

/*
 * Función para mezclar direcciones aleatoriamente usando esp_random()
 */
std::vector<std::pair<int, int>> obtenerDireccionesAleatorias() {
    std::vector<std::pair<int, int>> direcciones = {
        {0, -1},  // Norte
        {1, 0},   // Este
        {0, 1},   // Sur
        {-1, 0}   // Oeste
    };

    // Mezclar direcciones usando esp_random()
    for (int i = direcciones.size() - 1; i > 0; --i) {
        int j = esp_random() % (i + 1);
        std::swap(direcciones[i], direcciones[j]);
    }

    return direcciones;
}

/*
 * Función para tallar el laberinto recursivamente
 */
void tallarLaberinto(int x, int y) {
    terreno[y][x] = CAMINO; // Marcar como camino

    std::vector<std::pair<int, int>> direcciones = obtenerDireccionesAleatorias();

    for (auto dir : direcciones) {
        int nx = x + dir.first * 2;
        int ny = y + dir.second * 2;

        // Asegurar que la nueva posición esté dentro de los límites y sea una pared
        if (nx >= 0 && nx < ancho && ny >= 0 && ny < largo && terreno[ny][nx] == PARED) {
            terreno[y + dir.second][x + dir.first] = CAMINO; // Eliminar la pared intermedia
            tallarLaberinto(nx, ny);
        }
    }
}

/*
 * Función para inicializar el terreno y la serpiente
 */
void iniciarTerreno() {
    // Generar el laberinto
    generarLaberinto(ancho, largo);

    // Reiniciar la serpiente
    serpiente.clear();
    // Establecer la cabeza de la serpiente en la entrada
    for (int y = 0; y < largo; y++) {
        for (int x = 0; x < ancho; x++) {
            if (terreno[y][x] == ENTRADA) {
                serpiente.push_back({x, y});
                break;
            }
        }
        if (serpiente.size() > 0) break;
    }
    direccion = {0, 0}; // Reiniciar dirección

    // Generar manzanas (por ejemplo, 3 manzanas por nivel)
    generarManzanas(3);

    // Actualizar el terreno con la posición de la serpiente y las manzanas
    actualizarTerreno();

    // Imprimir el terreno en los LEDs
    imprimirTerreno();
}

/*
 * Función para actualizar el terreno con la posición de la serpiente y las manzanas
 */
void actualizarTerreno() {
    for (int y = 0; y < largo; y++) {
        for (int x = 0; x < ancho; x++) {
            if (terreno[y][x] == 2) {
                terreno[y][x] = CAMINO;
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
