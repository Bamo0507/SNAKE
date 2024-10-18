#include <iostream>

int main(){
    
    int Fondo[5] = {51, 63, 184, 214, 177};
    int Paredes[5] = {80, 97, 136, 130, 92};
    int Manzana[3] = {9, 88, 197};
    int Snake[3] = {28, 77, 84};
    int CabezaSnake[3] = {10, 106, 65};

    //Colors for background
    std::cout << "\033[48;5;51m  \033[0m"; // Celeste
    std::cout << "\033[48;5;63m  \033[0m"; // Lavada
    std::cout << "\033[48;5;184m  \033[0m"; // Amarillo
    std::cout << "\033[48;5;214m  \033[0m"; // Naranja
    std::cout << "\033[48;5;177m  \033[0m"; // Violeta 


    std::cout << std::endl;

    //Complementary Colors for Background
    std::cout << "\033[48;5;80m  \033[0m";//Celeste oscuro
    std::cout << "\033[48;5;97m  \033[0m"; // Lavada oscuro
    std::cout << "\033[48;5;136m  \033[0m"; // Gold
    std::cout << "\033[48;5;130m  \033[0m"; // Naranja Oscuro
    std::cout << "\033[48;5;92m  \033[0m"; // Violeta Oscuro


    std::cout << std::endl;

    //Apple Colors
    std::cout << "\033[48;5;9m  \033[0m"; // Rojo
    std::cout << "\033[48;5;88m  \033[0m"; // Rojo Oscuro
    std::cout << "\033[48;5;197m  \033[0m"; // Rosa Profundo


    std::cout << std::endl;

    //Snake Colors
    std::cout << "\033[48;5;28m  \033[0m"; // Verde Claro
    std::cout << "\033[48;5;77m  \033[0m"; // Verde palido
    std::cout << "\033[48;5;84m  \033[0m"; // Verde marino


    std::cout << std::endl;

    //Snake Head Colors
    std::cout << "\033[48;5;10m  \033[0m"; // Verde Oscuro
    std::cout << "\033[48;5;106m  \033[0m"; // Verde amarilloso
    std::cout << "\033[48;5;65m  \033[0m"; // Verde marino oscuro


    std::cout << std::endl;



    

}


