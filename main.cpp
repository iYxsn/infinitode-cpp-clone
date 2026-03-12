#include "Game.h"
#include <iostream>

int main(int, char**)
{
    // Build & run the game
    Game game;
    if (!game.init())
    {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    game.run();
    return 0;
}
