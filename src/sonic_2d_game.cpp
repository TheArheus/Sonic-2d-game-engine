#include "sonic_2d_game.h"

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

internal void
InitializeTileMap(memory_block* Block, tile_map* TileMap, s32 SizeX, s32 SizeY)
{
    TileMap->TileMapWidth  = SizeX;
    TileMap->TileMapHeight = SizeY;
    TileMap->Tiles = PushArray(Block, u8, SizeX * SizeY);
}

internal u8
GetTileValue(tile_map* TileMap, u32 TileX, u32 TileY)
{
    u8 Result = TileMap->Tiles[TileY * TileMap->TileMapWidth + TileX];
    return Result;
}

internal void
SetTileValue(tile_map* TileMap, u32 TileX, u32 TileY, u8 Value)
{
    TileMap->Tiles[TileY * TileMap->TileMapWidth + TileX] = Value;
}

#if defined(__cplusplus)
extern "C" 
#endif
GAME_MAIN_RENDER_AND_UPDATE_LOOP(GameMainRenderAndUpdateLoop)
{
    game_state* GameState = (game_state*)GameMemory->PermamentStorage;

    r32 PixelsInMeter = 128.0f; // NOTE: I want to use this as Tile Size

    AllocateMemoryBlock(&GameState->World, (u8*)GameMemory->PermamentStorage + sizeof(game_state), GameMemory->PermamentStorageSize - sizeof(game_state));

    if(!GameMemory->IsInitialized)
    {
        GameMemory->IsInitialized = true;

        GameState->DeltaTime = Input->DeltaTimeForFrame;

        GameState->Player.P = V2(RenderBuffer->Width / 2.0f, RenderBuffer->Height / 2.0f);

        GameState->TestChunkSystem.TileChunks[0].TileMap = PushStruct(&GameState->World, tile_map);
        InitializeTileMap(&GameState->World, GameState->TestChunkSystem.TileChunks[0].TileMap, CHUNK_SIZE, CHUNK_SIZE);
        GameState->TestChunkSystem.TileChunkCount += 1;

        for(u32 TileY = 0;
            TileY < GameState->TestChunkSystem.TileChunks[0].TileMap->TileMapHeight;
            ++TileY)
        {
            for(u32 TileX = 0;
                TileX < GameState->TestChunkSystem.TileChunks[0].TileMap->TileMapWidth;
                ++TileX)
            {
                if((TileY % 2) == 0)
                {
                    if((TileX % 2) == 0)
                    {
                        SetTileValue(GameState->TestChunkSystem.TileChunks[0].TileMap, TileX, TileY, 1);
                    }
                    else
                    {
                        SetTileValue(GameState->TestChunkSystem.TileChunks[0].TileMap, TileX, TileY, 2);
                    }
                }
                else
                {
                    if((TileX % 2) == 0)
                    {
                        SetTileValue(GameState->TestChunkSystem.TileChunks[0].TileMap, TileX, TileY, 2);
                    }
                    else
                    {
                        SetTileValue(GameState->TestChunkSystem.TileChunks[0].TileMap, TileX, TileY, 1);
                    }
                }
            }
        }
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
                    GameState->Player.ddP.y +=  1.0f;
                }
                if(Controller->Down.EndedDown)
                {
                    GameState->Player.ddP.y += -1.0f;
                }
                if(Controller->Left.EndedDown)
                {
                    GameState->Player.ddP.x += -1.0f;
                }
                if(Controller->Right.EndedDown)
                {
                    GameState->Player.ddP.x +=  1.0f;
                }
            }
        }
    }


    // NOTE: Testing Chunking System
    for(u32 ChunkIndex = 0;
        ChunkIndex < GameState->TestChunkSystem.TileChunkCount;
        ++ChunkIndex)
    {
        // NOTE: Get chunks for their x + y position hash value
        // NOTE: Maybe this is not the most efficient way to implement chunking system
        tile_chunk* Chunk = GameState->TestChunkSystem.TileChunks + ChunkIndex;

        for (u32 TileY = 0;
            TileY < Chunk->TileMap->TileMapHeight;
            ++TileY)
        {
            for (u32 TileX = 0;
                TileX < Chunk->TileMap->TileMapWidth;
                ++TileX)
            {
                v2 Start = V2(TileX * PixelsInMeter, TileY * PixelsInMeter);
                v2 End = Start + PixelsInMeter;

                u32 Color = 0xFF000000;
                u8 ID = GetTileValue(Chunk->TileMap, TileX, TileY);
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

    GameState->Player.dP += GameState->Player.ddP * GameState->DeltaTime;
    GameState->Player.P  += GameState->Player.dP  * GameState->DeltaTime * PixelsInMeter;

    v2 RenderStart = GameState->Player.P - PixelsInMeter/2;
    v2 RenderEnd   = RenderStart + PixelsInMeter;
    DrawRectangle(RenderBuffer, RenderStart, RenderEnd, 0xFFFFFFFF);

    GameState->TotalTime += GameState->DeltaTime;
}
