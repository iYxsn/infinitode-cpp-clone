#include "Entity.h"
#include "Renderer.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr float kPi = 3.1415926535f;

// Normalizes angle to [-pi, pi] for stable rotation.
float normalizeAngle(float angle)
{
    while (angle > kPi)
    {
        angle -= 2.0f * kPi;
    }
    while (angle < -kPi)
    {
        angle += 2.0f * kPi;
    }
    return angle;
}
} // namespace

Entity::Entity(float x, float y) : position_{x, y}, active_(true)
{
    // Base entity starts active.
}

Entity::~Entity()
{
    // Virtual for safe polymorphic destruction.
}

bool Entity::isActive() const
{
    // Used to remove finished entities.
    return active_;
}

const SDL_FPoint& Entity::position() const
{
    // Shared access to world coordinates.
    return position_;
}

Enemy::Enemy(EnemyType type, const std::vector<SDL_FPoint>& path)
    : Entity(0.0f, 0.0f),
      type_(type),
      stats_(statsForType(type)),
      hp_(stats_.hp),
      path_(path),
      nextPointIndex_(1),
      reachedEnd_(false),
      angleRad_(0.0f)
{
    // Spawn at first path point when available.
    if (path_.empty())
    {
        active_ = false;
        return;
    }

    position_ = path_.front();
}

void Enemy::update(float dt)
{
    // Ignore update for inactive enemies.
    if (!active_)
    {
        return;
    }

    if (nextPointIndex_ >= path_.size())
    {
        reachedEnd_ = true;
        active_ = false;
        return;
    }

    const SDL_FPoint target = path_[nextPointIndex_];
    const float dx = target.x - position_.x;
    const float dy = target.y - position_.y;
    const float distance = std::sqrt(dx * dx + dy * dy);

    if (distance < 0.0001f)
    {
        position_ = target;
        ++nextPointIndex_;
        return;
    }

    angleRad_ = std::atan2(dy, dx);

    const float step = stats_.speed * dt;
    if (step >= distance)
    {
        position_ = target;
        ++nextPointIndex_;
        return;
    }

    position_.x += (dx / distance) * step;
    position_.y += (dy / distance) * step;
}

void Enemy::draw(Renderer& renderer) const
{
    // Draw enemy body.
    const int size = 12;
    const int x = static_cast<int>(position_.x) - size / 2;
    const int y = static_cast<int>(position_.y) - size / 2;
    renderer.fillRect(x, y, size, size, stats_.color.r, stats_.color.g, stats_.color.b, stats_.color.a);
    renderer.drawRect(x, y, size, size, 15, 15, 15, 255);

    // Draw small orientation line.
    const int line = 10;
    const int x2 = static_cast<int>(position_.x + std::cos(angleRad_) * static_cast<float>(line));
    const int y2 = static_cast<int>(position_.y + std::sin(angleRad_) * static_cast<float>(line));
    renderer.drawLine(static_cast<int>(position_.x), static_cast<int>(position_.y), x2, y2, 240, 240, 240, 255);
}

void Enemy::applyDamage(float damage)
{
    // Apply resistance then remove enemy on zero HP.
    if (!active_)
    {
        return;
    }

    const float effective = damage * (1.0f - stats_.resistance);
    hp_ -= std::max(0.0f, effective);

    if (hp_ <= 0.0f)
    {
        active_ = false;
    }
}

bool Enemy::isFlying() const
{
    // Required by antiair filtering.
    return stats_.flying;
}

bool Enemy::reachedEnd() const
{
    // Used to subtract player lives.
    return reachedEnd_;
}

EnemyStats Enemy::statsForType(EnemyType type)
{
    // Simple values for visible differences between types.
    if (type == EnemyType::Regular)
    {
        return EnemyStats{100.0f, 60.0f, 0.0f, false, SDL_Color{60, 200, 90, 255}};
    }
    if (type == EnemyType::Fast)
    {
        return EnemyStats{70.0f, 95.0f, 0.0f, false, SDL_Color{240, 200, 60, 255}};
    }
    if (type == EnemyType::Strong)
    {
        return EnemyStats{180.0f, 45.0f, 0.10f, false, SDL_Color{240, 120, 60, 255}};
    }
    if (type == EnemyType::Armored)
    {
        return EnemyStats{220.0f, 40.0f, 0.35f, false, SDL_Color{90, 200, 200, 255}};
    }
    if (type == EnemyType::Helicopter)
    {
        return EnemyStats{130.0f, 80.0f, 0.05f, true, SDL_Color{70, 120, 240, 255}};
    }

    return EnemyStats{110.0f, 100.0f, 0.05f, true, SDL_Color{90, 90, 230, 255}};
}

Tower::Tower(TowerType type, SDL_Point cell, SDL_FPoint worldPosition)
    : Entity(worldPosition.x, worldPosition.y),
      type_(type),
      stats_(statsForType(type)),
      cell_(cell),
      level_(1),
      cooldown_(0.0f),
      angleRad_(0.0f)
{
    // Tower starts ready to shoot.
}

void Tower::update(float dt)
{
    // Reduce attack cooldown over time.
    cooldown_ = std::max(0.0f, cooldown_ - dt);
}

void Tower::draw(Renderer& renderer) const
{
    // Draw tower body.
    const int size = 18;
    const int x = static_cast<int>(position_.x) - size / 2;
    const int y = static_cast<int>(position_.y) - size / 2;
    renderer.fillRect(x, y, size, size, stats_.color.r, stats_.color.g, stats_.color.b, stats_.color.a);
    renderer.drawRect(x, y, size, size, 20, 20, 20, 255);

    // Draw orientation line.
    const int line = 12;
    const int x2 = static_cast<int>(position_.x + std::cos(angleRad_) * static_cast<float>(line));
    const int y2 = static_cast<int>(position_.y + std::sin(angleRad_) * static_cast<float>(line));
    renderer.drawLine(static_cast<int>(position_.x), static_cast<int>(position_.y), x2, y2, 255, 255, 255, 255);
}

