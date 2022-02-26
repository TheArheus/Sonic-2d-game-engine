#if !defined(_WIN32_GAME_ENGINE_H_)

struct win32_draw_buffer
{
    BITMAPINFO BitmapInfo;
    int Width;
    int Height;
    int Pitch;
    void* Memory;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

struct win32_game_code
{
    // NOTE: There could be more different main game code functions
    HMODULE GameCodeLibrary;
    FILETIME LastCodeWrite;
    game_main_render_and_update_loop* GameMainRenderAndUpdateLoop;
    b32 IsValid;
};

#define _WIN32_GAME_ENGINE_H_
#endif
