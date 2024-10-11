#include <sstream>
#include <iostream>
#include <vector> // Include vector header
#include <unistd.h>
#include <thread>
#include <chrono>

class TerminalUpdater {
public:
    void Print(std::ostream& out) const;
    void SetValue(size_t row, size_t col, int val); // Declare SetValue method

private:
    //std::vector<std::vector<int>> myGrid = {{1, 1, 1}, {1, 0, 1}, {1, 1, 1}}; // Example grid
    std::vector<std::vector<int>> myGrid = std::vector<std::vector<int>>(10, std::vector<int>(15, 0));
    //generation of the matrix that can be any type, you just need to define the x and y size and then enter the "Emptyness" value 
    //here the x value is 15 and the y value is 10 and its all filled with 0's
    //To call a value in the matrix myGrid[i][j] is used
};

int main(){

    //for ( int i=0; i < 100; ++i )
    //{
    //    std::cout << i << '\r';
    //    std::cout.flush(); // Ensure the output is flushed to the console
    //    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Add a delay to see the output
    //}

    TerminalUpdater updater;
    updater.Print(std::cout);
    sleep(5);
    updater.SetValue(1,1,1); // Change value in coords like this x,y,value -> this code changes the value in x = 1, y = 1 to one
    updater.Print(std::cout);
}

void TerminalUpdater::Print(std::ostream& out) const
{
    size_t size = 10; // Define size variable
    std::vector<int> vec(size, 0); // Correct vector initialization

    
    // Move the cursor to the top-left corner of the terminal
    out << "\033[H";

    for(size_t i = 0; i < myGrid.size(); i++)
    {
        for(size_t j = 0; j < myGrid[i].size(); j++)
        {
            out << myGrid[i][j] << " ";
        }
        out << "\n";
    }
    out.flush(); // Ensure the output is flushed to the console
}

void TerminalUpdater::SetValue(size_t row, size_t col, int val)
{
    myGrid[row % myGrid.size()][col % myGrid[0].size()] = val; // Ensure correct column indexing
}
/*
void TerminalUpdater::Print(std::ostream& out) const
{
    // Move the cursor to the top-left corner of the terminal
    out << "\033[H";

    for(size_t i = 0; i < myGrid.size(); i++)
    {
        for(size_t j = 0; j < myGrid[i].size(); j++)
        {
            out << myGrid[i][j] << " ";
        }
        out << "\n";
    }
    out.flush(); // Ensure the output is flushed to the console
}
*/