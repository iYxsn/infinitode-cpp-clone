#include "App.h"

App::App() : window_(nullptr), renderer_(nullptr)
{
    // Start with empty handles.
}

App::~App()
{
    // SDL must be closed once at the end of the program.
    SDL_Quit();
}

void App::WindowDeleter::operator()(SDL_Window* w) const
{
    // Safely destroy window resource.
    if (w != nullptr)
    {
        SDL_DestroyWindow(w);
    }
}

void App::RendererDeleter::operator()(SDL_Renderer* r) const
{
    // Safely destroy renderer resource.
    if (r != nullptr)
    {
        SDL_DestroyRenderer(r);
    }
}

bool App::init(const std::string& title, int width, int height)
{
    // Initialize SDL video subsystem.
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        return false;
    }

    SDL_Window* w = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );

    if (w == nullptr)
    {
        return false;
    }

    // Store window immediately so it is always cleaned by RAII.
    window_.reset(w);

    // Prefer accelerated renderer, but allow software fallback.
    SDL_Renderer* r = SDL_CreateRenderer(
        window_.get(),
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (r == nullptr)
    {
        r = SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_SOFTWARE);
    }

    if (r == nullptr)
    {
        return false;
    }

    renderer_.reset(r);
    return true;
}

bool App::pollEvent(SDL_Event& e) const
{
    // SDL returns 1 when an event was fetched.
    return SDL_PollEvent(&e) == 1;
}

SDL_Renderer* App::renderer() const
{
    // Expose renderer for drawing wrapper construction.
    return renderer_.get();
}

void App::setWindowTitle(const std::string& title) const
{
    // Allows lightweight textual HUD without font dependency.
    if (window_ == nullptr)
    {
        return;
    }

    SDL_SetWindowTitle(window_.get(), title.c_str());
}
