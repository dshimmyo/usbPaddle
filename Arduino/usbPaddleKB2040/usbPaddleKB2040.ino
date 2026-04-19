#include "Adafruit_TinyUSB.h"

uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_GAMEPAD()
};

Adafruit_USBD_HID usb_hid;

// Define Pins
#define POT_PIN A0
#define BTN_UP 3
#define BTN_DOWN 4
#define BTN_LEFT 5
#define BTN_RIGHT 6
#define BTN_A 7
#define BTN_B 8
#define BTN_SELECT 9
#define BTN_START 10

void setup() {
  TinyUSBDevice.setManufacturerDescriptor("DKS Interactive LLC");
  TinyUSBDevice.setProductDescriptor("DKS Paddle v0");
  TinyUSBDevice.setID(0x239A, 0x8108); // Optional: Adafruit's VID/PID for KB2040

  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);

  analogReadResolution(12);
}

void loop() {
  if (!usb_hid.ready()) return;

  // 1. Paddle (X-Axis)
  int rawValue = analogRead(POT_PIN);
  int8_t x_axis = map(rawValue, 0, 4095, -127, 127);

  // 2. D-Pad (The Hat)
  uint8_t hat = GAMEPAD_HAT_CENTERED;
  if      (!digitalRead(BTN_UP))    hat = GAMEPAD_HAT_UP;
  else if (!digitalRead(BTN_DOWN))  hat = GAMEPAD_HAT_DOWN;
  else if (!digitalRead(BTN_LEFT))  hat = GAMEPAD_HAT_LEFT;
  else if (!digitalRead(BTN_RIGHT)) hat = GAMEPAD_HAT_RIGHT;

  // 3. The "Stella Match" Buttons
  uint32_t buttons = 0;
  
  // Use the indices that your RetroArch menu explicitly listed
  if (!digitalRead(BTN_A))      buttons |= (1 << 4);  // Fire (4)
  if (!digitalRead(BTN_B))      buttons |= (1 << 3);  // Trigger (3) - Usually 'Back' or 'A' in menu
  if (!digitalRead(BTN_SELECT)) buttons |= (1 << 9);  // Select (9)
  if (!digitalRead(BTN_START))  buttons |= (1 << 10); // Reset/Start (10)

  // 4. Clean Report
  hid_gamepad_report_t report;
  memset(&report, 0, sizeof(report));
  report.x       = x_axis;
  report.hat     = hat;
  report.buttons = buttons;

  usb_hid.sendReport(0, &report, sizeof(report));
  delay(10);
}