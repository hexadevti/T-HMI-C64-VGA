/*
 Copyright (C) 2024 retroelec <retroelec42@gmail.com>

 This program is free software; you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation; either version 3 of the License, or (at your
 option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 for more details.

 For the complete text of the GNU General Public License see
 http://www.gnu.org/licenses/.
*/
#include "Config.h"
#ifdef USE_VGADISPLAY
#include "HardwareInitializationException.h"
#include "VGADISPLAY.h"
#include "Arduino.h"
#include <esp_log.h>


static const char *TAG = "VGADISPLAY";

void VGADISPLAY::init() {
  //Mode mode(16, 96, 48, 640, 4, 2, 30, 240, 23760000, 0, 0, 2);
  
  PinConfig pins(-1,-1,-1,Config::RED0,Config::RED1,
                 -1,-1,-1,-1,Config::GREEN0,Config::GREEN1,
                 -1,-1,-1,Config::BLUE0,Config::BLUE1,
                 Config::HSYNC,Config::VSYNC);
  while (!vga.init(pins, Mode::MODE_320x240x60, 16, 3))
    delay(1);
  ESP_LOGI(TAG, "VGA Initiated.");
  vga.show();
  vga.start();
  vga.fillRect(0,0,Config::LCDWIDTH,Config::LCDHEIGHT,0x043f);
}

void VGADISPLAY::drawFrame(uint16_t frameColor) {
}

void VGADISPLAY::drawBitmap(uint16_t *bitmap) {
  int x_offset = 0;
  int y_offset = 20;

  uint16_t pos = 0;
  for (size_t y = 0; y < 200; y++)
    {
      for (size_t x = 0; x < 320; x++)
      {
        vga.dotFast16(x+x_offset, y+y_offset, bitmap[pos]);
        pos++;        
      }
    }
}

const uint16_t *VGADISPLAY::getC64Colors() const { return c64Colors; }
#endif
