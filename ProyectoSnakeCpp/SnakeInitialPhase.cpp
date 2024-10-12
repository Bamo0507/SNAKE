#include <atomic> // Para manejar variables compartidas de forma segura
#include <chrono>
#include <cstdlib> //random
#include <ctime>
#include <iostream>
#include <pthread.h> // Para el manejo de hilos
#include <termios.h> // Para manejar la entrada de teclado en Linux
#include <thread>
#include <unistd.h> // Para usar la función read
#include <vector>   //para la matriz

// Variables para el juego preliminar
int ancho = 16, largo = 11;
int appleCount = 1; // Modificar para que aumente con el nivel
std::vector<std::vector<int>>
    terreno; // 0 = terreno, 1 = manzana, 2 = serpiente (guía para identificar
             // qué representa cada uno)
std::atomic<bool> movimiento_completado(true); //variable para reconocer si la serpiente ya cambio su posición antes de leer otro input



// Estructura para manejar lógica de matriz
//--------------------------------------
struct Coordenada {
  int x, y;
};
//--------------------------------------

// LÓGICA POSICIONAL INICIAL
//---------------------------
std::vector<Coordenada> serpiente = {
    {5, 5}}; // Posición inicial de la serpiente (optimizar para que comience al
             // inicio del laberinto)
Coordenada direccion = {
    0,
    0}; // Variable para manejar hacia dónde se dirige la cabeza de la serpiente
Coordenada manzana; // Coordenadas de la manzana
//---------------------------

std::atomic<bool> game_over(false); // Variable de contol sobre estado de juego

// Declaración de funciones (RUTINAS)
//--------------------------------------
void *moverSerpiente(void *arg);
void *manejarInput(void *arg);
void *actualizarTerreno(void *arg);
//--------------------------------------

//--------------------------------------
// ALTERACIONES EN COMPORTAMIENTO DE LA TERMINAL
// AYUDA A TENER UN COMPORTAMIENTO DE JUEGO

// Configuración de comportamiento de la terminal
void configurarTerminal() {
  struct termios new_settings; // modifica la configuración de la entrada
  tcgetattr(0, &new_settings);
  new_settings.c_lflag &=
      ~ICANON; // Elimina método canónico (esperar enter para registrar un input
               // - toma todo lo que se apache como input)
  new_settings.c_lflag &=
      ~ECHO; // permite que lo que se typea no se vea en terminal
  tcsetattr(0, TCSANOW,
            &new_settings); // aplica los cambios descritos anteriormente
}

// Función para resetear la terminal
// Se mostrará lo que se typee, y se espera el enter
void restaurarTerminal() {
  struct termios default_settings;
  tcgetattr(0, &default_settings);
  default_settings.c_lflag |= ICANON;
  default_settings.c_lflag |= ECHO;
  tcsetattr(0, TCSANOW, &default_settings);
}
//--------------------------------------
//--------------------------------------

//-------------------------------------------------------------------------------------------------
// LÓGICA DEL JUEGO
// Funcion para generar el laberinto
void generarLaberitno(int width, int height){
  int startx = height/2;

    // Entry Point
    terreno[startx][0] = 5;
    //Exit Point
    terreno[startx][width-1] = 6;
    //Maze Generator w Exit and Entry point
    int Num_of_walls = (width*height)*0.4;

    int x, y; 
    for (int i = 0; i < Num_of_walls; i++) {

        while (true)
        {
          x = rand() % (width - 2);
          y = rand() % (height - 2); 
          if (terreno[x][y] != 0){
            terreno[y][x] = 3;
            break;
          }
          else{
            continue;
          }
        }
        
    }
}

// Función para generar manzanas en posiciones aleatorias
void generarManzana() {
  int x, y; //variables que servirán para las coordenadas de la manzana

  // Generar nuevas coordenadas hasta que no caigan en los bordes (valor 3) 
  do {
    x = rand() % (ancho - 2) + 1; // Excluir 0 y ancho-1 para evitar bordes
    y = rand() % (largo - 2) + 1; // Excluir 0 y largo-1 para evitar bordes
  } while (terreno[y][x] != 0); // Asegurarnos de que la posición esté libre (sin 1, 2 o 3)

  manzana = {x, y};
  terreno[y][x] = 1; // Colocar la manzana
}

