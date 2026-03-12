#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>

class Renderer
{
public:
    // Builds a small drawing wrapper around SDL_Renderer
    explicit Renderer(SDL_Renderer* r);

    // Clears the frame with a dark background
    void clear();
    // Draws a filled rectangle with color
    void fillRect(int x, int y, int w, int h, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255);
    // Draws a rectangle outline with color
    void drawRect(int x, int y, int w, int h, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255);
    // Draws one colored line
    void drawLine(int x1, int y1, int x2, int y2, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255);
    // Displays the rendered frame
    void present();

private:
    // Non-owning pointer managed by App
    SDL_Renderer* r_;
};

#endif
