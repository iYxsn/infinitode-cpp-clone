#include "Game.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace
{
// Shared rendering constants.
constexpr int kWindowWidth = 960;
constexpr int kWindowHeight = 540;
constexpr int kMapOffsetX = 20;
constexpr int kMapOffsetY = 20;
constexpr int kCellSize = 32;
constexpr int kHudPadding = 20;
constexpr int kTowerButtonSize = 44;
} // namespace

Game::Game()
    : app_(),
      input_(),
      map_(),
      worldPath_(),
      waveEnemies_(),
      waveSpawnIndex_(0),
      waveSpawnTimer_(0.0f),
      waveRunning_(false),
      paused_(false),
      enemies_(),
      towers_(),
      projectiles_(),
      selectedTowerType_(TowerType::Basic),
      cursorCell_{0, 0},
      money_(220),
      lives_(20),
      waveNumber_(0),
      titleRefreshTimer_(0.0f),
      renderer_(nullptr),
      running_(false)
{
    // Game starts stopped until init succeeds.
}

bool Game::init()
{
    // Create SDL app resources.
    if (!app_.init("Infinitode Lite", kWindowWidth, kWindowHeight))
    {
        return false;
    }

    // Build render helper once renderer exists.
    renderer_ = std::make_unique<Renderer>(app_.renderer());

    // Load initial ASCII map.
    if (!map_.loadFromFile("assets/maps/map1.txt"))
    {
        return false;
    }

    // Build path used by enemy movement.
    buildWorldPath();

    // Put cursor on first tower slot if one exists.
    for (int row = 0; row < map_.rows(); ++row)
    {
        for (int col = 0; col < map_.cols(); ++col)
        {
            if (map_.isTowerSlot(col, row))
            {
                cursorCell_ = SDL_Point{col, row};
                row = map_.rows();
                break;
            }
        }
    }

    running_ = true;
    return true;
}

void Game::run()
{
    // Convert SDL ticks (ms) to seconds for dt.
    const float msToSec = 0.001f;
    Uint32 last = SDL_GetTicks();

    while (running_)
    {
        const Uint32 now = SDL_GetTicks();
        const float dt = static_cast<float>(now - last) * msToSec;
        last = now;

        processEvents();
        update(dt);
        render();
    }
}

void Game::processEvents()
{
    // Consume all pending SDL events this frame.
    SDL_Event event;
    while (app_.pollEvent(event))
    {
        input_.update(event);

        if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
        {
            handleKeyDown(event.key.keysym.sym);
        }

        if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            handleMouseButton(event.button);
        }
    }

    if (input_.quitRequested())
    {
        running_ = false;
    }
}

void Game::handleKeyDown(SDL_Keycode key)
{
    // Toggle pause with space.
    if (key == SDLK_SPACE)
    {
        paused_ = !paused_;
        return;
    }

    // Start next wave with Enter.
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
    {
        startNextWave();
        return;
    }

    // Select tower type with keys 1..5.
    const std::optional<TowerType> type = towerTypeFromKey(key);
    if (type.has_value())
    {
        selectedTowerType_ = *type;
        return;
    }

    // Place tower on selected cell.
    if (key == SDLK_b)
    {
        tryPlaceTower(cursorCell_, selectedTowerType_);
        return;
    }

    // Upgrade tower on selected cell.
    if (key == SDLK_u)
    {
        tryUpgradeTower(cursorCell_);
        return;
    }

    // Move map selection cursor with arrows.
    if (key == SDLK_LEFT)
    {
        cursorCell_.x = std::max(0, cursorCell_.x - 1);
        return;
    }
    if (key == SDLK_RIGHT)
    {
        cursorCell_.x = std::min(map_.cols() - 1, cursorCell_.x + 1);
        return;
    }
    if (key == SDLK_UP)
    {
        cursorCell_.y = std::max(0, cursorCell_.y - 1);
        return;
    }
    if (key == SDLK_DOWN)
    {
        cursorCell_.y = std::min(map_.rows() - 1, cursorCell_.y + 1);
        return;
    }
}

