#include <framework/App.h>

namespace frm
{
    App::App() :
        m_window(nullptr),
        m_currentSwapbuffer(0)
    {
    }
    
    App::~App()
    {
        if (m_window != nullptr) {
            SDL_DestroyWindow(m_window);
        }

        SDL_Quit();
    }
    
    void App::init(int w, int h)
    {
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
            throw std::runtime_error("Cannot init SDL");
        }

        m_window = SDL_CreateWindow("vulkan-learn",
                                    SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED,
                                    w,
                                    h,
                                    SDL_WINDOW_VULKAN);

        if (m_window == nullptr) {
            throw std::runtime_error("Cannot create window");
        }

        m_vkCtx.initDevice(m_window);
        onInit(m_vkCtx);
    }

    void App::dispatch()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        bool run = true;

        while (run) {
            SDL_Event event;
            auto newTime = std::chrono::high_resolution_clock::now();
            double dt = std::chrono::duration<double>(newTime - currentTime).count();

            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_QUIT:
                        run = false;
                }
            }

            onUpdate(m_vkCtx, dt);

            m_vkCtx.prepareNextSwapbuffer(m_currentSwapbuffer);
            onRender(m_vkCtx, dt);
            m_vkCtx.present(m_currentSwapbuffer);

            currentTime = newTime;
        }

        onDestroy();
    }

    void App::onInit(VulkanContext& context)
    {
    }

    void App::onDestroy()
    {
    }

    void App::onGuiRender()
    {
    }

    void App::getClientSizeRect(VkRect2D& rect)
    {
        int w, h;

        assert(m_window != nullptr);

        SDL_GetWindowSize(m_window, &w, &h);

        rect.offset.x = 0;
        rect.offset.y = 0;
        rect.extent.width = static_cast<uint32_t>(w);
        rect.extent.height = static_cast<uint32_t>(h);
    }
}
