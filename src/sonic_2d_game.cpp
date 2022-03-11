#include "sonic_2d_game.h"
#include "world.cpp"

internal void
ClearColorBuffer(game_render_buffer* RenderBuffer, u32 Color)
{
    u8* Row = (u8*)RenderBuffer->Memory;
    for(u32 Y = 0;
        Y < RenderBuffer->Height;
        ++Y)
    {
        u32* Pixel = (u32*)Row;
        for(u32 X = 0;
            X < RenderBuffer->Width;
            ++X)
        {
            *Pixel++ = Color;
        }
        Row += RenderBuffer->Pitch;
    }
}

internal void
DrawRectangle(game_render_buffer* RenderBuffer, v2 Start, v2 End, u32 Color)
{
    s32 StartRenderX = Start.x;
    s32 StartRenderY = Start.y;
    s32 EndRenderX = End.x;
    s32 EndRenderY = End.y;

    if(StartRenderX < 0) StartRenderX = 0;
    if(StartRenderY < 0) StartRenderY = 0;
    if(EndRenderX < 0) EndRenderX = 0;
    if(EndRenderY < 0) EndRenderY = 0;

    if(StartRenderX > RenderBuffer->Width)  StartRenderX = RenderBuffer->Width;
    if(StartRenderY > RenderBuffer->Height) StartRenderY = RenderBuffer->Height;
    if(EndRenderX > RenderBuffer->Width)  EndRenderX = RenderBuffer->Width;
    if(EndRenderY > RenderBuffer->Height) EndRenderY = RenderBuffer->Height;

    u8* Row = ((u8*)RenderBuffer->Memory + 
               RenderBuffer->Pitch * StartRenderY + 
               sizeof(u32) * StartRenderX);
    for(u32 Y = StartRenderY;
        Y < EndRenderY;
        ++Y)
    {
        u32* Pixel = (u32*)Row;
        for(u32 X = StartRenderX;
            X < EndRenderX;
            ++X)
        {
            *Pixel++ = Color;
        }
        Row += RenderBuffer->Pitch;
    }
}

