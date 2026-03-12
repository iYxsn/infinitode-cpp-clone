#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>

class Input
{
public:
    // Builds input state for 1 game session
    Input();

    // Updates input state from one SDL event
    void update(const SDL_Event& e);
    // Returns true if the user asked to quit
    bool quitRequested() const;

private:
    bool quit_;
};

#endif
