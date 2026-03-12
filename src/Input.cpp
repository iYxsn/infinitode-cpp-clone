#include "Input.h"

Input::Input() : quit_(false)
{
    // No quit request at startup.
}

void Input::update(const SDL_Event& e)
{
    // Window close button.
    if (e.type == SDL_QUIT)
    {
        quit_ = true;
    }

    // Escape key for fast exit in development.
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
    {
        quit_ = true;
    }
}

bool Input::quitRequested() const
{
    // Read-only access to quit flag.
    return quit_;
}
