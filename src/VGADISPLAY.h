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
#ifndef VGADISPLAY_H
#define VGADISPLAY_H

#include "Config.h"
#include "DisplayDriver.h"
#include "VGA.h"
#include <cstdint>

// no elegant/simple solution for max() at compile time in C++11
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

class VGADISPLAY : public DisplayDriver {
private:
static const uint16_t c64_black = 0x0000;
  static const uint16_t c64_white = 0xffff;
  static const uint16_t c64_red = 0x8000;
  static const uint16_t c64_turquoise = 0xa7fc;
  static const uint16_t c64_purple = 0xc218;
  static const uint16_t c64_green = 0x064a;
  static const uint16_t c64_blue = 0x0014;
  static const uint16_t c64_yellow = 0xe74e;
  static const uint16_t c64_orange = 0xd42a;
  static const uint16_t c64_brown = 0x6200;
  static const uint16_t c64_lightred = 0xfbae;
  static const uint16_t c64_grey1 = 0x3186;
  static const uint16_t c64_grey2 = 0x73ae;
  static const uint16_t c64_lightgreen = 0xa7ec;
  static const uint16_t c64_lightblue = 0x043f;
  static const uint16_t c64_grey3 = 0xb5d6;
  VGA vga;
  
  const uint16_t c64Colors[16] = {
      c64_black,  c64_white,      c64_red,       c64_turquoise,
      c64_purple, c64_green,      c64_blue,      c64_yellow,
      c64_orange, c64_brown,      c64_lightred,  c64_grey1,
      c64_grey2,  c64_lightgreen, c64_lightblue, c64_grey3};

  static const uint16_t BORDERWIDTH = (Config::LCDWIDTH - 320) / 2;
  static const uint16_t BORDERHEIGHT = (Config::LCDHEIGHT - 200) / 2;
  static const uint16_t FRAMEMEMSIZE =
      MAX(320 * BORDERHEIGHT, BORDERWIDTH *Config::LCDHEIGHT);

public:
  void init() override;
  void drawFrame(uint16_t frameColor) override;
  void drawBitmap(uint16_t *bitmap) override;
  const uint16_t *getC64Colors() const override;
};

#endif // VGADISPLAY_H
