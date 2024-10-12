#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>

class TerminalUpdater {
public:
    void Print(std::ostream& out) const;
    void SetValue(size_t row, size_t col, int val);
    void GenerateMaze();

private:
    static const int GRID_HEIGHT = 11;
    static const int GRID_WIDTH = 16;

    std::vector<std::vector<int>> myGrid = std::vector<std::vector<int>>(GRID_HEIGHT, std::vector<int>(GRID_WIDTH, 3)); // 3 for walls
    std::vector<std::pair<int, int>> directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}}; // Right, Down, Left, Up

    void carvePath(int x, int y, int exitX, int exitY, bool &exitReached);
};

int main() {
    TerminalUpdater updater;
    updater.Print(std::cout);
    updater.GenerateMaze();
    updater.Print(std::cout);
}

void TerminalUpdater::Print(std::ostream& out) const {
    out << "\033[H"; // Move the cursor to the top-left corner of the terminal

    for (size_t i = 0; i < myGrid.size(); i++) {
        for (size_t j = 0; j < myGrid[i].size(); j++) {
            out << myGrid[i][j] << " ";
        }
        out << "\n";
    }
    out.flush(); // Ensure the output is flushed to the console
}

void TerminalUpdater::SetValue(size_t row, size_t col, int val) {
    myGrid[row % myGrid.size()][col % myGrid[0].size()] = val;
}

void TerminalUpdater::GenerateMaze() {
    std::srand(std::time(0));

    // Entry and Exit Points
    int entryY = 5 % myGrid.size();
    int exitY = 5 % myGrid.size();
    int exitX = 15 % myGrid[0].size();

    myGrid[entryY][0] = 5;  // Entry point
    myGrid[exitY][exitX] = 6; // Exit point

    // Initialize the grid with walls (3)
    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            if (!(y == entryY && (x == 0 || x == exitX))) {
                myGrid[y][x] = 3; // Wall
            }
        }
    }

    // Start carving the path from the entry point and ensure the exit is reached
    bool exitReached = false;
    carvePath(1, entryY, exitX, exitY, exitReached);
}

void TerminalUpdater::carvePath(int x, int y, int exitX, int exitY, bool &exitReached) {
    myGrid[y][x] = 0; // Carve out the path

    // Check if we have reached the exit
    if (x == exitX && y == exitY) {
        exitReached = true;
        return;
    }

    // Randomize the direction order
    std::vector<int> dirOrder = {0, 1, 2, 3};
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(dirOrder.begin(), dirOrder.end(), g);

    // Try carving in each direction
    for (int i : dirOrder) {
        int dx = directions[i].first;
        int dy = directions[i].second;

        int nx = x + 2 * dx; // Move by 2 cells
        int ny = y + 2 * dy;

        // Check bounds and if the next cell is a wall
        if (nx >= 1 && nx < GRID_WIDTH - 1 && ny >= 1 && ny < GRID_HEIGHT - 1 && myGrid[ny][nx] == 3) {
            // Carve path between current and next cell
            myGrid[y + dy][x + dx] = 0; // Carve the wall between
            carvePath(nx, ny, exitX, exitY, exitReached); // Recursively carve
            if (exitReached) return; // Stop if we have reached the exit
        }
    }
}