void Tower::aimToward(const SDL_FPoint& target, float dt)
{
    // Rotate progressively toward target according to rotation speed.
    const float wanted = std::atan2(target.y - position_.y, target.x - position_.x);
    const float diff = normalizeAngle(wanted - angleRad_);

    const float maxStep = stats_.rotationSpeed * dt;
    if (std::fabs(diff) <= maxStep)
    {
        angleRad_ = wanted;
        return;
    }

    angleRad_ += (diff > 0.0f ? maxStep : -maxStep);
    angleRad_ = normalizeAngle(angleRad_);
}

bool Tower::canShoot() const
{
    // Tower can shoot when cooldown is finished.
    return cooldown_ <= 0.0f;
}

void Tower::onShot()
{
    // Restart attack cooldown.
    cooldown_ = stats_.attackCooldown;
}

bool Tower::upgrade(int& money)
{
    // Upgrade if player can pay current upgrade cost.
    if (money < stats_.upgradeCost)
    {
        return false;
    }

    money -= stats_.upgradeCost;
    ++level_;

    stats_.range *= 1.08f;
    stats_.damage *= 1.22f;
    stats_.projectileSpeed *= 1.06f;
    stats_.attackCooldown = std::max(0.18f, stats_.attackCooldown * 0.93f);
    stats_.upgradeCost = static_cast<int>(static_cast<float>(stats_.upgradeCost) * 1.45f);

    return true;
}

TowerType Tower::type() const
{
    // Exposes type for HUD.
    return type_;
}

const SDL_Point& Tower::cell() const
{
    // Exposes tile position for placement checks.
    return cell_;
}

float Tower::range() const
{
    // Exposes range for targeting.
    return stats_.range;
}

float Tower::damage() const
{
    // Exposes damage for projectile creation.
    return stats_.damage;
}

float Tower::projectileSpeed() const
{
    // Exposes projectile speed for projectile creation.
    return stats_.projectileSpeed;
}

float Tower::angleRad() const
{
    // Exposes current rotation for projectile direction.
    return angleRad_;
}

bool Tower::antiAir() const
{
    // True only for antiair tower.
    return stats_.antiAir;
}

int Tower::cost() const
{
    // Exposes build cost for placement checks.
    return stats_.cost;
}

TowerStats Tower::statsForType(TowerType type)
{
    // Default stats required by assignment.
    if (type == TowerType::Basic)
    {
        return TowerStats{120.0f, 22.0f, 0.60f, 3.2f, 220.0f, 50, 40, SDL_Color{70, 200, 80, 255}, false};
    }
    if (type == TowerType::Sniper)
    {
        return TowerStats{220.0f, 60.0f, 1.40f, 2.1f, 320.0f, 85, 65, SDL_Color{80, 190, 220, 255}, false};
    }
    if (type == TowerType::Cannon)
    {
        return TowerStats{145.0f, 46.0f, 1.10f, 2.0f, 180.0f, 95, 75, SDL_Color{220, 120, 70, 255}, false};
    }
    if (type == TowerType::Freezing)
    {
        return TowerStats{135.0f, 16.0f, 0.45f, 3.0f, 210.0f, 75, 55, SDL_Color{80, 160, 255, 255}, false};
    }

    return TowerStats{175.0f, 26.0f, 0.55f, 3.4f, 265.0f, 80, 60, SDL_Color{190, 190, 80, 255}, true};
}

Projectile::Projectile(SDL_FPoint origin, float angleRad, float speed, float damage, bool antiAir)
    : Entity(origin.x, origin.y),
      velocity_{std::cos(angleRad) * speed, std::sin(angleRad) * speed},
      damage_(damage),
      antiAir_(antiAir),
      lifeSec_(3.0f)
{
    // Projectile starts at tower position.
}

void Projectile::update(float dt)
{
    // Move projectile and despawn after lifetime.
    if (!active_)
    {
        return;
    }

    position_.x += velocity_.x * dt;
    position_.y += velocity_.y * dt;

    lifeSec_ -= dt;
    if (lifeSec_ <= 0.0f)
    {
        active_ = false;
    }
}

void Projectile::draw(Renderer& renderer) const
{
    // Draw projectile body.
    const int size = 6;
    const int x = static_cast<int>(position_.x) - size / 2;
    const int y = static_cast<int>(position_.y) - size / 2;

    if (antiAir_)
    {
        renderer.fillRect(x, y, size, size, 250, 250, 120, 255);
    }
    else
    {
        renderer.fillRect(x, y, size, size, 250, 250, 250, 255);
    }
    renderer.drawRect(x, y, size, size, 25, 25, 25, 255);
}

float Projectile::damage() const
{
    // Exposes projectile damage.
    return damage_;
}

bool Projectile::canHitEnemy(const Enemy& enemy) const
{
    // Antiair hits flying units, others hit ground units.
    if (antiAir_)
    {
        return enemy.isFlying();
    }

    return !enemy.isFlying();
}

bool Projectile::collidesWith(const Enemy& enemy) const
{
    // Basic radius-based collision check.
    const float dx = enemy.position().x - position_.x;
    const float dy = enemy.position().y - position_.y;
    const float distSq = dx * dx + dy * dy;
    const float radius = 10.0f;
    return distSq <= radius * radius;
}

void Projectile::consume()
{
    // Despawns projectile after one hit.
    active_ = false;
}
