//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2017 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#include <cmath>
#include <csignal>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <system_error>

#include "OledBitmap.h"
#include "OledI2C.h"
#include "LinuxKeys.h"

//-------------------------------------------------------------------------

using Bitmap = SSD1306::OledBitmap<SSD1306::OledI2C::Width,
                                   SSD1306::OledI2C::Height>;

using Point = SSD1306::OledPoint;
//-------------------------------------------------------------------------

namespace
{
volatile static std::sig_atomic_t run = 1;
}

//-------------------------------------------------------------------------

static void
signalHandler(
    int signalNumber)
{
    switch (signalNumber)
    {
    case SIGINT:
    case SIGTERM:

        run = 0;
        break;
    };
}

//-------------------------------------------------------------------------

int
newApple(
    SSD1306::OledPixel& pixels)
{
    for (int row = 0 ; row < pixels.height() ; ++row)
    {
        for (int col = 0 ; col < pixels.width() ; ++col)
        {
            Point p{col, row};
            pixels.unsetPixel(p);
        }
    }

    return 0;
}

//-------------------------------------------------------------------------

int
main()
{
    try
    {
        constexpr std::array<int, 2> signals{SIGINT, SIGTERM};

        for (auto signal : signals)
        {
            if (std::signal(signal, signalHandler) == SIG_ERR)
            {
                std::string what{"installing "};
                what += strsignal(signal);
                what += " signal handler";

                throw std::system_error(errno,
                                        std::system_category(),
                                        what);
            }
        }

       SSD1306::OledI2C oled{"/dev/i2c-0", 0x3C};

        Bitmap pixels;

        newApple(pixels);
        oled.setFrom(pixels);
        oled.displayUpdate();

        LinuxKeys linuxKeys;
        LinuxKeys::PressedKey key;

        std::ifstream file("video.rawr", std::ios::binary);

        //-----------------------------------------------------------------

        while (run)
        {
            key = linuxKeys.pressed();
            if (key.isPressed)
            {
                run = 0;
                break;
            }

            for (int y = 0; y < 64; y+=2){
                for (int x = 0; x < 128; x+=4){
                    char c;
                    Point p{x, y};
                    if (!file.read(&c, 1)) {
                        run=0;
                        break;
                    }
                    if (c == 0x2E)        // '.'
                        pixels.unsetPixel(p);
                    else if (c == 0x20)   // ' '
                        pixels.setPixel(p);
                }
            }
            oled.setFrom(pixels);
            oled.displayUpdate();
            usleep(30000);
        }

        //-----------------------------------------------------------------

        file.close();
        oled.clear();
        oled.displayUpdate();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return 0;
}