void Game::handleMouseButton(const SDL_MouseButtonEvent& button)
{
    // Left-click on map: select cell then try to place tower.
    if (button.button == SDL_BUTTON_LEFT && isInsideMapPixels(button.x, button.y))
    {
        const SDL_Point cell = screenToCell(button.x, button.y);
        cursorCell_ = cell;
        tryPlaceTower(cell, selectedTowerType_);
        return;
    }

    // Right-click on map: try to upgrade tower on clicked cell.
    if (button.button == SDL_BUTTON_RIGHT && isInsideMapPixels(button.x, button.y))
    {
        const SDL_Point cell = screenToCell(button.x, button.y);
        cursorCell_ = cell;
        tryUpgradeTower(cell);
        return;
    }

    // Left-click on HUD buttons: select tower or control wave/pause.
    if (button.button != SDL_BUTTON_LEFT)
    {
        return;
    }

    const int panelX = kMapOffsetX + map_.cols() * kCellSize + kHudPadding;
    const int firstButtonY = 40;
    const int stepY = 56;

    for (int i = 0; i < 5; ++i)
    {
        const int x0 = panelX;
        const int y0 = firstButtonY + i * stepY;
        const bool inside = button.x >= x0 && button.x <= x0 + kTowerButtonSize &&
                            button.y >= y0 && button.y <= y0 + kTowerButtonSize;
        if (inside)
        {
            selectedTowerType_ = static_cast<TowerType>(i);
            return;
        }
    }

    const int startX = panelX;
    const int startY = 350;
    const int startW = 120;
    const int startH = 34;
    const bool clickStart = button.x >= startX && button.x <= startX + startW &&
                            button.y >= startY && button.y <= startY + startH;
    if (clickStart)
    {
        startNextWave();
        return;
    }

    const int pauseX = panelX;
    const int pauseY = 392;
    const int pauseW = 120;
    const int pauseH = 34;
    const bool clickPause = button.x >= pauseX && button.x <= pauseX + pauseW &&
                            button.y >= pauseY && button.y <= pauseY + pauseH;
    if (clickPause)
    {
        paused_ = !paused_;
    }
}

void Game::update(float dt)
{
    // Freeze gameplay when paused, but keep rendering and input.
    if (!paused_ && lives_ > 0)
    {
        updateWave(dt);
        updateEntities(dt);
        resolveCombat();
        cleanupEntities();
    }

    // Keep title refreshed with current game stats.
    titleRefreshTimer_ += dt;
    if (titleRefreshTimer_ >= 0.2f)
    {
        std::ostringstream title;
        title << "Infinitode Lite | Wave " << waveNumber_
              << " | Money " << money_
              << " | Lives " << lives_
              << " | Enemies " << enemies_.size();

        if (paused_)
        {
            title << " | PAUSE";
        }
        if (lives_ <= 0)
        {
            title << " | GAME OVER";
        }

        app_.setWindowTitle(title.str());
        titleRefreshTimer_ = 0.0f;
    }
}

void Game::updateWave(float dt)
{
    // Do nothing if no wave is running.
    if (!waveRunning_)
    {
        return;
    }

    waveSpawnTimer_ += dt;

    // Spawn one enemy at fixed interval.
    const float spawnInterval = 0.8f;
    if (waveSpawnIndex_ < waveEnemies_.size() && waveSpawnTimer_ >= spawnInterval)
    {
        waveSpawnTimer_ = 0.0f;
        enemies_.push_back(std::make_unique<Enemy>(waveEnemies_[waveSpawnIndex_], worldPath_));
        ++waveSpawnIndex_;
    }

    // Wave ends when queue is exhausted and all enemies are gone.
    if (waveSpawnIndex_ >= waveEnemies_.size() && enemies_.empty())
    {
        waveRunning_ = false;
        money_ += 35 + waveNumber_ * 6;
    }
}

