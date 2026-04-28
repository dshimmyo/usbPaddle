#include "Adafruit_TinyUSB.h"
#include <Adafruit_NeoPixel.h>

// NeoPixel setup for mode feedback
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

uint8_t const desc_normal[] = { 
  // Collection 1: Standard Gamepad for Anbernic (ID 1) //Composite HID (Absolute) profile settings
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

  // // Collection 2: Absolute Paddle for Steam Deck (ID 2) //EXPERIMENTAL
  // 0x05, 0x01,        // Usage Page (Generic Desktop)
  // 0x09, 0x37,        // Usage (Dial) 
  // 0xa1, 0x01,        // Collection (Application)
  //   0x85, 0x02,      //   REPORT ID (2)
  //   0x09, 0x30,      //   Usage (X)
  //   0x15, 0x81,      //   Logical Minimum (-127)
  //   0x25, 0x7f,      //   Logical Maximum (127)
  //   0x75, 0x08,      //   Report Size (8 bits)
  //   0x95, 0x01,      //   Report Count (1)
  //   0x81, 0x02,      //   Input (Data, Var, Abs)
  // 0xc0               // End Collection
};

// --- 2. XINPUT DESCRIPTOR (Xbox 360 Spoof) ---
uint8_t const desc_xinput[] = {
  0x05, 0x01, 0x09, 0x05, 0xa1, 0x01,
    0x09, 0x01, 0xa1, 0x00,
      0x09, 0x30, 0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x01, 0x81, 0x02,
    0xc0,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x0a, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0a, 0x81, 0x02,
    0x75, 0x06, 0x95, 0x01, 0x81, 0x01,
  0xc0
};

// --- 3. PS4 DESCRIPTOR (DualShock 4 Spoof) ---
uint8_t const desc_ps4[] = {
  0x05, 0x01, 0x09, 0x05, 0xa1, 0x01,
    0x09, 0x30, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75, 0x08, 0x95, 0x01, 0x81, 0x02,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x0e, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0e, 0x81, 0x02,
    0x75, 0x02, 0x95, 0x01, 0x81, 0x01,
  0xc0
};

// --- 4. ABSOLUTE MOUSE DESCRIPTOR ---
uint8_t const desc_mouse_abs[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x02,        // Usage (Mouse)
  0xA1, 0x01,        // Collection (Application)
    0x09, 0x01,      //   Usage (Pointer)
    0xA1, 0x00,      //   Collection (Physical)
      0x05, 0x09,    //     Usage Page (Button)
      0x19, 0x01,    //     Usage Minimum (1)
      0x29, 0x03,    //     Usage Maximum (3)
      0x15, 0x00,    //     Logical Minimum (0)
      0x25, 0x01,    //     Logical Maximum (1)
      0x95, 0x03,    //     Report Count (3)
      0x75, 0x01,    //     Report Size (1)
      0x81, 0x02,    //     Input (Data, Var, Abs)
      0x95, 0x01,    //     Report Count (1)
      0x75, 0x05,    //     Report Size (5)
      0x81, 0x03,    //     Input (Const, Var, Abs) - Padding
      0x05, 0x01,    //     Usage Page (Generic Desktop)
      0x09, 0x30,    //     Usage (X)
      0x09, 0x31,    //     Usage (Y)
      0x15, 0x00,    //     Logical Minimum (0)
      0x26, 0xFF, 0x7F,// Logical Maximum (32767) for high precision
      0x75, 0x10,    //     Report Size (16 bits)
      0x95, 0x02,    //     Report Count (2)
      0x81, 0x02,    //     Input (Data, Var, Abs)
    0xC0,
  0xC0
};

Adafruit_USBD_HID usb_hid;

// Define Pins
#define POT_PIN A0
#define POT_PIN1 A1 //saving for later
#define POT_PIN2 A2 //saving for later
#define POT_PIN3 A3 //saving for later
#define BTN_UP 3
#define BTN_DOWN 4
#define BTN_LEFT 5
#define BTN_RIGHT 6
#define BTN_A 7 //outside (right)
#define BTN_B 8 //inside (left)
#define BTN_SELECT 9
#define BTN_START 10
#define BTN_MENU 0 //move to 11 on stemmaQT of KB2040
#define BTN_X 1 //move to 12 on stemmaQT of KB2040
#define BTN_Y 2

#define PADDLE_DPAD_HAT_UP        0
#define PADDLE_DPAD_HAT_RIGHT     2
#define PADDLE_DPAD_HAT_DOWN      4
#define PADDLE_DPAD_HAT_LEFT      6
#define PADDLE_DPAD_HAT_CENTERED  8

enum { MODE_NORMAL, MODE_XINPUT, MODE_PS4, MODE_MOUSE };
int currentMode = MODE_NORMAL;

typedef struct TU_ATTR_PACKED {
  int8_t  x;
  int8_t  y;
  uint32_t buttons;
  uint8_t hat_byte;
} dks_report_t;


