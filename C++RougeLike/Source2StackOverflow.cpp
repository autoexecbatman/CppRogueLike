#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

class Maze {
public:
    void load(std::string&& mazeFilePath);

    void run();

private:
    using Grid = std::vector<std::string>;

    enum class MoveDirection {
        UP,
        DOWN,
        RIGHT,
        LEFT
    };

    enum class MoveResult {
        OK,
        OUT_OF_BOUNDS,
        COLLISION,
        GOAL
    };

    struct Position {
        int row;
        int col;
    };

    void display() const;

    MoveResult movePlayer(MoveDirection direction);

    Grid m_grid;
    Position m_playerPosition;
    Position m_goalPosition;

};

void Maze::load(std::string&& mazeFilePath) {
    m_grid.clear();

    std::ifstream mazeFileStream(mazeFilePath); //todo - readonly flag

    int currentRow = 0;

    for (std::string line; std::getline(mazeFileStream, line);) {
        int currentCol = 0;

        std::string row;

        std::copy_if(std::begin(line), std::end(line), std::back_inserter(row), [&](decltype(row)::value_type c) {
            switch (c) {
            case 'i':
                m_playerPosition.row = currentRow;
                m_playerPosition.col = currentCol;
                break;
            case 'g':
                m_goalPosition.row = currentRow;
                m_goalPosition.col = currentCol;
                break;
            default:
                break;
            }

            ++currentCol;

            return true;
            });

        m_grid.emplace_back(std::move(row));

        ++currentRow;
    }
}

void Maze::display() const {
    std::copy(std::begin(m_grid), std::end(m_grid), std::ostream_iterator<std::string>(std::cout, "\n"));
}

void Maze::run() {
    bool running = true;

    char key;

    while (running) {
        display();

        MoveResult moveResult;

        std::cin >> key;

        switch (key) {
        case 'w':
            moveResult = movePlayer(MoveDirection::UP);
            break;
        case 'a':
            moveResult = movePlayer(MoveDirection::LEFT);
            break;
        case 's':
            moveResult = movePlayer(MoveDirection::DOWN);
            break;
        case 'd':
            moveResult = movePlayer(MoveDirection::RIGHT);
            break;
        default:
            std::cerr << "Please use WASD keys to move player" << std::endl;
            break;
        }

        switch (moveResult) {
        case MoveResult::OUT_OF_BOUNDS:
            running = false;
            std::cout << "failure (out of bounds) - game over" << std::endl;
            break;
        case MoveResult::COLLISION:
            running = false;
            std::cout << "failure (collision) - game over" << std::endl;
            break;
        case MoveResult::GOAL:
            running = false;
            std::cout << "success - game over" << std::endl;
            break;
        default:
            break;
        }
    }
}

Maze::MoveResult Maze::movePlayer(Maze::MoveDirection direction) {
    Position previousPlayerPosition = m_playerPosition;

    switch (direction) {
    case MoveDirection::UP:
        m_playerPosition.row -= 1;
        break;
    case MoveDirection::LEFT:
        m_playerPosition.col -= 1;
        break;
    case MoveDirection::DOWN:
        m_playerPosition.row += 1;
        break;
    case MoveDirection::RIGHT:
        m_playerPosition.col += 1;
    }

    //check bounds
    try {
        m_grid.at(m_playerPosition.row).at(m_playerPosition.col);
    }
    catch (const std::out_of_range exc) {
        return MoveResult::OUT_OF_BOUNDS;
    }

    //check collision
    if (m_grid[m_playerPosition.row][m_playerPosition.col] == 'x') {
        return MoveResult::COLLISION;
    }
    //check goal
    else if (m_grid[m_playerPosition.row][m_playerPosition.col] == 'g') {
        return MoveResult::GOAL;
    }

    m_grid[previousPlayerPosition.row][previousPlayerPosition.col] = ' ';
    m_grid[m_playerPosition.row][m_playerPosition.col] = 'i';

    return MoveResult::OK;
}

int main() {
    auto maze = std::unique_ptr<Maze>(new Maze);
    maze->load("maze1.txt");
    maze->run();
}