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
#include "usbkb.h"
#include "C64Emu.h"
#include "Config.h"
#include "ExternalCmds.h"
#include "Joystick.h"
#include <cstring>
#include <esp_log.h>
#include "EspUsbHost.h"

static const char *TAG = "USBKB";

static const uint8_t NUMOFCYCLES_KEYPRESSEDDOWN = 3;

static const uint8_t VIRTUALJOYSTICKLEFT_ACTIVATED = 0x01;
static const uint8_t VIRTUALJOYSTICKLEFT_DEACTIVATED = 0x81;
static const uint8_t VIRTUALJOYSTICKRIGHT_ACTIVATED = 0x02;
static const uint8_t VIRTUALJOYSTICKRIGHT_DEACTIVATED = 0x82;
static const uint8_t VIRTUALJOYSTICKUP_ACTIVATED = 0x04;
static const uint8_t VIRTUALJOYSTICKUP_DEACTIVATED = 0x84;
static const uint8_t VIRTUALJOYSTICKDOWN_ACTIVATED = 0x08;
static const uint8_t VIRTUALJOYSTICKDOWN_DEACTIVATED = 0x88;



#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
  ((byte) & 0x80 ? '1' : '0'),     \
      ((byte) & 0x40 ? '1' : '0'), \
      ((byte) & 0x20 ? '1' : '0'), \
      ((byte) & 0x10 ? '1' : '0'), \
      ((byte) & 0x08 ? '1' : '0'), \
      ((byte) & 0x04 ? '1' : '0'), \
      ((byte) & 0x02 ? '1' : '0'), \
      ((byte) & 0x01 ? '1' : '0')

class ExeExtCmdTask
{
private:
  TaskHandle_t exeExtCmdTaskHandle;
  USBKB &usbkb;
  ExternalCmds &externalCmds;
  uint8_t type;

  static void exeExternalCmd(void *parameter)
  {
    ExeExtCmdTask *instance = static_cast<ExeExtCmdTask *>(parameter);
    instance->type =
        instance->externalCmds.executeExternalCmd(instance->usbkb.buffer);
    // signal that task is done using a task notification
    BaseType_t notifyResult = xTaskNotifyGive(instance->exeExtCmdTaskHandle);
    if (notifyResult == pdPASS)
    {
      ESP_LOGD(TAG, "Notification sent successfully");
    }
    else
    {
      ESP_LOGD(TAG, "Failed to send notification");
    }
    // delete task
    vTaskDelete(NULL);
  }

public:
  ExeExtCmdTask(USBKB &usbkb, ExternalCmds &externalCmds)
      : exeExtCmdTaskHandle(NULL), usbkb(usbkb), externalCmds(externalCmds) {}

  uint8_t exe()
  {
    exeExtCmdTaskHandle = xTaskGetCurrentTaskHandle();
    xTaskCreate(ExeExtCmdTask::exeExternalCmd, // Task function
                "externalCmdTask",             // Name of the task
                10000,                         // Stack size (in words)
                this,                          // Task input parameter
                1,                             // Priority of the task
                NULL                           // Task handle
    );
    // wait for task to be completed
    uint32_t ulNotificationValue;
    BaseType_t result = xTaskNotifyWait(
        0x00,                 // No bits to clear on entry
        ULONG_MAX,            // Clear all bits on exit
        &ulNotificationValue, // Where to store the notification value
        pdMS_TO_TICKS(5000)   // Wait max 5 seconds
    );
    if (result == pdPASS)
    {
      // Return the stored return value
      ESP_LOGD(TAG, "task completed, type = %d", type);
      return type;
    }
    else
    {
      ESP_LOGD(TAG, "failed to send notification or timed out");
      return 0;
    }
  }
};

class MyEspUsbHost : public EspUsbHost
{
  private:
    volatile bool control = false;
    volatile bool shift = false;
    volatile bool left_shift = false;
    volatile bool right_shift = false;
    volatile bool left_alt = false;
    volatile bool left_win = false;
    volatile bool right_alt = false;
    volatile bool right_win = false;
  
