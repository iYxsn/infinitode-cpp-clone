#include "Map.h"
#include "Renderer.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <queue>

Map::Map() : grid_(), path_(), start_{0, 0}, end_{0, 0}, hasStart_(false), hasEnd_(false)
{
    // Map starts empty until file load.
}

bool Map::loadFromFile(const std::string& path)
{
    // Start from a clean state for every load.
    reset();

    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        // Handle Windows line endings if present.
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
        {
            continue;
        }

        // Enforce rectangular map.
        if (!grid_.empty() && line.size() != grid_.front().size())
        {
            reset();
            return false;
        }

        const bool all_allowed = std::all_of(
            line.begin(),
            line.end(),
            [this](char tile) { return isAllowedTile(tile); }
        );

        if (!all_allowed)
        {
            reset();
            return false;
        }

        const int row = static_cast<int>(grid_.size());
        for (int col = 0; col < static_cast<int>(line.size()); ++col)
        {
            if (line[col] == 'S')
            {
                if (hasStart_)
                {
                    reset();
                    return false;
                }
                start_ = {col, row};
                hasStart_ = true;
            }
            else if (line[col] == 'E')
            {
                if (hasEnd_)
                {
                    reset();
                    return false;
                }
                end_ = {col, row};
                hasEnd_ = true;
            }
        }

        grid_.push_back(line);
    }

    // One start/end and a valid path are mandatory.
    if (grid_.empty() || !hasStart_ || !hasEnd_)
    {
        return false;
    }

    if (!buildPath())
    {
        reset();
        return false;
    }

    return true;
}

void Map::draw(Renderer& renderer, int offsetX, int offsetY, int cellSize) const
{
    // Draw each tile as a colored rectangle.
    for (int row = 0; row < rows(); ++row)
    {
        for (int col = 0; col < cols(); ++col)
        {
            const char tile = grid_[row][col];
            const int x = offsetX + col * cellSize;
            const int y = offsetY + row * cellSize;

            if (tile == '.')
            {
                renderer.fillRect(x, y, cellSize, cellSize, 34, 90, 40);
            }
            else if (tile == '#')
            {
                renderer.fillRect(x, y, cellSize, cellSize, 156, 130, 92);
            }
            else if (tile == 'S')
            {
                renderer.fillRect(x, y, cellSize, cellSize, 40, 90, 220);
            }
            else if (tile == 'E')
            {
                renderer.fillRect(x, y, cellSize, cellSize, 220, 70, 70);
            }
            else if (tile == 'T')
            {
                renderer.fillRect(x, y, cellSize, cellSize, 120, 90, 180);
            }

            renderer.drawRect(x, y, cellSize, cellSize, 20, 20, 20);
        }
    }
}

int Map::rows() const
{
    // Number of text lines in the map.
    return static_cast<int>(grid_.size());
}

const std::vector<SDL_Point>& Map::path() const
{
    // Read-only path access used by enemy movement.
    return path_;
}

bool Map::isInside(int col, int row) const
{
    // Validates coordinates against map dimensions.
    return row >= 0 && row < rows() && col >= 0 && col < cols();
}

bool Map::isTowerSlot(int col, int row) const
{
    // Tower can be placed only on reserved slot tiles.
    if (!isInside(col, row))
    {
        return false;
    }

    return grid_[row][col] == 'T';
}

char Map::tileAt(int col, int row) const
{
    // Safe access to one map tile.
    if (!isInside(col, row))
    {
        return '.';
    }

    return grid_[row][col];
}

int Map::cols() const
{
    // Number of symbols per map line.
    if (grid_.empty())
    {
        return 0;
    }
    return static_cast<int>(grid_.front().size());
}

void Map::reset()
{
    // Restore default empty state.
    grid_.clear();
    path_.clear();
    start_ = {0, 0};
    end_ = {0, 0};
    hasStart_ = false;
    hasEnd_ = false;
}

bool Map::buildPath()
{
    // BFS on walkable cells to find shortest path.
    if (rows() == 0 || cols() == 0)
    {
        return false;
    }

    const int width = cols();
    const int height = rows();
    const int total = width * height;
    const int start_id = start_.y * width + start_.x;
    const int end_id = end_.y * width + end_.x;

    std::vector<int> previous(total, -1);
    std::vector<bool> visited(total, false);
    std::queue<int> frontier;

    visited[start_id] = true;
    frontier.push(start_id);

    const std::array<int, 4> dx = {1, -1, 0, 0};
    const std::array<int, 4> dy = {0, 0, 1, -1};

    while (!frontier.empty())
    {
        const int current = frontier.front();
        frontier.pop();

        if (current == end_id)
        {
            break;
        }

        const int cx = current % width;
        const int cy = current / width;

        for (int i = 0; i < 4; ++i)
        {
            const int nx = cx + dx[i];
            const int ny = cy + dy[i];

            if (nx < 0 || nx >= width || ny < 0 || ny >= height)
            {
                continue;
            }

            const int next = ny * width + nx;
            if (visited[next] || !isWalkableTile(grid_[ny][nx]))
            {
                continue;
            }

            visited[next] = true;
            previous[next] = current;
            frontier.push(next);
        }
    }

    if (!visited[end_id])
    {
        return false;
    }

    path_.clear();
    for (int id = end_id; id != -1; id = previous[id])
    {
        const int x = id % width;
        const int y = id / width;
        path_.push_back(SDL_Point{x, y});
    }

    std::reverse(path_.begin(), path_.end());
    return !path_.empty();
}

bool Map::isAllowedTile(char tile) const
{
    // Minimal set for Step 2.
    return tile == '.' || tile == '#' || tile == 'S' || tile == 'E' || tile == 'T';
}

bool Map::isWalkableTile(char tile) const
{
    // Enemies can walk on path/start/end.
    return tile == '#' || tile == 'S' || tile == 'E';
}