// TODO: Create a function to load a levels from a custom file
// For this thing I should create a custom file to do a thing
// and this will be my assets and stored in transient store after
// NOTE: After loading a level, it will be stored as a whole
// array of values with a corresponding values for each row
// I should think about how this values should be stored in the file
// Maybe this function will load all levels, not just one
// and then I would load then they will be in need
// NOTE: This function right now in testing mode
internal void
ChunkLevel(chunk_system* Chunks)
{
    // NOTE: Set up maximum level size
    const int LevelWidth  = 16;
    const int LevelHeight = 16;
    
    int TestLevel[LevelHeight][LevelWidth] = 
    {
        {2, 1, 2, 1, 2, 1, 2, 1, 1, 2, 1, 2, 1, 2, 1, 2},
        {1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 1},
        {1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1},
        {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
    };

    // NOTE: This is way too unificient
    // Think about better solution
    for(u32 ChunkY = 0;
        ChunkY < (LevelHeight / CHUNK_SIZE);
        ++ChunkY)
    {
        for(u32 ChunkX = 0;
            ChunkX < (LevelWidth / CHUNK_SIZE);
            ++ChunkX)
        {
            tile_chunk* NewLevelChunk = GetChunk(Chunks, ChunkX, ChunkY);

            for(u32 TileY = 0;
                TileY < CHUNK_SIZE;
                ++TileY)
            {
                for(u32 TileX = 0;
                    TileX < CHUNK_SIZE;
                    ++TileX)
                {
                    u32 GetTileY = TileY + (ChunkY * CHUNK_SIZE);
                    u32 GetTileX = TileX + (ChunkX * CHUNK_SIZE);
                    NewLevelChunk->TileMap->Tiles[TileY * CHUNK_SIZE + TileX] = TestLevel[GetTileY][GetTileX];
                }
            }
        }
    }
}

#if defined(__cplusplus)
extern "C" 
#endif
GAME_MAIN_RENDER_AND_UPDATE_LOOP(GameMainRenderAndUpdateLoop)
{
    game_state* GameState = (game_state*)GameMemory->PermamentStorage;

    r32 PixelsInMeter = 128.0f; // NOTE: I want to use this as Tile Size
    r32 MaxSpeed = PixelsInMeter * 12;

    AllocateMemoryBlock(&GameState->World, (u8*)GameMemory->PermamentStorage + sizeof(game_state), GameMemory->PermamentStorageSize - sizeof(game_state));

    if(!GameMemory->IsInitialized)
    {
        GameMemory->IsInitialized = true;

        GameState->DeltaTime = Input->DeltaTimeForFrame;

        GameState->Player.P = V2(RenderBuffer->Width / 2.0f, RenderBuffer->Height / 2.0f);

        InitializeChunkSystem(&GameState->TestChunkSystem, &GameState->World);

        // NOTE: Should I chunk every level after loading them into memory
        // or should chunk the levels only on demand?
        ChunkLevel(&GameState->TestChunkSystem);
    }

    ClearColorBuffer(RenderBuffer, 0xFF000000);

    for(u32 ControllerIndex = 0;
        ControllerIndex < ArraySize(Input->Controllers);
        ++ControllerIndex)
    {
        game_controller_input* Controller = Input->Controllers + ControllerIndex;
        if(Controller->IsConnected)
        {
            if(Controller->IsAnalog)
            {
                GameState->Player.ddP += V2(Controller->StickAvarageX, Controller->StickAvarageY);
            }
            else
            {
                if(Controller->Up.EndedDown)
                {
                    GameState->Player.ddP.y +=  0.45f;
                }
                if(Controller->Down.EndedDown)
                {
                    GameState->Player.ddP.y += -0.45f;
                }
                if(Controller->Left.EndedDown)
                {
                    GameState->Player.ddP.x += -0.45f;
                }
                if(Controller->Right.EndedDown)
                {
                    GameState->Player.ddP.x +=  0.45f;
                }
            }
        }
    }


    // NOTE: Testing Chunking System
    for(u32 ChunkIndex = 0;
        ChunkIndex < ArraySize(GameState->TestChunkSystem.TileChunks);
        ++ChunkIndex)
    {
        // NOTE: Maybe this is not the most efficient way to implement chunking system
        tile_chunk* Chunk = GameState->TestChunkSystem.TileChunks + ChunkIndex;

        if(Chunk->Pos.ChunkX != CHUNK_UNINITIALIZED)
        {
            for (u32 TileY = 0;
                TileY < Chunk->TileMap->TileMapSize;
                ++TileY)
            {
                for (u32 TileX = 0;
                    TileX < Chunk->TileMap->TileMapSize;
                    ++TileX)
                {
                    v2 Start = V2(TileX * PixelsInMeter + Chunk->Pos.ChunkX * CHUNK_SIZE * PixelsInMeter, TileY * PixelsInMeter + Chunk->Pos.ChunkY * CHUNK_SIZE * PixelsInMeter);
                    v2 End = Start + PixelsInMeter;

                    u32 Color = 0xFF000000;
                    u32 ID = GetTileValue(Chunk->TileMap, TileX, TileY);
                    if(ID == 1)
                    {
                        Color = 0xFFFF00FF;
                    }
                    else if(ID == 2)
                    {
                        Color = 0xFF00FF00;
                    }
                    DrawRectangle(RenderBuffer, Start, End, Color);
                }
            }
        }
    }

    GameState->Player.dP += GameState->Player.ddP * GameState->DeltaTime;
    GameState->Player.P  += GameState->Player.dP  * GameState->DeltaTime * PixelsInMeter;

    v2 RenderStart = GameState->Player.P - PixelsInMeter/2;
    v2 RenderEnd   = RenderStart + PixelsInMeter;
    DrawRectangle(RenderBuffer, RenderStart, RenderEnd, 0xFFFFFFFF);

    GameState->TotalTime += GameState->DeltaTime;
}