    const uint8_t scancodes[128] PROGMEM = {
        0xff, 0xff, 0xff, 0xff, 0x0A, 0x1C, 0x14, 0x12,
        0x0E, 0x15, 0x1A, 0x1D, 0x21, 0x22, 0x25, 0x2A,
        0x24, 0x27, 0x26, 0x29, 0x3E, 0x11, 0x0D, 0x16,
        0x1E, 0x1F, 0x09, 0x17, 0x19, 0x0C, 0x38, 0x3B,
        0x08, 0x0B, 0x10, 0x13, 0x18, 0x1B, 0x20, 0x23,
        0x01, 0xff, 0x00, 0xff, 0x3C, 0x2b, 0x35, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    void onKeyboard(hid_keyboard_report_t report, hid_keyboard_report_t last_report)
    {

      // ESP_LOGE(TAG, "Key0=[0x%02x]->[0x%02x]", last_report.keycode[0], report.keycode[0]);
      //  ESP_LOGE(TAG, "modifier=["BYTE_TO_BINARY_PATTERN"]->["BYTE_TO_BINARY_PATTERN"], Key0=[0x%02x]->[0x%02x], Key1=[0x%02x]->[0x%02x], Key2=[0x%02x]->[0x%02x], Key3=[0x%02x]->[0x%02x], Key4=[0x%02x]->[0x%02x], Key5=[0x%02x]->[0x%02x]",
      //           BYTE_TO_BINARY(last_report.modifier), BYTE_TO_BINARY(report.modifier), last_report.keycode[0], report.keycode[0], last_report.keycode[1], report.keycode[1], last_report.keycode[2], report.keycode[2], last_report.keycode[3], report.keycode[3], last_report.keycode[4], report.keycode[4],last_report.keycode[5], report.keycode[5]);

      control = (report.modifier & 0b00010001) != 0;
      shift = (report.modifier & 0b00100010) != 0;
      left_shift = (report.modifier & 0b00000010) != 0;
      right_shift = (report.modifier & 0b00100000) != 0;
      left_alt = (report.modifier & 0b00000100) != 0;
      right_alt = (report.modifier & 0b01000000) != 0;
      left_win = (report.modifier & 0b00001000) != 0;
      right_win = (report.modifier & 0b10000000) != 0;

      // printLog(control ? "Control" : "");
      // printLog(shift ? "shift" : "");
      // printLog(left_alt ? "left_alt" : "");
      // printLog(left_win ? "left_win" : "");
      // printLog(right_alt ? "right_alt" : "");
      // printLog(right_win ? "right_win" : "");
      // Serial.println();
      if (last_report.keycode[1] == report.keycode[0] && last_report.keycode[1] != 0)
      { // up first key
        onKeyUp(last_report.keycode[0]);
      }
      else
      {
        if (report.keycode[1] != last_report.keycode[1])
        {
          if (report.keycode[1] != 0)
            onKeyDown(report.keycode[1]);
          else
            onKeyUp(last_report.keycode[1]);
        }
        else if (report.keycode[0] != last_report.keycode[0])
        {
          if (report.keycode[0] != 0)
            onKeyDown(report.keycode[0]);
          else
            onKeyUp(last_report.keycode[0]);
        }
      }
      if (last_report.modifier != report.modifier)
      {
        onModifierChange();
      }
    }

    void onModifierChange()
    {
      ctrlcode = 0;
      if (shift)
      {
        ctrlcode |= 0b001;
      }
      if (control)
      {
        ctrlcode |= 0b010;
      }
      if (left_win)
      { // Commodore
        ctrlcode |= 0b100;
      }
      if (left_alt)
      { // Command
        ctrlcode |= 0b10000000;
      }



      //ESP_LOGE(TAG, "ctrlcode=[0x%02x]", ctrlcode);
    }

    void onKeyDown(uint8_t keycode)
    {

      uint8_t code = scancodes[keycode];
      dc00 = getCode(code, true);
      dc01 = getCode(code, false);
      kbdcode = keycode;
      // if (ctrlcode & 128) {
      // // execute external command
      // // have to use a seperate task, because stack size of the BLE processing
      // // task is too small (and task size is not changeable under Arduino)
      // ESP_LOGI(TAG, "run external command...");
      // ExeExtCmdTask exeExtCmdTask(usbkbcopy,externalCmdscopy);
      // uint8_t type = exeExtCmdTask.exe();
      // // send notification
      //}

      //ESP_LOGE(TAG, "dc00=[0x%02x]", dc00);
      //ESP_LOGE(TAG, "dc01=[0x%02x]", dc01);
    }

    uint8_t getCode(uint8_t keycode, bool id)
    {
      if (id)
        return ~(1 << (keycode >> 3));
      else
        return ~(1 << (keycode & 7));
    }

    void onKeyUp(uint8_t keycode)
    {
      //ESP_LOGE(TAG, "KeyUp=[0x%02x]", keycode);
      dc00 = 0b11111111;
      dc01 = 0b11111111;
      kbdcode = 0;
    }

    void onKeyboardKey(uint8_t ascii, uint8_t keycode, uint8_t modifier) {
    };

  public:
    uint8_t dc01 = 0xff;
    uint8_t dc00 = 0xff;
    uint8_t ctrlcode = 0;
    uint8_t kbdcode = 0;

};

MyEspUsbHost usbHost; 

USBKB::USBKB() { 
  buffer = nullptr; 
}


void keyboardTask(void *pvParameters)
{
  while (true)
  {
    usbHost.task();
    delay(1);
  }
}

void USBKB::init(C64Emu *c64emu)
{
  if (buffer != nullptr)
  {
    // init method must be called only once
    return;
  }

  this->c64emu = c64emu;

  // init buffer
  buffer = new uint8_t[256];
  keypresseddowncnt = NUMOFCYCLES_KEYPRESSEDDOWN;
  keypresseddown = false;
  sentdc01 = 0xff;
  sentdc00 = 0xff;

  // init div
  virtjoystickvalue = 0xff;
  detectreleasekey = true;

  usbHost.begin();
  ESP_LOGE(TAG, "usbHost.begin()");
  usbHost.setHIDLocal(HID_LOCAL_Japan_Katakana);
  xTaskCreate(keyboardTask, "keyboardTask", 4096, NULL, 1, NULL);
}

void USBKB::handleKeyPress()
{
  if (usbHost.ctrlcode & 128 && usbHost.kbdcode != 0) {
    // execute external command
    // have to use a seperate task, because stack size of the BLE processing
    // task is too small (and task size is not changeable under Arduino)
    ESP_LOGI(TAG, "run external command...");
    ExternalCmds externalCmds;
    externalCmds.init(this->c64emu->ram, this->c64emu);
    uint8_t send[] = { usbHost.kbdcode };
    externalCmds.executeExternalCmd(send);
    
    // send notification
  }
  
}

uint8_t USBKB::getdc01(uint8_t querydc00, bool xchgports)
{
  uint8_t kbcode1;
  uint8_t kbcode2;
  shiftctrlcode = usbHost.ctrlcode;
  if (xchgports)
  {

    kbcode1 = usbHost.dc01;
    kbcode2 = usbHost.dc00;
  }
  else
  {
    kbcode1 = usbHost.dc00;
    kbcode2 = usbHost.dc01;
  }
  if (querydc00 == 0)
  {
    return kbcode2;
  }
  // special case "shift" + "commodore"
  if ((shiftctrlcode & 5) == 5)
  {
    if (querydc00 == kbcode1)
    {
      return kbcode2;
    }
    else
    {
      return 0xff;
    }
  }
  // key combined with a "special key" (shift, ctrl, commodore)?
  if ((~querydc00 & 2) && (shiftctrlcode & 1))
  { // *query* left shift key?
    if (kbcode1 == 0xfd)
    {
      // handle scan of key codes in the same "row"
      return kbcode2 & 0x7f;
    }
    else
    {
      return 0x7f;
    }
  }
  else if ((~querydc00 & 0x40) &&
           (shiftctrlcode & 1))
  { // *query* right shift key?
    if (kbcode1 == 0xbf)
    {
      // handle scan of key codes in the same "row"
      return kbcode2 & 0xef;
    }
    else
    {
      return 0xef;
    }
  }
  else if ((~querydc00 & 0x80) && (shiftctrlcode & 2))
  { // *query* ctrl key?
    if (kbcode1 == 0x7f)
    {
      // handle scan of key codes in the same "row"
      return kbcode2 & 0xfb;
    }
    else
    {
      return 0xfb;
    }
  }
  else if ((~querydc00 & 0x80) &&
           (shiftctrlcode & 4))
  { // *query* commodore key?
    if (kbcode1 == 0x7f)
    {
      // handle scan of key codes in the same "row"
      return kbcode2 & 0xdf;
    }
    else
    {
      return 0xdf;
    }
  }
  // query "main" key press
  if (querydc00 == kbcode1)
  {
    return kbcode2;
  }
  else
  {
    return 0xff;
  }
}

uint8_t USBKB::getKBJoyValue(bool port2) { return virtjoystickvalue; }

void USBKB::setKbcodes(uint8_t sentdc01, uint8_t sentdc00)
{
  this->sentdc01 = sentdc01;
  this->sentdc00 = sentdc00;
}