void setup() {
  pixel.begin();

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

  delay(50); // Debounce

  if (digitalRead(BTN_SELECT) == LOW) {
    currentMode = MODE_XINPUT;
    TinyUSBDevice.setID(0x045E, 0x028E); 
    usb_hid.setReportDescriptor(desc_xinput, sizeof(desc_xinput));
    pixel.setPixelColor(0, 0, 32, 0); // Green
  } else if (digitalRead(BTN_START) == LOW) {
    currentMode = MODE_PS4;
    TinyUSBDevice.setID(0x054C, 0x05C4); 
    usb_hid.setReportDescriptor(desc_ps4, sizeof(desc_ps4));
    pixel.setPixelColor(0, 0, 0, 32); // Blue
  } else if (digitalRead(BTN_X) == LOW) {
    currentMode = MODE_MOUSE;
    TinyUSBDevice.setID(0x046D, 0xC077); // Logitech Mouse
    usb_hid.setReportDescriptor(desc_mouse_abs, sizeof(desc_mouse_abs));
    pixel.setPixelColor(0, 32, 32, 0); // Yellow
  } else {
    currentMode = MODE_NORMAL;
    TinyUSBDevice.setID(0x239A, 0x8108);
    usb_hid.setReportDescriptor(desc_normal, sizeof(desc_normal));
    pixel.setPixelColor(0, 32, 0, 32); // Purple
  }
  pixel.show();

  TinyUSBDevice.setManufacturerDescriptor("DKS Interactive LLC");
  TinyUSBDevice.setProductDescriptor("DKS Paddle v0");

  usb_hid.setPollInterval(2);
  usb_hid.begin();
  analogReadResolution(12);
  usb_hid.setReportCallback(NULL, hid_out_report_cb);
}

void hid_out_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  digitalWrite(LED_BUILTIN, HIGH); 
}

void loop() {
  if (!usb_hid.ready()) return;

  // 1. Paddle (X-Axis)
  int rawValue = analogRead(POT_PIN);
if (currentMode == MODE_MOUSE) {
    // 1. Map to high-res absolute coordinates (0 to 32767)
    int32_t current_x = map(rawValue, 0, 4095, 0, 32767);
    
    uint8_t m_btns = 0;
    if (!digitalRead(BTN_A)) m_btns |= 0x01; // Left Click
    if (!digitalRead(BTN_B)) m_btns |= 0x02; // Right Click

    // State tracking for the silence filter
    static int32_t last_mouse_x = -1;
    static uint8_t last_mouse_btns = 0xFF;

    // 2. The Deadband: Only send if moved by > 50 units, or if a button was pressed/released
    if (abs(current_x - last_mouse_x) > 50 || m_btns != last_mouse_btns) {
      
      struct TU_ATTR_PACKED {
        uint8_t btns;
        int16_t x;
        int16_t y;
      } m_rep = { m_btns, (int16_t)current_x, 16384 }; // Y is locked to screen center

      usb_hid.sendReport(0, &m_rep, sizeof(m_rep));
      
      last_mouse_x = current_x;
      last_mouse_btns = m_btns;
    }
  } else {

    int8_t x_axis = map(rawValue, 0, 4095, -127, 127);
    uint8_t ps4_axis = map(rawValue, 0, 4095, 0, 255);

    // 2. D-Pad (The Hat)
    uint8_t hat = PADDLE_DPAD_HAT_CENTERED;
    if      (!digitalRead(BTN_UP))    hat = PADDLE_DPAD_HAT_UP;
    else if (!digitalRead(BTN_DOWN))  hat = PADDLE_DPAD_HAT_DOWN;
    else if (!digitalRead(BTN_LEFT))  hat = PADDLE_DPAD_HAT_LEFT;
    else if (!digitalRead(BTN_RIGHT)) hat = PADDLE_DPAD_HAT_RIGHT;

    // 3. Native Anbernic/RetroArch Mapping
    uint32_t buttons = 0;
    
    // Mapping your pins to your discovered indices
    if (!digitalRead(BTN_A))      buttons |= (1 << 4);  // Anbernic A Nintendo A
    if (!digitalRead(BTN_B))      buttons |= (1 << 3);  // Anbernic B Nintendo B
    if (!digitalRead(BTN_X))      buttons |= (1 << 6);  // Anbernic X
    if (!digitalRead(BTN_Y))      buttons |= (1 << 5);  // Anbernic Y
    if (!digitalRead(BTN_SELECT)) buttons |= (1 << 9);  // Anbernic Select
    if (!digitalRead(BTN_START))  buttons |= (1 << 10); // Anbernic Start
    if (!digitalRead(BTN_MENU))   buttons |= (1 << 11); // Anbernic Menu (Press)
    
  if (currentMode == MODE_XINPUT) {
      struct { int8_t x; uint16_t btns; } rep = { x_axis, (uint16_t)buttons };
      usb_hid.sendReport(0, &rep, sizeof(rep));
    } 
    else if (currentMode == MODE_PS4) {
      struct { uint8_t x; uint16_t btns; } rep = { ps4_axis, (uint16_t)buttons };
      usb_hid.sendReport(0, &rep, sizeof(rep));
    } 
    else {
      // Normal Mode: Send both Anbernic (ID 1) and Steam Deck (ID 2)
      dks_report_t report;
      memset(&report, 0, sizeof(report));
      report.x       = x_axis;
      report.buttons = buttons;
      report.hat_byte = hat;
      usb_hid.sendReport(1, &report, sizeof(report));

      // int8_t paddle_data = x_axis; 
      // usb_hid.sendReport(2, &paddle_data, sizeof(paddle_data));
    }
  }

  delay(10);
}