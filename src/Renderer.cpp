#include "Renderer.h"

Renderer::Renderer(SDL_Renderer* r) : r_(r)
{
    // Store non-owning renderer pointer.
}

void Renderer::clear()
{
    if (r_ == nullptr)
    {
        return;
    }

    // Step 1 visual: plain dark background.
    SDL_SetRenderDrawColor(r_, 20, 20, 20, 255);
    SDL_RenderClear(r_);
}

void Renderer::fillRect(int x, int y, int w, int h, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
    if (r_ == nullptr)
    {
        return;
    }

    // Draw a filled cell.
    SDL_Rect rect = {x, y, w, h};
    SDL_SetRenderDrawColor(r_, red, green, blue, alpha);
    SDL_RenderFillRect(r_, &rect);
}

void Renderer::drawRect(int x, int y, int w, int h, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
    if (r_ == nullptr)
    {
        return;
    }

    // Draw cell border for grid readability.
    SDL_Rect rect = {x, y, w, h};
    SDL_SetRenderDrawColor(r_, red, green, blue, alpha);
    SDL_RenderDrawRect(r_, &rect);
}

void Renderer::drawLine(int x1, int y1, int x2, int y2, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
    if (r_ == nullptr)
    {
        return;
    }

    // Draw line for aim indicators and HUD.
    SDL_SetRenderDrawColor(r_, red, green, blue, alpha);
    SDL_RenderDrawLine(r_, x1, y1, x2, y2);
}

void Renderer::present()
{
    if (r_ == nullptr)
    {
        return;
    }

    // Swap back/front buffers.
    SDL_RenderPresent(r_);
}
