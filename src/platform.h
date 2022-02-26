#include <stdint.h>

#define internal static
#define global_variable static
#define local_persist static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float  r32;
typedef double r64;

typedef s32 b32;
typedef s64 b64;

#define ArraySize(Array) (sizeof(Array) / sizeof((Array)[0]))

#if DEBUG
#define Assert(Expression) if(!(Expression)) {*(int*)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Megabytes(Value) (Value * 1024ll)
#define Kilobytes(Value) (Megabytes(Value) * 1024ll)
#define Gigabytes(Value) (Kilobytes(Value) * 1024ll)
#define Terabytes(Value) (Gigabytes(Value) * 1024ll)

struct game_render_buffer
{
    void* Memory;
    u32 Width;
    u32 Height;
    u32 Pitch;
};

struct game_button_state
{
    s32 HalfTransitionCount;
    b32 EndedDown;
};

struct game_controller_input
{
    b32 IsAnalog;
    b32 IsConnected;

    r32 StickAvarageX;
    r32 StickAvarageY;

    union
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state Up;
            game_button_state Left;
            game_button_state Right;
            game_button_state Down;

            game_button_state ActionUp;
            game_button_state ActionLeft;
            game_button_state ActionRight;
            game_button_state ActionDown;

            game_button_state ShoulderLeft;
            game_button_state ShoulderRight;

            game_button_state Start;
            game_button_state Back;
        };
    };
};

struct game_input
{
    game_button_state MouseButtons[5];
    s32 MouseX, MouseY;

    game_controller_input Controllers[5];
};

struct game_memory
{
    b32 IsInitialized;

    u64 PermamentStorageSize;
    void* PermamentStorage;

    u64 TransientStorageSize;
    void* TransientStorage;
};

#define GAME_MAIN_RENDER_AND_UPDATE_LOOP(name) void name(game_memory* GameMemory, game_input* Input, game_render_buffer* RenderBuffer)
typedef GAME_MAIN_RENDER_AND_UPDATE_LOOP(game_main_render_and_update_loop);

