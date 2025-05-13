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
#ifndef USBKB_H
#define USBKB_H

#include <cstdint>
#include <string>

class C64Emu;
class ExternalCmds;

class USBKB {
private:
  C64Emu *c64emu;
  uint8_t sentdc01;
  uint8_t sentdc00;


public:
  bool deviceConnected;
  uint8_t *buffer;
  uint8_t shiftctrlcode;
  uint8_t keypresseddowncnt;
  uint8_t virtjoystickvalue;
  bool keypresseddown;
  bool detectreleasekey;

  USBKB();
  void init(C64Emu *c64emu);
  void handleKeyPress();
  uint8_t getdc01(uint8_t dc00, bool xchgports);
  uint8_t getKBJoyValue(bool port2);
  void setKbcodes(uint8_t sentdc01, uint8_t sentdc00);
};

#endif // USBKB_H
