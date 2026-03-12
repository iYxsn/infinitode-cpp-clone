#ifndef APP_H
#define APP_H

#include <SDL.h>
#include <memory>
#include <string>

class App
{
public:
    // Builds an empty app wrapper
    App();
    // Releases SDL global state on shutdown
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Initializes SDL video, the window and the renderer
    bool init(const std::string& title, int width, int height);
    // Retrieves one SDL event if available
    bool pollEvent(SDL_Event& e) const;
    // Returns the internal SDL renderer handle (non-owning)
    SDL_Renderer* renderer() const;
    // Updates the window title (used for debug HUD text)
    void setWindowTitle(const std::string& title) const;

private:
    // Custom deleter for SDL_Window used by unique_ptr
    struct WindowDeleter
    {
        void operator()(SDL_Window* w) const;
    };

    // Custom deleter for SDL_Renderer used by unique_ptr
    struct RendererDeleter
    {
        void operator()(SDL_Renderer* r) const;
    };

    std::unique_ptr<SDL_Window, WindowDeleter> window_;
    std::unique_ptr<SDL_Renderer, RendererDeleter> renderer_;
};

#endif