// Generamos el terreno llenando todo con 0s (terreno vacío)
void iniciarTerreno() {
  terreno.resize(largo, std::vector<int>(ancho, 0));
  // Llenar los bordes del terreno con el valor 3 (bordes)
  for (int y = 0; y < largo; y++) {
    for (int x = 0; x < ancho; x++) {
      if (y == 0 || y == largo - 1 || x == 0 || x == ancho - 1) {
        terreno[y][x] = 3; // Borde de la matriz
      }
    }
  }
  generarLaberitno(ancho,largo);
  generarManzana(); // Generar la primera manzana
}

// Impresión de Terreno
void imprimirTerreno() {
  std::cout << "\033[H"; // Mueve el cursor a la esquina superior izquierda sin limpiar la pantalla

  // Recorremos la matriz completa
  for (int y = 0; y < largo; y++) {
    for (int x = 0; x < ancho; x++) {

      // posición inicial de la serpiente en 'x' y en 'y' con un color diferente
      if (serpiente[0].x == x && serpiente[0].y == y) {
        std::cout << "\033[48;5;10m  \033[0m"; // Color para la cabeza de la serpiente (verde brillante)
      }

      // Si en el terreno se ubica un 2 se trata del cuerpo de la serpiente
      else if (terreno[y][x] == 2) {
        std::cout << "\033[48;5;28m  \033[0m"; // Tono de verde para el cuerpo de la serpiente
      }

      // Si en el terreno se tiene un 1 se trata de la manzana
      else if (terreno[y][x] == 1) {
        std::cout << "\033[48;5;9m  \033[0m"; // Manzana roja
      }

      //Código para los bordes del terreno
      else if (terreno[y][x] == 3){
        std::cout << "\033[48;5;80m  \033[0m"; // Fondo de color azul
      }

      // Si no se encuentra nada en el terreno se muestra un 0 y por ende otro color
      else {
        std::cout << "\033[48;5;51m  \033[0m"; // Fondo de color aqua
      }
    }
    std::cout << std::endl;
  }
  std::cout.flush(); // Ayuda a que los cambios en terminal se muestren inmediatamente
}


// Limpia el terreno, esto se hace constantemente para simular el movimiento
void actualizarTerreno() {
  // Limpiar el terreno antes de actualizar
  for (int y = 0; y < largo; y++) {
    for (int x = 0; x < ancho; x++) {
      if (terreno[y][x] == 2)
        terreno[y][x] = 0;
    }
  }
  // Colocar la serpiente en el terreno
  for (auto &segmento : serpiente) {
    terreno[segmento.y][segmento.x] = 2;
  }
  // Colocar la manzana
  terreno[manzana.y][manzana.x] = 1;
}