void Game::updateEntities(float dt)
{
    // Update timers/movement for all entities.
    for (const auto& tower : towers_)
    {
        tower->update(dt);
    }
    for (const auto& enemy : enemies_)
    {
        enemy->update(dt);
    }
    for (const auto& projectile : projectiles_)
    {
        projectile->update(dt);
    }

    // Tower targeting and shooting.
    for (const auto& tower : towers_)
    {
        std::optional<std::size_t> targetIndex;
        float bestDistSq = 1.0e12f;

        for (std::size_t i = 0; i < enemies_.size(); ++i)
        {
            const Enemy& enemy = *enemies_[i];
            if (!enemy.isActive())
            {
                continue;
            }

            if (tower->antiAir() && !enemy.isFlying())
            {
                continue;
            }
            if (!tower->antiAir() && enemy.isFlying())
            {
                continue;
            }

            const float dx = enemy.position().x - tower->position().x;
            const float dy = enemy.position().y - tower->position().y;
            const float distSq = dx * dx + dy * dy;

            const float rangeSq = tower->range() * tower->range();
            if (distSq <= rangeSq && distSq < bestDistSq)
            {
                bestDistSq = distSq;
                targetIndex = i;
            }
        }

        if (!targetIndex.has_value())
        {
            continue;
        }

        const Enemy& target = *enemies_[*targetIndex];
        tower->aimToward(target.position(), dt);

        if (!tower->canShoot())
        {
            continue;
        }

        projectiles_.push_back(std::make_unique<Projectile>(
            tower->position(),
            tower->angleRad(),
            tower->projectileSpeed(),
            tower->damage(),
            tower->antiAir()
        ));

        tower->onShot();
    }
}

void Game::resolveCombat()
{
    // Check projectile/enemy impacts.
    for (const auto& projectile : projectiles_)
    {
        if (!projectile->isActive())
        {
            continue;
        }

        for (const auto& enemy : enemies_)
        {
            if (!enemy->isActive())
            {
                continue;
            }

            if (!projectile->canHitEnemy(*enemy))
            {
                continue;
            }

            if (!projectile->collidesWith(*enemy))
            {
                continue;
            }

            enemy->applyDamage(projectile->damage());
            projectile->consume();
            break;
        }
    }
}

void Game::cleanupEntities()
{
    // Apply rewards and life loss before removing inactive enemies.
    int reachedEndCount = 0;
    int killedCount = 0;

    for (const auto& enemy : enemies_)
    {
        if (enemy->isActive())
        {
            continue;
        }

        if (enemy->reachedEnd())
        {
            ++reachedEndCount;
        }
        else
        {
            ++killedCount;
        }
    }

    lives_ = std::max(0, lives_ - reachedEndCount);
    money_ += killedCount * 12;

    // Stop spawning when player is dead.
    if (lives_ <= 0)
    {
        waveRunning_ = false;
    }

    enemies_.erase(
        std::remove_if(
            enemies_.begin(),
            enemies_.end(),
            [](const std::unique_ptr<Enemy>& enemy) { return !enemy->isActive(); }
        ),
        enemies_.end()
    );

    projectiles_.erase(
        std::remove_if(
            projectiles_.begin(),
            projectiles_.end(),
            [](const std::unique_ptr<Projectile>& projectile) { return !projectile->isActive(); }
        ),
        projectiles_.end()
    );
}

void Game::render()
{
    // Skip rendering if initialization failed.
    if (renderer_ == nullptr)
    {
        return;
    }

    renderer_->clear();

    // Draw world.
    map_.draw(*renderer_, kMapOffsetX, kMapOffsetY, kCellSize);
    for (const auto& tower : towers_)
    {
        tower->draw(*renderer_);
    }
    for (const auto& enemy : enemies_)
    {
        enemy->draw(*renderer_);
    }
    for (const auto& projectile : projectiles_)
    {
        projectile->draw(*renderer_);
    }

    // Draw overlays.
    drawCursor();
    drawHud();

    renderer_->present();
}

