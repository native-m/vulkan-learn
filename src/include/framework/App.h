#pragma once

#include <SDL2/SDL.h>
#include <framework/VulkanContext.h>

#undef main

namespace frm
{
    class App
    {
    public:
        App();
        ~App();

        void init(int w, int h);
        void dispatch();

        virtual void onInit(VulkanContext& context);
        virtual void onDestroy();
        virtual void onUpdate(VulkanContext& context, double dt) = 0;
        virtual void onRender(VulkanContext& context, double dt) = 0;
        virtual void onGuiRender();

        VulkanContext& getContext() { return m_vkCtx; };
        const uint32_t getCurrentSwapbuffer() const { return m_currentSwapbuffer; }
        void getClientSizeRect(VkRect2D& rect);

        template<class T>
        static int run(int w, int h);

    private:
        VulkanContext m_vkCtx;
        SDL_Window* m_window;
        uint32_t m_currentSwapbuffer;

        void prepareNextFrame();
        void swap();
    };

    template<class T>
    int App::run(int w, int h)
    {
        T theApp;

        theApp.init(w, h);
        theApp.dispatch();

        return 0;
    }
}
