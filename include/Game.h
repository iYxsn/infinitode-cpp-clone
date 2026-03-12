#ifndef GAME_H
#define GAME_H

#include "App.h"
#include "Entity.h"
#include "Input.h"
#include "Map.h"
#include "Renderer.h"
#include <SDL.h>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Game
{
public:
    // Creates the game state before SDL initialization
    Game();

    // Initializes subsystems and game data
    bool init();
    // Runs the main loop until quit
    void run();

private:
    // Reads SDL events and game controls
    void processEvents();
    // Handles keyboard controls
    void handleKeyDown(SDL_Keycode key);
    // Handles mouse placement and GUI clicks
    void handleMouseButton(const SDL_MouseButtonEvent& button);

    // Updates simulation
    void update(float dt);
    // Updates wave spawning logic
    void updateWave(float dt);
    // Updates enemies, towers and projectiles
    void updateEntities(float dt);
    // Resolves projectile/enemy collisions
    void resolveCombat();
    // Removes inactive entities and applies rewards/penalties
    void cleanupEntities();

    // Renders full frame
    void render();
    // Draws minimal side HUD
    void drawHud();
    // Draws map cursor used with arrows
    void drawCursor();

    // Builds world path from map path cells
    void buildWorldPath();
    // Starts a new wave when requested
    void startNextWave();
    // Creates enemy sequence for one wave
    std::vector<EnemyType> buildWave(int waveNumber) const;

    // Tries to place/upgrade towers on map slots
    bool tryPlaceTower(const SDL_Point& cell, TowerType type);
    bool tryUpgradeTower(const SDL_Point& cell);
    // Returns tower index on one cell if present
    std::optional<std::size_t> findTowerAtCell(const SDL_Point& cell) const;

    // Converts between map cells and world pixels
    SDL_FPoint cellToWorld(const SDL_Point& cell) const;
    SDL_Point screenToCell(int x, int y) const;
    // Returns true if point is inside map render area
    bool isInsideMapPixels(int x, int y) const;

    // Returns selected tower type for key 1 to 5
    std::optional<TowerType> towerTypeFromKey(SDL_Keycode key) const;
    // Returns display color for one tower type
    SDL_Color towerColor(TowerType type) const;

    App app_;
    Input input_;
    Map map_;

    std::vector<SDL_FPoint> worldPath_;
    std::vector<EnemyType> waveEnemies_;
    std::size_t waveSpawnIndex_;
    float waveSpawnTimer_;
    bool waveRunning_;
    bool paused_;

    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<std::unique_ptr<Tower>> towers_;
    std::vector<std::unique_ptr<Projectile>> projectiles_;

    TowerType selectedTowerType_;
    SDL_Point cursorCell_;

    int money_;
    int lives_;
    int waveNumber_;

    float titleRefreshTimer_;

    std::unique_ptr<Renderer> renderer_;
    bool running_;
};

#endif
