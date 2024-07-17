#include <coreinit/foreground.h>
#include <coreinit/screen.h>
#include <iostream>
#include <nn/ccr.h>
#include <nsysccr/cdc.h>
#include <proc_ui/procui.h>
#include <vpad/input.h>
#include <coreinit/stopwatch.h>
#include <whb/proc.h>

constexpr static size_t formatBufferSize = 1024;
static char formatBuffer[formatBufferSize];

void ScreenUpdate(uint32_t seconds)
{


    auto pairingState = CCRSysGetPairingState();
    if (pairingState == CCR_SYS_PAIRING_IN_PROGRESS)
    {
        OSScreenClearBufferEx(SCREEN_TV, 0x00FF0000);

        std::snprintf(formatBuffer, formatBufferSize, "State: Pairing %d/60s", seconds);
        OSScreenPutFontEx(SCREEN_TV, 0, 10, formatBuffer);

        uint32_t pinCode;
        CCRSysGetPincode(&pinCode);
        std::snprintf(formatBuffer, formatBufferSize, "Pairing pin: %04d", pinCode);
        OSScreenPutFontEx(SCREEN_TV, 0, 12, formatBuffer);
        OSScreenPutFontEx(SCREEN_TV, 0, 13, "Spade = 0, Heart = 1, Diamond = 2, Club = 3");
    }
    else
    {
        OSScreenClearBufferEx(SCREEN_DRC, 0x7f'7f'7f'00);
        OSScreenPutFontEx(SCREEN_DRC, 24, 8, "Look at the TV");
        OSScreenFlipBuffersEx(SCREEN_DRC);

        OSScreenClearBufferEx(SCREEN_TV, 0x0000FF00);
        OSScreenPutFontEx(SCREEN_TV, 0, 10, "State: Not pairing");
        OSScreenPutFontEx(SCREEN_TV, 0, 12, "Press A to start pairing");
    }
    OSScreenFlipBuffersEx(SCREEN_TV);
}

int main()
{
    WHBProcInit();
    VPADInit();
    CCRSysInit();
    OSScreenInit();

    auto *tvBuffer = std::aligned_alloc(0x100, OSScreenGetBufferSizeEx(SCREEN_TV));
    OSScreenSetBufferEx(SCREEN_TV, tvBuffer);
    OSScreenEnableEx(SCREEN_TV, TRUE);

    auto *drcBuffer = std::aligned_alloc(0x100, OSScreenGetBufferSizeEx(SCREEN_DRC));
    OSScreenSetBufferEx(SCREEN_DRC, drcBuffer);
    OSScreenEnableEx(SCREEN_DRC, TRUE);


    auto res = CCRCDCSetMultiDrc(2);
    if (res != 0)
    {
        std::snprintf(formatBuffer, formatBufferSize, "Setting multi-DRC failed: %d", res);
        OSScreenPutFontEx(SCREEN_TV, 0, 10, formatBuffer);
        while (WHBProcIsRunning())
        {}
    }
    else
    {
        VPADStatus prevStatus = {};
        VPADStatus status = {};
        VPADReadError error;

        OSStopwatch stopwatch;
        OSInitStopwatch(&stopwatch, "PairTimer");
        while (WHBProcIsRunning())
        {
            ScreenUpdate(OSTicksToSeconds(OSCheckStopwatch(&stopwatch)));

            auto samples = VPADRead(VPAD_CHAN_0, &status, 1, &error);
            if (samples == 0)
                continue;
            if ((status.hold & VPAD_BUTTON_A) && (~prevStatus.hold & VPAD_BUTTON_A))
            {
                CCRSysStartPairing(1, 60);
                OSStartStopwatch(&stopwatch);
            }
        }
        free(tvBuffer);
        free(drcBuffer);

    }

    CCRSysExit();
    VPADShutdown();
    WHBProcShutdown();
    return 0;
}