void Game::drawHud()
{
    // Draw right panel background.
    const int panelX = kMapOffsetX + map_.cols() * kCellSize + kHudPadding;
    renderer_->fillRect(panelX - 12, 20, 240, 500, 35, 35, 35, 255);
    renderer_->drawRect(panelX - 12, 20, 240, 500, 80, 80, 80, 255);

    // Tower selection buttons.
    for (int i = 0; i < 5; ++i)
    {
        const TowerType type = static_cast<TowerType>(i);
        const SDL_Color c = towerColor(type);
        const int x = panelX;
        const int y = 40 + i * 56;

        renderer_->fillRect(x, y, kTowerButtonSize, kTowerButtonSize, c.r, c.g, c.b, 255);
        renderer_->drawRect(x, y, kTowerButtonSize, kTowerButtonSize, 15, 15, 15, 255);

        if (selectedTowerType_ == type)
        {
            renderer_->drawRect(x - 2, y - 2, kTowerButtonSize + 4, kTowerButtonSize + 4, 255, 255, 255, 255);
        }
    }

    // Start/Pause buttons.
    renderer_->fillRect(panelX, 350, 120, 34, 60, 150, 80, 255);
    renderer_->drawRect(panelX, 350, 120, 34, 20, 20, 20, 255);

    renderer_->fillRect(panelX, 392, 120, 34, 180, 130, 60, 255);
    renderer_->drawRect(panelX, 392, 120, 34, 20, 20, 20, 255);

    // Money/Lives visual bars.
    const int moneyWidth = std::min(180, std::max(0, money_));
    renderer_->fillRect(panelX, 450, moneyWidth, 12, 240, 210, 90, 255);
    renderer_->drawRect(panelX, 450, 180, 12, 20, 20, 20, 255);

    const int livesWidth = std::min(180, std::max(0, lives_ * 9));
    renderer_->fillRect(panelX, 470, livesWidth, 12, 220, 80, 80, 255);
    renderer_->drawRect(panelX, 470, 180, 12, 20, 20, 20, 255);

    const int waveFill = std::min(180, waveNumber_ * 10);
    renderer_->fillRect(panelX, 490, waveFill, 12, 120, 170, 240, 255);
    renderer_->drawRect(panelX, 490, 180, 12, 20, 20, 20, 255);
}

void Game::drawCursor()
{
    // Highlight selected map cell.
    const int x = kMapOffsetX + cursorCell_.x * kCellSize;
    const int y = kMapOffsetY + cursorCell_.y * kCellSize;
    const SDL_Color c = towerColor(selectedTowerType_);

    renderer_->drawRect(x, y, kCellSize, kCellSize, c.r, c.g, c.b, 255);
    renderer_->drawRect(x + 1, y + 1, kCellSize - 2, kCellSize - 2, 255, 255, 255, 255);
}

void Game::buildWorldPath()
{
    // Convert map cell coordinates to pixel centers.
    worldPath_.clear();
    for (const SDL_Point& cell : map_.path())
    {
        worldPath_.push_back(cellToWorld(cell));
    }
}

void Game::startNextWave()
{
    // Avoid overlapping waves.
    if (waveRunning_ || lives_ <= 0)
    {
        return;
    }

    ++waveNumber_;
    waveEnemies_ = buildWave(waveNumber_);
    waveSpawnIndex_ = 0;
    waveSpawnTimer_ = 0.0f;
    waveRunning_ = true;
}

