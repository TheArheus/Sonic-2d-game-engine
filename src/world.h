
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

    // NOTE: Offset for a chunk
    // will be used for a drawing a chunk tiles
    s8 Offset;
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
    //s32 TileChunkCount;

    // TODO: Implement hashing with external chaining
    // NOTE: Maximum level width and height will be ((2 ^ N) / 2) chunks
    // Chunk is 4 by 4
    tile_chunk TileChunks[1 << 12];
};
