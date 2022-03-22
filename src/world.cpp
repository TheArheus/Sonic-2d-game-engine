
#define CHUNK_UNINITIALIZED INT_MAX

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
    Assert(TileX < TileMap->TileMapSize);
    Assert(TileY < TileMap->TileMapSize);

    u32 Result = TileMap->Tiles[TileY * TileMap->TileMapSize + TileX];
    return Result;
}

internal void
SetTileValue(tile_map* TileMap, u32 TileX, u32 TileY, u8 Value)
{
    Assert(TileX < TileMap->TileMapSize);
    Assert(TileY < TileMap->TileMapSize);

    TileMap->Tiles[TileY * TileMap->TileMapSize + TileX] = Value;
}

internal void
InitializeChunkSystem(chunk_system* Chunks, memory_block* Block)
{
    Chunks->MaxChunkX = SquareRoot(ArraySize(Chunks->TileChunks));
    Chunks->MaxChunkY = Chunks->MaxChunkX;

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

internal v2
ChunkPositionToWorldPosition(world_position Pos, r32 PixelsInMeter)
{
    v2 Result = {};
    Result.x = Pos.ChunkX*CHUNK_SIZE*PixelsInMeter + Pos.OffsetX;
    Result.y = Pos.ChunkY*CHUNK_SIZE*PixelsInMeter + Pos.OffsetY;
    return Result;
}

internal void
RecanonicalizeCoord(r32 RelCoord, r32 PixelsInMeter, s32* ChunkPos, r32* Offset)
{
    *ChunkPos = RelCoord / (CHUNK_SIZE * PixelsInMeter);
    *Offset = RelCoord - *ChunkPos * CHUNK_SIZE * PixelsInMeter;
}

internal world_position
MapIntoChunkSpace(chunk_system* Chunks, r32 PixelsInMeter, r32 RelX, r32 RelY)
{
    Assert(RelX >= 0);
    Assert(RelY >= 0);
    Assert(RelX < Chunks->MaxChunkX*CHUNK_SIZE*PixelsInMeter);
    Assert(RelY < Chunks->MaxChunkY*CHUNK_SIZE*PixelsInMeter);

    world_position Result = {};

    RecanonicalizeCoord(RelX, PixelsInMeter, &Result.ChunkX, &Result.OffsetX);
    RecanonicalizeCoord(RelY, PixelsInMeter, &Result.ChunkY, &Result.OffsetY);

    return Result;
}

internal world_position
Substract(world_position* A, world_position* B, r32 PixelsInMeter)
{
    world_position Result = {};
    
    Result.ChunkX = A->ChunkX - B->ChunkX;
    Result.ChunkY = A->ChunkY - B->ChunkY;
    Result.OffsetX = (A->OffsetX - B->OffsetX);
    Result.OffsetY = (A->OffsetY - B->OffsetY);
    if(Result.OffsetX < 0)
    {
        Result.ChunkX -=  1;
        Result.OffsetX = CHUNK_SIZE*PixelsInMeter + Result.OffsetX;
    }
    if(Result.OffsetY < 0)
    {
        Result.ChunkY -=  1;
        Result.OffsetY = CHUNK_SIZE*PixelsInMeter + Result.OffsetY;
    }

    return Result;
}

// NOTE: Get chunks for their x + y position hash value
// NOTE: Use it to allocate new chunks if they are not allocated (only if using hashing here)
internal tile_chunk*
GetChunk(chunk_system* Chunks, u32 ChunkX, u32 ChunkY)
{
    Assert(ChunkX < Chunks->MaxChunkX);
    Assert(ChunkY < Chunks->MaxChunkY);

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

