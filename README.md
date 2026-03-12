# Mini Tower Defense (C++ / SDL2)

Small learning project inspired by Infinitode-like gameplay

Goal: practice clean C++ project structure, OOP, game loop, map handling, entities, and real-time logic

## Features
- ASCII map loading (`assets/maps/map1.txt`)
- Start/end/path/tower slots (`S`, `E`, `#`, `T`)
- 6 enemy types: `Regular`, `Fast`, `Strong`, `Armored`, `Helicopter`, `Jet`
- 5 tower types: `Basic`, `Sniper`, `Cannon`, `Freezing`, `Antiair`
- Projectile system, collisions, damage, enemy resistances
- Wave spawning
- Tower upgrades (cost-based)
- Keyboard + mouse controls
- Minimal HUD panel

## Controls
- `Arrows`: move map cursor
- `1..5`: select tower type
- `B`: place selected tower on selected cell
- `U`: upgrade tower on selected cell
- `Enter`: start next wave
- `Space`: pause/resume
- `Esc`: quit
- `Left click` map: select + try place
- `Right click` map: upgrade tower
- `Left click` right panel: tower selection / start / pause

## Build
```bash
g++ -std=c++17 -Wall -Wextra -pedantic \
  main.cpp src/App.cpp src/Game.cpp src/Input.cpp src/Renderer.cpp src/Map.cpp src/Entity.cpp \
  -Iinclude $(sdl2-config --cflags --libs) -o infinitode_full
```

## Run
```bash
./infinitode_full
```

## Quick headless smoke test
```bash
SDL_VIDEODRIVER=dummy timeout 3s ./infinitode_full; echo EXIT:$?
```
Expected: `EXIT:124` (timeout stops a healthy game loop).

## Project Structure
- `include/`: headers
- `src/`: implementations
- `assets/maps/`: ASCII maps

## Learning Notes
This project is intentionally small and focused on fundamentals:
- C++ class design and separation (`.h/.cpp`)
- STL containers and algorithms
- simple inheritance/polymorphism (`Entity` hierarchy)
- SDL encapsulation via dedicated classes

## Not Implemented (optional/bonus)
- map editor
- tower modifiers system