// Función para mover la serpiente (es rutina)
void *moverSerpiente(void *arg) {
  // ciclo para que funcione mientras no se ha pérdido
  while (!game_over) {
    // Se inicializa la posición de la cabeza en una posición determinada
    Coordenada nueva_cabeza = {serpiente[0].x + direccion.x,
                               serpiente[0].y + direccion.y};

    // Manejo para cuando estamos en un punto fuera de la matriz
    if (nueva_cabeza.x < 1 || nueva_cabeza.x >= ancho-1 || nueva_cabeza.y < 1 ||
        nueva_cabeza.y >= largo-1) {
      std::cout << R"(
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼
███▀▀▀██┼███▀▀▀███┼███▀█▄█▀███┼██▀▀▀
██┼┼┼┼██┼██┼┼┼┼┼██┼██┼┼┼█┼┼┼██┼██┼┼┼
██┼┼┼▄▄▄┼██▄▄▄▄▄██┼██┼┼┼▀┼┼┼██┼██▀▀▀
██┼┼┼┼██┼██┼┼┼┼┼██┼██┼┼┼┼┼┼┼██┼██┼┼┼
███▄▄▄██┼██┼┼┼┼┼██┼██┼┼┼┼┼┼┼██┼██▄▄▄
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼
███▀▀▀███┼▀███┼┼██▀┼██▀▀▀┼██▀▀▀▀██▄┼
██┼┼┼┼┼██┼┼┼██┼┼██┼┼██┼┼┼┼██┼┼┼┼┼██┼
██┼┼┼┼┼██┼┼┼██┼┼██┼┼██▀▀▀┼██▄▄▄▄▄▀▀┼
██┼┼┼┼┼██┼┼┼██┼┼█▀┼┼██┼┼┼┼██┼┼┼┼┼██┼
███▄▄▄███┼┼┼─▀█▀┼┼─┼██▄▄▄┼██┼┼┼┼┼██▄
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼
      )";

      std::cout << R"(
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼█┼┼┼┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼███┼┼┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼████┼┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼██████┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼████████┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼██████████┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼████████████┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼██████████████┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼█████████████┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼████████████┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼█████████┼┼┼┼┼┼┼┼
      )";
      
      game_over = true;
      pthread_exit(0);
    }

    // Detectar colisión de la cabeza con el cuerpo
    for (size_t i = 1; i < serpiente.size(); i++) {
      if (nueva_cabeza.x == serpiente[i].x && nueva_cabeza.y == serpiente[i].y) {
        std::cout << R"(
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼
███▀▀▀██┼███▀▀▀███┼███▀█▄█▀███┼██▀▀▀
██┼┼┼┼██┼██┼┼┼┼┼██┼██┼┼┼█┼┼┼██┼██┼┼┼
██┼┼┼▄▄▄┼██▄▄▄▄▄██┼██┼┼┼▀┼┼┼██┼██▀▀▀
██┼┼┼┼██┼██┼┼┼┼┼██┼██┼┼┼┼┼┼┼██┼██┼┼┼
███▄▄▄██┼██┼┼┼┼┼██┼██┼┼┼┼┼┼┼██┼██▄▄▄
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼
███▀▀▀███┼▀███┼┼██▀┼██▀▀▀┼██▀▀▀▀██▄┼
██┼┼┼┼┼██┼┼┼██┼┼██┼┼██┼┼┼┼██┼┼┼┼┼██┼
██┼┼┼┼┼██┼┼┼██┼┼██┼┼██▀▀▀┼██▄▄▄▄▄▀▀┼
██┼┼┼┼┼██┼┼┼██┼┼█▀┼┼██┼┼┼┼██┼┼┼┼┼██┼
███▄▄▄███┼┼┼─▀█▀┼┼─┼██▄▄▄┼██┼┼┼┼┼██▄
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼
          )";

          std::cout << R"(
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼█┼┼┼┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼███┼┼┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼████┼┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼██████┼┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼████████┼┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼██████████┼┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼████████████┼┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼██████████████┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼█████████████┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼████████████┼┼┼┼┼┼┼
┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼┼█████████┼┼┼┼┼┼┼┼
          )";        
        game_over = true;
        pthread_exit(0);
      }
    }

    // Detectar si la serpiente comió una manzana - Si está en misma posición
    // detectada por la estructura coordenada
    if (nueva_cabeza.x == manzana.x && nueva_cabeza.y == manzana.y) {
      serpiente.push_back(
          {-1, -1}); // Agrega un nuevo segmento a la parte 'final' del vector,
                     // ya se reconoce como 2 al actualizar terreno
      generarManzana(); // Generar una nueva manzana
    }

    // Mover el cuerpo de la serpiente
    for (int i = serpiente.size() - 1; i > 0; i--) {
      serpiente[i] = serpiente[i - 1];
    }
    serpiente[0] = nueva_cabeza; // Actualizamos la posición actual de la cabeza
                                 // (lógica de color ya implementada)

    movimiento_completado = true; //registramos el cambio de posición

    // Lograr flujo de juego
    actualizarTerreno(); // Actualizar el terreno después del movimiento
    imprimirTerreno();   // Imprimir el terreno con los nuevos cambios

    // Modifica velocidad del juego
    // se maneja como un refrescamiento de pantalla (podríamos colocar una reducción gradual con cada nivel que se complete)
    std::this_thread::sleep_for(
        std::chrono::milliseconds(200)
      ); // Velocidad más rápida
  }

  pthread_exit(0);
}
      