std::vector<EnemyType> Game::buildWave(int waveNumber) const
{
    // Creates a simple progression for enemy variety.
    const int count = 6 + waveNumber * 2;
    std::vector<EnemyType> wave;
    wave.reserve(static_cast<std::size_t>(count));

    for (int i = 0; i < count; ++i)
    {
        EnemyType type = EnemyType::Regular;

        if (waveNumber >= 5 && i % 7 == 0)
        {
            type = EnemyType::Armored;
        }
        else if (waveNumber >= 4 && i % 6 == 0)
        {
            type = EnemyType::Jet;
        }
        else if (waveNumber >= 3 && i % 5 == 0)
        {
            type = EnemyType::Helicopter;
        }
        else if (waveNumber >= 2 && i % 4 == 0)
        {
            type = EnemyType::Strong;
        }
        else if (i % 3 == 0)
        {
            type = EnemyType::Fast;
        }

        wave.push_back(type);
    }

    return wave;
}

bool Game::tryPlaceTower(const SDL_Point& cell, TowerType type)
{
    // Check placement rules.
    if (!map_.isTowerSlot(cell.x, cell.y))
    {
        return false;
    }

    if (findTowerAtCell(cell).has_value())
    {
        return false;
    }

    std::unique_ptr<Tower> tower = std::make_unique<Tower>(type, cell, cellToWorld(cell));
    if (money_ < tower->cost())
    {
        return false;
    }

    money_ -= tower->cost();
    towers_.push_back(std::move(tower));
    return true;
}

bool Game::tryUpgradeTower(const SDL_Point& cell)
{
    // Upgrade tower at cell if present.
    const std::optional<std::size_t> index = findTowerAtCell(cell);
    if (!index.has_value())
    {
        return false;
    }

    return towers_[*index]->upgrade(money_);
}

std::optional<std::size_t> Game::findTowerAtCell(const SDL_Point& cell) const
{
    // Search tower occupancy by map cell.
    for (std::size_t i = 0; i < towers_.size(); ++i)
    {
        if (towers_[i]->cell().x == cell.x && towers_[i]->cell().y == cell.y)
        {
            return i;
        }
    }

    return std::nullopt;
}

SDL_FPoint Game::cellToWorld(const SDL_Point& cell) const
{
    // Cell center in world pixels.
    const float x = static_cast<float>(kMapOffsetX + cell.x * kCellSize + kCellSize / 2);
    const float y = static_cast<float>(kMapOffsetY + cell.y * kCellSize + kCellSize / 2);
    return SDL_FPoint{x, y};
}

SDL_Point Game::screenToCell(int x, int y) const
{
    // Screen pixel to map cell.
    const int col = (x - kMapOffsetX) / kCellSize;
    const int row = (y - kMapOffsetY) / kCellSize;
    return SDL_Point{col, row};
}

bool Game::isInsideMapPixels(int x, int y) const
{
    // Bounds check in rendered map rectangle.
    const int left = kMapOffsetX;
    const int top = kMapOffsetY;
    const int right = kMapOffsetX + map_.cols() * kCellSize;
    const int bottom = kMapOffsetY + map_.rows() * kCellSize;
    return x >= left && x < right && y >= top && y < bottom;
}

std::optional<TowerType> Game::towerTypeFromKey(SDL_Keycode key) const
{
    // Keyboard binding for tower selection.
    if (key == SDLK_1)
    {
        return TowerType::Basic;
    }
    if (key == SDLK_2)
    {
        return TowerType::Sniper;
    }
    if (key == SDLK_3)
    {
        return TowerType::Cannon;
    }
    if (key == SDLK_4)
    {
        return TowerType::Freezing;
    }
    if (key == SDLK_5)
    {
        return TowerType::Antiair;
    }

    return std::nullopt;
}

SDL_Color Game::towerColor(TowerType type) const
{
    // Shared colors for cursor/HUD icons.
    if (type == TowerType::Basic)
    {
        return SDL_Color{70, 200, 80, 255};
    }
    if (type == TowerType::Sniper)
    {
        return SDL_Color{80, 190, 220, 255};
    }
    if (type == TowerType::Cannon)
    {
        return SDL_Color{220, 120, 70, 255};
    }
    if (type == TowerType::Freezing)
    {
        return SDL_Color{80, 160, 255, 255};
    }

    return SDL_Color{190, 190, 80, 255};
}
