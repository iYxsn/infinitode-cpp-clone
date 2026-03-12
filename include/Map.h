#ifndef MAP_H
#define MAP_H

#include <SDL.h>
#include <string>
#include <vector>

class Renderer;

class Map
{
public:
    // Builds an empty map
    Map();

    // Loads and parses an ASCII map file
    bool loadFromFile(const std::string& path);
    // Draws the grid using simple colors by tile type
    void draw(Renderer& renderer, int offsetX, int offsetY, int cellSize) const;
    // Returns computed path cells from S to E.
    const std::vector<SDL_Point>& path() const;
    // Returns true if (col,row) is inside map bounds
    bool isInside(int col, int row) const;
    // Returns true when tile is a reserved tower slot
    bool isTowerSlot(int col, int row) const;
    // Returns tile character at (col,row), or '.' if invalid
    char tileAt(int col, int row) const;

    // Returns number of rows in loaded map
    int rows() const;
    // Returns number of columns in loaded map
    int cols() const;

private:
    // Resets map content before loading
    void reset();
    // Computes a path from start to end on walkable cells
    bool buildPath();
    // Validates allowed symbols used in map file
    bool isAllowedTile(char tile) const;
    // Returns true for tiles enemies can walk on
    bool isWalkableTile(char tile) const;

    std::vector<std::string> grid_;
    std::vector<SDL_Point> path_;
    SDL_Point start_;
    SDL_Point end_;
    bool hasStart_;
    bool hasEnd_;
};

#endif
