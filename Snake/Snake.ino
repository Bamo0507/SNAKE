/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-neopixel-led-strip
 */

#include <Adafruit_NeoPixel.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include "esp_system.h"  // for esp_random()

#define PIN_NEO_PIXEL 16  // The ESP32 pin GPIO16 connected to NeoPixel
#define PIN_LIGHT_SENSOR 36 // The ESP32 pin GPIO36 connected to light sensor
#define NUM_PIXELS 64  // The number of LEDs (pixels) on NeoPixel LED strip

// Dimensiones iniciales del terreno
int ancho = 8; // Ancho inicial del terreno (debe ser impar para el algoritmo)
int largo = 8; // Alto inicial del terreno (debe ser impar para el algoritmo)
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

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

void setup() {
  NeoPixel.begin();  // initialize NeoPixel strip object (REQUIRED)
  Serial.begin(9600); // open the serial port at 9600 bps:
  pinMode(PIN_LIGHT_SENSOR, INPUT);
}

void loop() {
  int Light_value = analogRead(PIN_LIGHT_SENSOR); // value between 0 to 4095
  int Brightness = std::floor((Light_value / 16));
  if(Brightness < 20){
    Brightness = 10;
  }

  NeoPixel.setBrightness(Brightness); // a value from 0 to 255
  Serial.println(Brightness);

  iniciarTerreno();
  delay(5000); 


  // turn pixels to green one-by-one with delay between each pixel
  /*
  for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {           // for each pixel
    NeoPixel.setPixelColor(pixel, NeoPixel.Color(0, 255, 0));  // it only takes effect if pixels.show() is called
    NeoPixel.show();                                           // update to the NeoPixel Led Strip

    delay(50);  // 500ms pause between each pixel
  }

  // turn off all pixels for two seconds
  NeoPixel.clear();
  NeoPixel.show();  // update to the NeoPixel Led Strip
  delay(500);      // 2 seconds off time

  // turn on all pixels to red at the same time for two seconds
  for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {           // for each pixel
    NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 0, 0));  // it only takes effect if pixels.show() is called
  }
  NeoPixel.show();  // update to the NeoPixel Led Strip
  delay(1000);      // 1 second on time

  // turn off all pixels for one seconds
  NeoPixel.clear();
  NeoPixel.show();  // update to the NeoPixel Led Strip
  delay(500);      // 1 second off time
  */
}

// Imprimir el terreno en la terminal
void imprimirTerreno() {
    //NeoPixel.clear();  // set all pixel colors to 'off'. It only takes effect if pixels.show() is called

    for (int y = 0; y < largo; y++) {
        for (int x = 0; x < ancho; x++) {
            int pixel = ((y * 8) + x);
            if (serpiente[0].x == x && serpiente[0].y == y) { // Cabeza de la serpiente
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(40, 120, 40));  // it only takes effect if pixels.show() is called
                //NeoPixel.show();                                           // update to the NeoPixel Led Strip
            } else if (terreno[y][x] == 2) {// Cuerpo de la serpiente
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(0, 255, 0));  // it only takes effect if pixels.show() is called
                //NeoPixel.show();  
            } else if (terreno[y][x] == 1) {// Manzana
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 0, 0));  // it only takes effect if pixels.show() is called
                //NeoPixel.show();  
            } else if (terreno[y][x] == 3) {// Pared
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(0, 255, 255));  // it only takes effect if pixels.show() is called
                //NeoPixel.show();
            } else if (terreno[y][x] == 5) { //Entrada
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 255, 0));  // it only takes effect if pixels.show() is called
                //NeoPixel.show();
            } else if (terreno[y][x] == 6) { //Salida
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(255, 0, 255));  // it only takes effect if pixels.show() is called
                //NeoPixel.show();
            } else { //Fondo
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(0, 0, 255));  // it only takes effect if pixels.show() is called
                //NeoPixel.show();
            }
        }
    }
    NeoPixel.show();
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

    // Reiniciar el terreno con paredes
    terreno = std::vector<std::vector<int>>(height, std::vector<int>(width, 3));

    // Asegurar que las dimensiones sean impares
    if (width % 2 == 0) width--;
    if (height % 2 == 0) height--;

    // Punto inicial para tallar el laberinto
    int startX = 1;
    int startY = 1;

    tallarLaberinto(startX, startY);

    // Establecer entrada y salida
    terreno[1][0] = 5;                    // Entrada
    terreno[6][7] = 6;   // Salida

    

}

// Function to recursively carve the maze
void tallarLaberinto(int x, int y) {
    terreno[y][x] = 0; // Mark as path

    // Movement directions: N, S, E, W
    std::vector<std::pair<int, int>> directions = {
        {0, -2},  // North
        {0, 2},   // South
        {2, 0},   // East
        {-2, 0}   // West
    };

    // Shuffle directions manually using esp_random()
    for (int i = directions.size() - 1; i > 0; --i) {
        int j = esp_random() % (i + 1);
        std::swap(directions[i], directions[j]);
    }

    // Attempt to carve in each direction
    for (auto dir : directions) {
        int nx = x + dir.first;
        int ny = y + dir.second;

        // Ensure the new position is within bounds and is a wall
        if (nx > 0 && nx < ancho - 1 && ny > 0 && ny < largo - 1 && terreno[ny][nx] == 3) {
            terreno[ny][nx] = 0;
            terreno[y + dir.second / 2][x + dir.first / 2] = 0; // Remove intermediate wall
            tallarLaberinto(nx, ny);
        }
    }
}

// Inicializar el terreno y la serpiente
void iniciarTerreno() {
    //Cambiar de color el terreno para cada nivel
    //colorfondo = rand() % 4;
    //colorsnake = rand() % 2;

    // Ajustar dimensiones del laberinto para cada nivel
    //ancho = ancho_inicial + (nivel - 1) * 2; // Incrementar el tamaño del laberinto en cada nivel
    //largo = largo_inicial + (nivel - 1) * 2;

    // Asegurar que las dimensiones sean impares
    //if (ancho % 2 == 0) ancho++;
    //if (largo % 2 == 0) largo++;

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

// Actualizar el terreno con la posición de la serpiente y las manzanas
void actualizarTerreno() {
    //std::lock_guard<std::mutex> lock(terreno_mutex);

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