#include "platform.h"

#include <windows.h>
#include <xinput.h>

#include "win32_game_engine.h"

/*
 * Plans on game:
 *
 * 1) Start working on physics and camera
 * 2) Working on renderer
 *    - SIMD
 *    - Better tile rendering
 *    - Sorting by z value
 *    - Some optimizations
 * 3) Start working on assets system
 * 4) Sound system
 * 5) GUI system
 *    - Main menu GUI
 *    - In-Game tile editor system
 *    - Level editing system
*/

global_variable bool IsRunning;
global_variable win32_draw_buffer GlobalBackBuffer;
global_variable s64 GlobalPerfCountFrequency;

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(xinput_get_state);
XINPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable xinput_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable xinput_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_


internal void
Win32LoadXInput()
{
    HMODULE XInputLibrary = LoadLibraryA("Xinput1_4.dll");
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("XInput1_3.dll");
    }
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("XInput1_0.dll");
    }
    if(XInputLibrary)
    {
        XInputGetState = (xinput_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (xinput_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

inline FILETIME
Win32GetLastWriteTime(char* Filename)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Attributes;
    if(GetFileAttributesExA(Filename, GetFileExInfoStandard, &Attributes))
    {
        LastWriteTime = Attributes.ftLastWriteTime;
    }

    return LastWriteTime;
}

internal win32_game_code
Win32LoadGameCode(char* SourceGameCode, char* TempGameCode)
{
    win32_game_code Result = {};

    CopyFile(SourceGameCode, TempGameCode, FALSE);
    Result.GameCodeLibrary = LoadLibraryA(TempGameCode);

    if(Result.GameCodeLibrary)
    {
        Result.IsValid = true;
        Result.LastCodeWrite = Win32GetLastWriteTime(SourceGameCode);

        Result.GameMainRenderAndUpdateLoop = 
            (game_main_render_and_update_loop*)GetProcAddress(Result.GameCodeLibrary, "GameMainRenderAndUpdateLoop");
    }

    if(!Result.IsValid)
    {
        Result.GameMainRenderAndUpdateLoop = 0;
    }

    return Result;
}

internal void
Win32UnloadGameCode(win32_game_code* GameCode)
{
    if(GameCode->GameCodeLibrary)
    {
        FreeLibrary(GameCode->GameCodeLibrary);
        GameCode->GameCodeLibrary = 0;
    }
    GameCode->IsValid = false;
    GameCode->GameMainRenderAndUpdateLoop = 0;
}

internal win32_window_dimension
Win32GetWindowDimensions(HWND Window)
{
    win32_window_dimension Result = {};
    RECT WindowRect = {};
    GetClientRect(Window, &WindowRect);

    Result.Width  = WindowRect.right  - WindowRect.left;
    Result.Height = WindowRect.bottom - WindowRect.top;

    return Result;
}

internal void
Win32ResizeDIBSection(win32_draw_buffer* BackBuffer, win32_window_dimension WindowDimensions)
{
    if(BackBuffer->Memory)
    {
        VirtualFree(BackBuffer->Memory, 0, MEM_RELEASE);
    }

    BackBuffer->BitmapInfo.bmiHeader.biSize        = sizeof(BITMAPINFO);
    BackBuffer->BitmapInfo.bmiHeader.biWidth       = WindowDimensions.Width;
    BackBuffer->BitmapInfo.bmiHeader.biHeight      = -WindowDimensions.Height;
    BackBuffer->BitmapInfo.bmiHeader.biPlanes      = 1;
    BackBuffer->BitmapInfo.bmiHeader.biBitCount    = 32;
    BackBuffer->BitmapInfo.bmiHeader.biCompression = BI_RGB;

    BackBuffer->Width  = WindowDimensions.Width;
    BackBuffer->Height = WindowDimensions.Height;

    s32 BytesPerPixel = sizeof(u32);
    BackBuffer->Pitch = BytesPerPixel * WindowDimensions.Width;

    int BitmapSize = BackBuffer->Pitch * WindowDimensions.Height;

    BackBuffer->Memory = VirtualAlloc(0, BitmapSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

internal void
Win32UpdateWindow(win32_draw_buffer* BackBuffer, HDC DeviceContext)
{
    StretchDIBits(DeviceContext, 
                  0, 0, BackBuffer->Width, BackBuffer->Height, 
                  0, 0, BackBuffer->Width, BackBuffer->Height, 
                  BackBuffer->Memory, &BackBuffer->BitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal void
Win32ProcessKeyboard(game_button_state* Button, b32 IsDown)
{
    if(Button->EndedDown != IsDown)
    {
        Button->EndedDown = IsDown;
        ++Button->HalfTransitionCount;
    }
}

internal r32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    r32 Result = 0;

    if(Value < -DeadZoneThreshold)
    {
        Result = (r32)(Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold);
    }
    else if(Value > DeadZoneThreshold)
    {
        Result = (r32)(Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold);
    }

    return Result;
}

internal void
Win32ProcessXInputButton(DWORD XInputButtonState, game_button_state* NewState, game_button_state* OldState, DWORD ButtonBit)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal void
Win32DispatchMessages(game_controller_input* KeyboardController)
{
    MSG Message = {};

    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 KeyCode = (u32)Message.wParam;

                b32 IsDown  = ((Message.lParam & (1 << 31)) == 0);
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);

                if(IsDown != WasDown)
                {
                    if(KeyCode == 'W')
                    {
                        Win32ProcessKeyboard(&KeyboardController->Up, IsDown);
                    }
                    else if(KeyCode == 'A')
                    {
                        Win32ProcessKeyboard(&KeyboardController->Left, IsDown);
                    }
                    else if(KeyCode == 'S')
                    {
                        Win32ProcessKeyboard(&KeyboardController->Down, IsDown);
                    }
                    else if(KeyCode == 'D')
                    {
                        Win32ProcessKeyboard(&KeyboardController->Right, IsDown);
                    }
                    else if(KeyCode == 'Q')
                    {
                        Win32ProcessKeyboard(&KeyboardController->ShoulderLeft, IsDown);
                    }
                    else if(KeyCode == 'E')
                    {
                        Win32ProcessKeyboard(&KeyboardController->ShoulderRight, IsDown);
                    }
                    else if(KeyCode == VK_UP)
                    {
                        Win32ProcessKeyboard(&KeyboardController->ActionUp, IsDown);
                    }
                    else if(KeyCode == VK_DOWN)
                    {
                        Win32ProcessKeyboard(&KeyboardController->ActionDown, IsDown);
                    }
                    else if(KeyCode == VK_LEFT)
                    {
                        Win32ProcessKeyboard(&KeyboardController->ActionLeft, IsDown);
                    }
                    else if(KeyCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboard(&KeyboardController->ActionRight, IsDown);
                    }
                    else if(KeyCode == VK_SPACE)
                    {
                        Win32ProcessKeyboard(&KeyboardController->Back, IsDown);
                    }
                    else if(KeyCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboard(&KeyboardController->Start, IsDown);
                    }
                }
                if(IsDown)
                {
                    b32 AltKeyWasDown = (Message.lParam & (1 << 29));
                    if((KeyCode == VK_F4) && AltKeyWasDown)
                    {
                        IsRunning = false;
                    }
                }

            } break;
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

internal game_controller_input*
GetController(game_input* Input, s32 Index)
{
    game_controller_input* Result = Input->Controllers + Index;
    return Result;
}

LRESULT CALLBACK 
MainWindowCallback(HWND Window, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = 0;
    switch(uMsg)
    {
#if 0
        // NOTE: Only if I want to make a window 
        // to be resizable
        case WM_SIZE:
        {
            win32_window_dimension ResizedDimension = Win32GetWindowDimensions(Window);
            Win32ResizeDIBSection(&GlobalBackBuffer, ResizedDimension);
        } break;
#endif

        case WM_CLOSE:
        {
            IsRunning = false;
        } break;

        case WM_DESTROY:
        {
            IsRunning = false;
            PostQuitMessage(0);
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint = {};

            HDC PaintContext = BeginPaint(Window, &Paint);
            Win32UpdateWindow(&GlobalBackBuffer, PaintContext);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProc(Window, uMsg, wParam, lParam);
        }
    }

    return Result;
}

internal void
Win32GetEXEFileName(char** EXEFileNameResult, char** OnePastSlashResult)
{
    char EXEFileName[MAX_PATH];
    DWORD SizeOfFileName = GetModuleFileName(0, EXEFileName, sizeof(EXEFileName));
    char* OnePastSlash = EXEFileName;
    for(char* Scan = EXEFileName;
        *Scan;
        ++Scan)
    {
        if(*Scan == '\\')
        {
            OnePastSlash = Scan + 1;
        }
    }

    *EXEFileNameResult = EXEFileName;
    *OnePastSlashResult = OnePastSlash;
}

internal void
CatStrings(size_t SizeSourceA, char* SourceA, 
           size_t SizeSourceB, char* SourceB, 
           char* Result)
{
    for(s32 IndexA = 0;
        IndexA < SizeSourceA;
        ++IndexA)
    {
        *Result++ = SourceA[IndexA];
    }

    for(s32 IndexB = 0;
        IndexB < SizeSourceB;
        ++IndexB)
    {
        *Result++ = SourceB[IndexB];
    }

    *Result++ = '\0';
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline r32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    r32 Result = (r32)(End.QuadPart - Start.QuadPart) / (r32)GlobalPerfCountFrequency;
    return Result;
}

int CALLBACK 
WinMain(HINSTANCE Instance, 
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    // NOTE: Counts Per Second
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    UINT DesiredSchedulerMS = 1;
    b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    char* EXEFileName;
    char* OnePastSlash;
    Win32GetEXEFileName(&EXEFileName, &OnePastSlash);

    char SourceGameCodeDLLFileName[] = "sonic_2d_game.dll";
    char SourceGameCodeDLLFullPath[MAX_PATH];
    CatStrings(OnePastSlash - EXEFileName, EXEFileName, 
               sizeof(SourceGameCodeDLLFileName) - 1, SourceGameCodeDLLFileName, 
               SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFileName[] = "sonic_2d_game_temp.dll";
    char TempGameCodeDLLFullPath[MAX_PATH];
    CatStrings(OnePastSlash - EXEFileName, EXEFileName, 
               sizeof(TempGameCodeDLLFileName) - 1, TempGameCodeDLLFileName, 
               TempGameCodeDLLFullPath);

    WNDCLASS WindowClass = {};

    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "Game Engine";

    // win32_window_dimension MainWindowDimension = { 1224, 714 };
    win32_window_dimension MainWindowDimension = { 1240, 720 };
    Win32ResizeDIBSection(&GlobalBackBuffer, MainWindowDimension);
    Win32LoadXInput();

    s32 MonitorRefreshHz = 60;

    DEVMODEA DisplaySettings;
    if(SUCCEEDED(EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &DisplaySettings)))
    {
        MonitorRefreshHz = DisplaySettings.dmDisplayFrequency;
    }

    s32 GameUpdateHz = MonitorRefreshHz / 2;
    r32 TargetSecondsPerFrame = 1.0f / (r32)GameUpdateHz;

    if(RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(0, "Game Engine", 0, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT, MainWindowDimension.Width, MainWindowDimension.Height, NULL, NULL, Instance, NULL);

        if(WindowHandle)
        {
            LARGE_INTEGER LastCounter = Win32GetWallClock();

            IsRunning = true;

#if INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(4);
#else
            LPVOID BaseAddress = 0;
#endif

            game_memory GameMemory = {};
            GameMemory.PermamentStorageSize = Megabytes((u64)64);
            GameMemory.TransientStorageSize = Gigabytes((u64) 2);

            u64 TotalMemory       = GameMemory.PermamentStorageSize + GameMemory.TransientStorageSize;
            void* GameMemoryBlock = VirtualAlloc(BaseAddress, TotalMemory, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

            GameMemory.PermamentStorage = GameMemoryBlock;
            GameMemory.TransientStorage = (u8*)GameMemoryBlock + GameMemory.PermamentStorageSize;

            game_input Input[2]  = {};
            game_input* NewInput = &Input[0];
            game_input* OldInput = &Input[1];

            win32_game_code GameCode = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);

            ShowWindow(WindowHandle, ShowCode);
            while(IsRunning)
            {
                NewInput->DeltaTimeForFrame = TargetSecondsPerFrame;
                FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                if(CompareFileTime(&GameCode.LastCodeWrite, &NewDLLWriteTime))
                {
                    Win32UnloadGameCode(&GameCode);
                    GameCode = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
                }

                POINT MousePoint;
                GetCursorPos(&MousePoint);
                ScreenToClient(WindowHandle, &MousePoint);

                Win32ProcessKeyboard(&NewInput->MouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
                Win32ProcessKeyboard(&NewInput->MouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
                Win32ProcessKeyboard(&NewInput->MouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
                Win32ProcessKeyboard(&NewInput->MouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
                Win32ProcessKeyboard(&NewInput->MouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));

                game_controller_input* NewKeyboardInput = GetController(NewInput, 0);
                game_controller_input* OldKeyboardInput = GetController(OldInput, 0);
                *NewKeyboardInput = {};
                NewKeyboardInput->IsConnected = true;

                Win32DispatchMessages(NewKeyboardInput);

                for(DWORD ControllerIndex = 0;
                    ControllerIndex < XUSER_MAX_COUNT;
                    ++ControllerIndex)
                {
                    DWORD ControllerIndexToUse = ControllerIndex + 1;
                    game_controller_input* NewController = GetController(NewInput, ControllerIndexToUse);
                    game_controller_input* OldController = GetController(OldInput, ControllerIndexToUse);
                    
                    XINPUT_STATE ControllerState = {};

                    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;
                        NewController->IsConnected = true;

                        NewController->StickAvarageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                        NewController->StickAvarageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                        if((NewController->StickAvarageX != 0) || 
                           (NewController->StickAvarageY != 0))
                        {
                            NewController->IsAnalog = true;
                        }

                        if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                        {
                            NewController->StickAvarageY = 1;
                            NewController->IsAnalog = false;
                        }
                        if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                        {
                            NewController->StickAvarageX = -1;
                            NewController->IsAnalog = false;
                        }
                        if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                        {
                            NewController->StickAvarageX = 1;
                            NewController->IsAnalog = false;
                        }
                        if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                        {
                            NewController->StickAvarageY = -1;
                            NewController->IsAnalog = false;
                        }

                        r32 Threshold = 0.5f;
                        Win32ProcessXInputButton((NewController->StickAvarageX > Threshold)?1:0, &NewController->Right, &OldController->Right, 1);
                        Win32ProcessXInputButton((NewController->StickAvarageX < Threshold)?1:0, &NewController->Left,  &OldController->Left,  1);
                        Win32ProcessXInputButton((NewController->StickAvarageY > Threshold)?1:0, &NewController->Up,    &OldController->Up,    1);
                        Win32ProcessXInputButton((NewController->StickAvarageY < Threshold)?1:0, &NewController->Down,  &OldController->Down,  1);

                        Win32ProcessXInputButton(Pad->wButtons, &NewController->ActionDown,    &OldController->ActionDown,    XINPUT_GAMEPAD_A);
                        Win32ProcessXInputButton(Pad->wButtons, &NewController->ActionLeft,    &OldController->ActionLeft,    XINPUT_GAMEPAD_B);
                        Win32ProcessXInputButton(Pad->wButtons, &NewController->ActionRight,   &OldController->ActionRight,   XINPUT_GAMEPAD_X);
                        Win32ProcessXInputButton(Pad->wButtons, &NewController->ActionUp,      &OldController->ActionUp,      XINPUT_GAMEPAD_Y);
                        Win32ProcessXInputButton(Pad->wButtons, &NewController->ShoulderLeft,  &OldController->ShoulderLeft,  XINPUT_GAMEPAD_LEFT_SHOULDER);
                        Win32ProcessXInputButton(Pad->wButtons, &NewController->ShoulderRight, &OldController->ShoulderRight, XINPUT_GAMEPAD_RIGHT_SHOULDER);
                    }
                }

                game_render_buffer RenderBuffer = {};
                RenderBuffer.Memory = GlobalBackBuffer.Memory;
                RenderBuffer.Width  = GlobalBackBuffer.Width;
                RenderBuffer.Height = GlobalBackBuffer.Height;
                RenderBuffer.Pitch  = GlobalBackBuffer.Pitch;

                if(GameCode.GameMainRenderAndUpdateLoop)
                {
                    GameCode.GameMainRenderAndUpdateLoop(&GameMemory, NewInput, &RenderBuffer);
                }

                LARGE_INTEGER WorkCounter = Win32GetWallClock();
                r32 SecondsElapsedForWork = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                r32 SecondsElapsedForFrame = SecondsElapsedForWork;
                if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                {
                    while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        if(SleepIsGranular)
                        {
                            DWORD SleepMS = (DWORD)(1000.0f*(TargetSecondsPerFrame - SecondsElapsedForFrame));
                            if(SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }
                        }
                        SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                    }
                }
                else
                {
                    // NOTE: Missed framerate
                }

                HDC WindowPaintContext = GetDC(WindowHandle);
                Win32UpdateWindow(&GlobalBackBuffer, WindowPaintContext);
                ReleaseDC(WindowHandle, WindowPaintContext);

#if 0
                s32 MillisecondsPerFrame = (s32)((1000 * CounterElapsed) / PerfCountFrequency);
                s32 FPS = PerfCountFrequency / CounterElapsed;
#endif

                game_input* Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;

                LARGE_INTEGER EndCounter = Win32GetWallClock();
                LastCounter = EndCounter;
            }
        }
        else
        {
        }
    }
    else
    {
    }

    return 0;
}
