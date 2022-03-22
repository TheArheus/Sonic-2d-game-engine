
#define CHUNK_SIZE 4

struct tile_map
{
    s32 TileMapSize;

    u32* Tiles;
};

struct world_position
{
    s32 ChunkX;
    s32 ChunkY;

    // NOTE: Offset for a entity
    // will be used for a drawing a chunk tiles
    r32 OffsetX;
    r32 OffsetY;
};

// NOTE: Store the ammount of entities for each chunk
// and maybe store entities itself
struct tile_chunk
{
    world_position Pos;

    tile_map* TileMap;
    //tile_chunk* NextChunk;
};

struct chunk_system
{
    s32 MaxChunkX;
    s32 MaxChunkY;

    // TODO: Implement hashing with external chaining
    // Don't wan't this to be an internal chaining
    // NOTE: Maximum level width and height will be sqrt(2 ^ N) chunks
    tile_chunk TileChunks[1 << 12];
};
