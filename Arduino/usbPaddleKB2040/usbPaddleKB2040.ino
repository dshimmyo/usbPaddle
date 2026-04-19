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

  analogReadResolution(12);
}

void loop() {
  if (!usb_hid.ready()) return;

  // Read Paddle
  int rawValue = analogRead(POT_PIN);
  int8_t x_axis = map(rawValue, 0, 4095, -127, 127);

  // Read D-Pad and set Hat Switch
  uint8_t hat = GAMEPAD_HAT_CENTERED;
  bool u = !digitalRead(BTN_UP);
  bool d = !digitalRead(BTN_DOWN);
  bool l = !digitalRead(BTN_LEFT);
  bool r = !digitalRead(BTN_RIGHT);

  if (u && r) hat = GAMEPAD_HAT_UP_RIGHT;
  else if (u && l) hat = GAMEPAD_HAT_UP_LEFT;
  else if (d && r) hat = GAMEPAD_HAT_DOWN_RIGHT;
  else if (d && l) hat = GAMEPAD_HAT_DOWN_LEFT;
  else if (u) hat = GAMEPAD_HAT_UP;
  else if (d) hat = GAMEPAD_HAT_DOWN;
  else if (l) hat = GAMEPAD_HAT_LEFT;
  else if (r) hat = GAMEPAD_HAT_RIGHT;

  // Read Face Buttons
  uint32_t buttons = 0;
  if (!digitalRead(BTN_A)) buttons |= GAMEPAD_BUTTON_A;
  if (!digitalRead(BTN_B)) buttons |= GAMEPAD_BUTTON_B;

  hid_gamepad_report_t report = {
    .x = x_axis, .y = 0, .z = 0, .rz = 0, .rx = 0, .ry = 0,
    .hat = hat, .buttons = buttons
  };

  usb_hid.sendReport(0, &report, sizeof(report));
  delay(10);
}