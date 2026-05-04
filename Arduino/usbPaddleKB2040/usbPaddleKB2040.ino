#include "Adafruit_TinyUSB.h"

uint8_t const desc_normal[] = { 
  // Collection 1: Standard Gamepad (Xbox/XInput Baseline)
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x05,        // Usage (Game Pad)
  0xa1, 0x01,        // Collection (Application)
    0x85, 0x01,      //   REPORT ID (1) - MUST BE EXPLICIT
    0x09, 0x01,      //   Usage (Pointer)
    0xa1, 0x00,      //   Collection (Physical)
      0x05, 0x01,    //     Usage Page (Generic Desktop)
      0x09, 0x30,    //     Usage (X)
      0x09, 0x31,    //     Usage (Y)
      0x15, 0x81,    //     Logical Minimum (-127)
      0x25, 0x7f,    //     Logical Maximum (127)
      0x75, 0x08,    //     Report Size (8 bits)
      0x95, 0x02,    //     Report Count (2)
      0x81, 0x02,    //     Input (Data, Var, Abs)
    0xc0,            //   End Physical Collection
    0x05, 0x09,      //   Usage Page (Button)
    0x19, 0x01,      //   Usage Minimum (1)
    0x29, 0x20,      //   Usage Maximum (32)
    0x15, 0x00,      //   Logical Minimum (0)
    0x25, 0x01,      //   Logical Maximum (1)
    0x75, 0x01,      //   Report Size (1 bit)
    0x95, 0x20,      //   Report Count (32)
    0x81, 0x02,      //   Input (Data, Var, Abs)
    0x05, 0x01,      //   Usage Page (Generic Desktop)
    0x09, 0x39,      //   Usage (Hat Switch)
    0x15, 0x00,      //   Logical Minimum (0)
    0x25, 0x07,      //   Logical Maximum (7)
    0x35, 0x00,      //   Physical Minimum (0)
    0x46, 0x3b, 0x01,//   Physical Maximum (315)
    0x65, 0x14,      //   Unit (20)
    0x75, 0x04,      //   Report Size (4 bits)
    0x95, 0x01,      //   Report Count (1)
    0x81, 0x42,      //   Input (Data, Var, Abs)
    0x75, 0x04,      //   Report Size (4 bits) - Padding
    0x95, 0x01,      //   Report Count (1)
    0x81, 0x01,      //   Input (Const)
  0xc0,              // End Collection (Application)

};

Adafruit_USBD_HID usb_hid;

// Define Pins
#define POT_PIN 26 //A0
#define POT_PIN1 27 //A1
#define POT_PIN2 28 //A2
#define POT_PIN3 29 //A3
#define BTN_UP 3
#define BTN_DOWN 4
#define BTN_LEFT 5
#define BTN_RIGHT 6
#define BTN_A 2 //down//7 (right)
#define BTN_B 7 //right//8 (left)
#define BTN_SELECT 9
#define BTN_START 10// underside of RP2040
#define BTN_MENU 0 
#define BTN_X 8//left //up 1
#define BTN_Y 1//up //down 2

#define PADDLE_DPAD_HAT_UP        0
#define PADDLE_DPAD_HAT_RIGHT     2
#define PADDLE_DPAD_HAT_DOWN      4
#define PADDLE_DPAD_HAT_LEFT      6
#define PADDLE_DPAD_HAT_CENTERED  8

typedef struct TU_ATTR_PACKED {
  int8_t  x;
  int8_t  y;
  uint32_t buttons;
  uint8_t hat_byte;
} dks_report_t;


void setup() {

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_X, INPUT_PULLUP);
  pinMode(BTN_Y, INPUT_PULLUP);
  pinMode(POT_PIN, INPUT_PULLDOWN); // (ADC)
  pinMode(POT_PIN1, INPUT_PULLDOWN); // (ADC)
  pinMode(POT_PIN2, INPUT_PULLDOWN); // (ADC)
  pinMode(POT_PIN3, INPUT_PULLDOWN); // (ADC)

  delay(50); // Debounce

  TinyUSBDevice.setID(0x239A, 0x8108);
  usb_hid.setReportDescriptor(desc_normal, sizeof(desc_normal));
  
  TinyUSBDevice.setManufacturerDescriptor("OSHP");
  TinyUSBDevice.setProductDescriptor("HAQ-Pad");

  usb_hid.setPollInterval(2);
  usb_hid.begin();
  analogReadResolution(12);
}

void loop() {
  if (!usb_hid.ready()) return;

  // 1. Paddle (X-Axis)
  int rawValue = analogRead(POT_PIN);

  int8_t x_axis = map(rawValue, 0, 4095, -127, 127);

  if (rawValue < 5) { //needs a pulldown
      x_axis = 0; //center when disconnected
  }

  // 2. D-Pad (The Hat)
  uint8_t hat = PADDLE_DPAD_HAT_CENTERED;
  if      (!digitalRead(BTN_UP))    hat = PADDLE_DPAD_HAT_UP;
  else if (!digitalRead(BTN_DOWN))  hat = PADDLE_DPAD_HAT_DOWN;
  else if (!digitalRead(BTN_LEFT))  hat = PADDLE_DPAD_HAT_LEFT;
  else if (!digitalRead(BTN_RIGHT)) hat = PADDLE_DPAD_HAT_RIGHT;

  // 3. Standard Xbox/XInput Mapping
  uint32_t buttons = 0;
  
  if (!digitalRead(BTN_A))      buttons |= (1 << 0);  // A (Bottom)
  if (!digitalRead(BTN_B))      buttons |= (1 << 1);  // B (Right)
  if (!digitalRead(BTN_X))      buttons |= (1 << 2);  // X (Left)
  if (!digitalRead(BTN_Y))      buttons |= (1 << 3);  // Y (Top)
  if (!digitalRead(BTN_SELECT)) buttons |= (1 << 6);  // Back/Select
  if (!digitalRead(BTN_START))  buttons |= (1 << 7);  // Start
  if (!digitalRead(BTN_MENU))   buttons |= (1 << 10); // Guide/Menu
  
  dks_report_t report;
  memset(&report, 0, sizeof(report));
  report.x       = x_axis;
  report.buttons = buttons;
  report.hat_byte = hat;
  usb_hid.sendReport(1, &report, sizeof(report));

  delay(10);
}