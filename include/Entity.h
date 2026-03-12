#ifndef ENTITY_H
#define ENTITY_H

#include <SDL.h>
#include <cstddef>
#include <vector>

class Renderer;

enum class EnemyType
{
    Regular,
    Fast,
    Strong,
    Armored,
    Helicopter,
    Jet
};

enum class TowerType
{
    Basic,
    Sniper,
    Cannon,
    Freezing,
    Antiair
};

struct EnemyStats
{
    float hp;
    float speed;
    float resistance;
    bool flying;
    SDL_Color color;
};

struct TowerStats
{
    float range;
    float damage;
    float attackCooldown;
    float rotationSpeed;
    float projectileSpeed;
    int cost;
    int upgradeCost;
    SDL_Color color;
    bool antiAir;
};

class Entity
{
public:
    // Builds a generic entity at position (x, y)
    Entity(float x, float y);
    // Virtual destructor for polymorphic use
    virtual ~Entity();

    // Updates entity logic
    virtual void update(float dt) = 0;
    // Draws entity on screen
    virtual void draw(Renderer& renderer) const = 0;

    // Returns true while entity is active
    bool isActive() const;
    // Returns current world position
    const SDL_FPoint& position() const;

protected:
    SDL_FPoint position_;
    bool active_;
};

class Enemy : public Entity
{
public:
    // Builds an enemy with its type and world path
    Enemy(EnemyType type, const std::vector<SDL_FPoint>& path);

    // Moves enemy toward next path point
    void update(float dt) override;
    // Draws enemy as a small colored square
    void draw(Renderer& renderer) const override;

    // Applies damage after resistance
    void applyDamage(float damage);
    // Returns true for air units
    bool isFlying() const;
    // Returns true if enemy reached map end
    bool reachedEnd() const;

private:
    // Returns default stats for one enemy type
    static EnemyStats statsForType(EnemyType type);

    EnemyType type_;
    EnemyStats stats_;
    float hp_;
    std::vector<SDL_FPoint> path_;
    std::size_t nextPointIndex_;
    bool reachedEnd_;
    float angleRad_;
};

class Tower : public Entity
{
public:
    // Builds one tower of given type on a map cell
    Tower(TowerType type, SDL_Point cell, SDL_FPoint worldPosition);

    // Updates internal cooldown timer
    void update(float dt) override;
    // Draws tower body and aiming direction
    void draw(Renderer& renderer) const override;

    // Rotates tower gradually toward target position
    void aimToward(const SDL_FPoint& target, float dt);
    // Returns true when attack cooldown is ready
    bool canShoot() const;
    // Resets cooldown after shot
    void onShot();
    // Upgrades tower stats if possible.
    bool upgrade(int& money);

    // Read only accessors used by gameplay.
    TowerType type() const;
    const SDL_Point& cell() const;
    float range() const;
    float damage() const;
    float projectileSpeed() const;
    float angleRad() const;
    bool antiAir() const;
    int cost() const;

private:
    // Returns default stats for one tower type
    static TowerStats statsForType(TowerType type);

    TowerType type_;
    TowerStats stats_;
    SDL_Point cell_;
    int level_;
    float cooldown_;
    float angleRad_;
};

class Projectile : public Entity
{
public:
    // Builds one projectile from origin using angle and speed
    Projectile(SDL_FPoint origin, float angleRad, float speed, float damage, bool antiAir);

    // Moves projectile and expires after max lifetime
    void update(float dt) override;
    // Draws projectile as a small square
    void draw(Renderer& renderer) const override;

    // Returns projectile damage
    float damage() const;
    // Returns true when this projectile can hit the enemy
    bool canHitEnemy(const Enemy& enemy) const;
    // Returns true if projectile overlaps enemy
    bool collidesWith(const Enemy& enemy) const;
    // Marks projectile as consumed
    void consume();

private:
    SDL_FPoint velocity_;
    float damage_;
    bool antiAir_;
    float lifeSec_;
};

#endif
