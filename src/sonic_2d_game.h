#include "platform.h"
#include "hmath.h"

struct memory_block
{
    u8* Base;

    memory_index Size;
    memory_index Used;
};

internal void
AllocateMemoryBlock(memory_block* Block, u8* Base, memory_index Size)
{
    Block->Base = (u8*)Base;
    Block->Size = Size;
    Block->Used = 0;
}

#define PushStruct(Block, Type) (Type*)PushSize_(Bock, sizeof(Type)); 
#define PushArray(Block, Type, Ammount) (Type*)PushSize_(Block, sizeof(Type) * Ammount);
#define PushSize(Block, Size) (Type*)PushSize_(Block, Size);
internal void*
PushSize_(memory_block* Block, memory_index Size)
{
    Assert((Block->Used + Size) <= Block->Size);
    void* Result = 0;

    if(((Block->Used + Size) <= Block->Size))
    {
        Result = Block->Base;

        Block->Base += Size;
        Block->Used += Size;
    }

    return Result;
}

struct tile_map
{
    s32 TileWidth;
    s32 TileHeight;
    u8* Tiles;
};

struct player
{
    v2 P;
    v2 dP;
    v2 ddP;
};

struct game_state
{
    memory_block World;

    tile_map TestTileMap;

    player Player;

    r32 DeltaTime;
    r32 TotalTime;
};
