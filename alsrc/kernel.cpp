/*
 * This file is part of Asea OS.
 * Copyright (C) 2018 - 2019 Ivan Kmeťo
 *
 * Asea OS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Asea OS is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Asea OS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common/types.h>
#include <gdt.h>
#include <memmgr.h>
#include <hwcom/interrupts.h>
#include <hwcom/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
//#include <drivers/vga.h>
#include <System/headers/sysnfo.h>
#include <System/headers/sysmsgs.h>
#include <System/lib/asl.h>
#include <System/core.h>

using namespace asea;
using namespace asea::common;
using namespace asea::drivers;
using namespace asea::hwcom;
using namespace asea::System::headers;
using namespace asea::System::lib;
using namespace asea::System::core;


void cursor_enable(uint8_t cursor_start, uint8_t cursor_end) {
    asl::io::outb(0x3D4, 0x0A);
    asl::io::outb(0x3D5, (asl::io::inb(0x3D5) & 0xC0) | cursor_start);
    asl::io::outb(0x3D4, 0x0B);
    asl::io::outb(0x3D5, (asl::io::inb(0x3D5) & 0xE0) | cursor_end);
}

void cursor_disable() {
    asl::io::outb(0x3D4, 0x0A);
    asl::io::outb(0x3D5, 0x20);
}

void cursor_update(int32_t x, int32_t y) {
    uint16_t pos = y * 80 + x;

    asl::io::outb(0x3D4, 0x0F);
    asl::io::outb(0x3D5, (uint8_t) (pos & 0xFF));
    asl::io::outb(0x3D4, 0x0E);
    asl::io::outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void printf(char* str) {
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;
    static uint8_t x = 0, y = 0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                y++;
                x = 0;
                break;

            case '\b':
                if (x <= 0)
                {
                    x = 0;
                }
                else {
                    x -= 1;
                }
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
                break;

            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            y++;
            x = 0;
        }

        if (y >= 25)
        {
            for(y = 0; y < 25; y++)
            for (x =0; x < 80; x++)
            VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';

            x = 0;
            y = 0;
        }
    }
    cursor_update(x, y);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
    public:
        void OnKeyDown(char c)
        {
            char* foo = " ";
            foo[0] = c;
            printf(foo);
        }
};

class MouseToConsole : public MouseEventHandler
{
        int8_t x, y;
    public:
        MouseToConsole()
        {
            static uint16_t* VideoMemory = (uint16_t*)0xb8000;
            x = 40;
            y = 12;
            VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
                                | ((VideoMemory[80*y+x] & 0x0F00) << 4)
                                | ((VideoMemory[80*y+x] & 0x00FF));
        }

    void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
                            | ((VideoMemory[80*y+x] & 0x0F00) << 4)
                            | ((VideoMemory[80*y+x] & 0x00FF));

        x += xoffset;
        if(x >= 80) x = 79;
        if(x < 0) x = 0;

        y -= yoffset;
        if(y >= 25) y = 24;
        if(y < 0) y = 0;

        VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
                            | ((VideoMemory[80*y+x] & 0x0F00) << 4)
                            | ((VideoMemory[80*y+x] & 0x00FF));
    }
};

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    cursor_disable();
    cursor_enable(0, 15);
    cursor_update(0, 0);

    AseaSystemInfo sysInfo;
    AseaSystemMessages sysMsgs;

    sysInfo.AS_PrintSysInfoMsg(SYSINFOMSG_COPYRIGHTVR);

    GlobalDescriptorTable gdt;
    InterruptManager interrupts(&gdt);

    /*
    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
    size_t heap = 10*1024*1024;
    MemMgr memMgr(heap, (*memupper)*1024 - heap - 10*1024);

    printf("heap: 0x");
    asl::printfHex((heap >> 24) & 0xFF);
    asl::printfHex((heap >> 16) & 0xFF);
    asl::printfHex((heap >> 8 ) & 0xFF);
    asl::printfHex((heap      ) & 0xFF);
    void* allocated = memMgr.malloc(1024);

    printf("\nallocated: 0x");
    asl::printfHex(((size_t)allocated >> 24) & 0xFF);
    asl::printfHex(((size_t)allocated >> 16) & 0xFF);
    asl::printfHex(((size_t)allocated >> 8 ) & 0xFF);
    asl::printfHex(((size_t)allocated      ) & 0xFF);
    printf("\n\n");
    */

    sysMsgs.AS_StatusMsgInf(STATUSMSG_INF_INITIALIZING);
    DriverManager drvManager;

    PrintfKeyboardEventHandler kbhandler;
    KeyboardDriver keyboard(&interrupts, &kbhandler);
    drvManager.AddDriver(&keyboard);

    MouseToConsole mousehandler;
    MouseDriver mouse(&interrupts, &mousehandler);
    drvManager.AddDriver(&mouse);

    PCInterconnectController PCIController;
    PCIController.SelectDrivers(&drvManager, &interrupts);

    drvManager.ActivateAll();
    sysMsgs.AS_StatusMsg(STATUSMSG_OK, "Hardware Initialization\n");

    interrupts.Activate();
    sysMsgs.AS_StatusMsg(STATUSMSG_OK, "Hardware Interrupts\n\n");

    while(1);
}
