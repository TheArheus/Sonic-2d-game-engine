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

#define PushStruct(Block, Type) (Type*)PushSize_(Block, sizeof(Type)); 
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

#define CHUNK_SIZE 4

struct tile_map
{
    s32 TileMapWidth;
    s32 TileMapHeight;
    u8* Tiles;
};

// NOTE: Store the ammount of entities for each chunk
// and maybe store entities itself
struct tile_chunk
{
    s32 PositionX;
    s32 PositionY;

    tile_map* TileMap;
    tile_chunk* NextChunk;
};

struct chunk_system
{
    s32 TileChunkCount;
    // TODO: Implement hashing with external chaining
    tile_chunk TileChunks[1024];
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

    // NOTE: I will use this for a world/level system
    tile_map TestTileMap;
    chunk_system TestChunkSystem;

    player Player;

    r32 DeltaTime;
    r32 TotalTime;
};
