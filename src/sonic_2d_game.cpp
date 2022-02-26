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

#if defined(__cplusplus)
extern "C" 
#endif
GAME_MAIN_RENDER_AND_UPDATE_LOOP(GameMainRenderAndUpdateLoop)
{
    game_state* GameState = (game_state*)GameMemory->PermamentStorage;

    r32 PixelsInMeter = 32.0f; // NOTE: I want to use this as Tile Size

    AllocateMemoryBlock(&GameState->World, (u8*)GameMemory->PermamentStorage + sizeof(game_state), GameMemory->PermamentStorageSize - sizeof(game_state));

    if(!GameMemory->IsInitialized)
    {
        GameMemory->IsInitialized = true;

        GameState->DeltaTime = Input->DeltaTimeForFrame;

        GameState->Player.P = V2(RenderBuffer->Width / 2.0f, RenderBuffer->Height / 2.0f);
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

    GameState->Player.dP += GameState->Player.ddP * GameState->DeltaTime;
    GameState->Player.P  += GameState->Player.dP  * GameState->DeltaTime * PixelsInMeter;

    v2 RenderStart = GameState->Player.P - PixelsInMeter/2;
    v2 RenderEnd   = RenderStart + PixelsInMeter;
    DrawRectangle(RenderBuffer, RenderStart, RenderEnd, 0xFFFFFFFF);

    GameState->TotalTime += GameState->DeltaTime;
}
