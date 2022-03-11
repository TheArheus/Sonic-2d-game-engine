

internal void
InitializeTileMap(memory_block* Block, tile_map* TileMap, s32 SizeX, s32 SizeY)
{
    TileMap->TileMapSize = SizeX;
    TileMap->TileMapSize = SizeY;
    TileMap->Tiles = PushArray(Block, u32, SizeX * SizeY);
}

internal u32
GetTileValue(tile_map* TileMap, u32 TileX, u32 TileY)
{
    u32 Result = TileMap->Tiles[TileY * TileMap->TileMapSize + TileX];
    return Result;
}

internal void
SetTileValue(tile_map* TileMap, u32 TileX, u32 TileY, u8 Value)
{
    TileMap->Tiles[TileY * TileMap->TileMapSize + TileX] = Value;
}

#define CHUNK_UNINITIALIZED INT_MAX

internal void
InitializeChunkSystem(chunk_system* Chunks, memory_block* Block)
{
    for(u32 ChunkIndex = 0;
        ChunkIndex < ArraySize(Chunks->TileChunks);
        ++ChunkIndex)
    {
        tile_chunk* Chunk = Chunks->TileChunks + ChunkIndex;
        Chunk->TileMap = PushStruct(Block, tile_map);
        Chunk->TileMap->TileMapSize = CHUNK_SIZE;
        Chunk->TileMap->Tiles = PushArray(Block, u32, CHUNK_SIZE * CHUNK_SIZE);
        Chunk->Pos.ChunkX = CHUNK_UNINITIALIZED;
    }
}

internal void
MapIntoChunkSpace(chunk_system* Chunks, r32 RelX, r32 RelY, u32* ChunkX, u32* ChunkY)
{
    Assert(RelX >= 0);
    Assert(RelY >= 0);
    Assert(RelX < ArraySize(Chunks->TileChunks)*CHUNK_SIZE);
    Assert(RelY < ArraySize(Chunks->TileChunks)*CHUNK_SIZE);

    *ChunkX = RelX / ArraySize(Chunks->TileChunks);
    *ChunkY = RelY / ArraySize(Chunks->TileChunks);
}

// NOTE: Get chunks for their x + y position hash value
// NOTE: Use it to allocate new chunks if they are not allocated (only if using hashing here)
internal tile_chunk*
GetChunk(chunk_system* Chunks, u32 ChunkX, u32 ChunkY)
{
    tile_chunk* Chunk = Chunks->TileChunks + (ChunkY*CHUNK_SIZE + ChunkX);

    Chunk->Pos.ChunkX = ChunkX;
    Chunk->Pos.ChunkY = ChunkY;

    return Chunk;
}

internal void
MoveChunk(chunk_system* Chunks, u32 SrcChunkX, u32 SrcChunkY, u32 DstChunkX, u32 DstChunkY)
{
    tile_chunk* SrcChunk = GetChunk(Chunks, SrcChunkX, SrcChunkY);
    tile_chunk* DstChunk = GetChunk(Chunks, DstChunkX, DstChunkY);
    DstChunk = SrcChunk;
}