// Rutina para manejar inputs del usuario
void *manejarInput(void *arg) {
  char tecla;
  // Estará vigente mientras no se haya perdido
  while (!game_over) {
    ssize_t bytes_read = read(0, &tecla, 1); // Lee una tecla sin bloquear
    if (bytes_read > 0) {
      // Solo permitir el cambio de dirección si el movimiento anterior se completó
      if (movimiento_completado.load()) {
        movimiento_completado = false; // Se deshabilita hasta que la serpiente se mueva

        // Si la serpiente está en movimiento (ya se apachó algo como w, a, s, d)
        if (direccion.x != 0 || direccion.y != 0) {
          // Si la serpiente se está moviendo horizontalmente (izquierda/derecha)
          if (direccion.x != 0) {
            // Solo permite movimiento arriba o abajo (w o s)
            if (tecla == 'w') {
              direccion = {0, -1}; // Movimiento hacia arriba
            } else if (tecla == 's') {
              direccion = {0, 1}; // Movimiento hacia abajo
            }
          }
          // Si la serpiente se está moviendo verticalmente (arriba/abajo)
          else if (direccion.y != 0) {
            // Solo permite movimiento a la izquierda o derecha (a o d)
            if (tecla == 'a') {
              direccion = {-1, 0}; // Movimiento hacia la izquierda
            } else if (tecla == 'd') {
              direccion = {1, 0}; // Movimiento hacia la derecha
            }
          }
        } else {
          // Si la serpiente no se ha movido aún, permitir cualquier dirección
          switch (tecla) {
          case 'w':
            direccion = {0, -1};
            break; // Movimiento hacia arriba
          case 's':
            direccion = {0, 1};
            break; // Movimiento hacia abajo
          case 'a':
            direccion = {-1, 0};
            break; // Movimiento hacia la izquierda
          case 'd':
            direccion = {1, 0};
            break; // Movimiento hacia la derecha
          }
        }
      }
    }
  }
  pthread_exit(0);
}

// Actualizar Terreno (VERSIÓN RUTINA)
// Esta ayuda a que las impresiones en la terminal se realicen en la terminal de
// forma paralela Ayuda con la fluidez - impresiones continuas y optimizadas
void *actualizarTerreno(void *arg) {
  while (!game_over) {
    actualizarTerreno();
    imprimirTerreno();
    std::this_thread::sleep_for(std::chrono::milliseconds(
      250)); // Velocidad de actualización del terreno
  }
  pthread_exit(0);
}
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------

// FUNCIÓN MAIN
//---------------------------------------------------------------------------
int main() {
  srand(time(0)); // Semilla para la generación aleatoria

  system("clear"); //Limpiar totalmente la terminal
  
  iniciarTerreno();

  // Configurar la terminal en modo no canónico
  configurarTerminal();

  pthread_t hilo_movimiento, hilo_input, hilo_actualizacion;

  // Crear hilos para manejar la serpiente, el input y la actualización del
  // terreno
  pthread_create(&hilo_movimiento, NULL, moverSerpiente, NULL);
  pthread_create(&hilo_input, NULL, manejarInput, NULL);
  pthread_create(&hilo_actualizacion, NULL, actualizarTerreno, NULL);

  // Esperar a que los hilos terminen
  pthread_join(hilo_movimiento, NULL);
  pthread_join(hilo_input, NULL);
  pthread_join(hilo_actualizacion, NULL);

  // Restaurar la configuración estándar de la terminal antes de salir
  restaurarTerminal();

  return 0;
}
//---------------------------------------------------------------------------
