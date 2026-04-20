#include "Adafruit_TinyUSB.h"

uint8_t const desc_hid_report[] = {
    0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01,
    0x05, 0x01, 0x09, 0xbb, 0x15, 0x00, 0x26, 0xff,
    0x00, 0x75, 0x08, 0x95, 0x02, 0x91, 0x02, 0x05,
    0x01, 0x09, 0x01, 0xa1, 0x00, 0x09, 0x30, 0x09,
    0x31, 0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95,
    0x02, 0x81, 0x02, 0xc0, 0x05, 0x09, 0x19, 0x01,
    0x29, 0x0a, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
    0x95, 0x0a, 0x81, 0x02, 0x75, 0x01, 0x95, 0x06,
    0x81, 0x03, 0xc0
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

// Hardware Pin for the Rumble Motor (via transistor/MOSFET)
#define MOTOR_PIN 2 // Use any PWM-capable pin

// Custom struct to match the manual XInput-style hex descriptor
typedef struct {
  int8_t  x;       // X Axis
  int8_t  y;       // Y Axis
  uint16_t buttons; // 10 buttons + 6 padded bits
} xinput_report_t;

// Global variables for rumble intensity
uint8_t left_motor_val = 0;
uint8_t right_motor_val = 0;

void setup() {
  // TinyUSBDevice.setManufacturerDescriptor("DKS Interactive LLC");
  // TinyUSBDevice.setProductDescriptor("DKS Paddle v0");
  // TinyUSBDevice.setID(0x239A, 0x8108); // Optional: Adafruit's VID/PID for KB2040
// Official Xbox 360 Controller for Windows IDs
  TinyUSBDevice.setID(0x045E, 0x028E); 
  TinyUSBDevice.setManufacturerDescriptor("Microsoft");
  TinyUSBDevice.setProductDescriptor("XBOX 360 For Windows");

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
  pinMode(MOTOR_PIN, OUTPUT);

  analogReadResolution(12);

  // Set the callback for receiving data FROM the host (Mac/PC/Handheld)
  usb_hid.setReportCallback(NULL, hid_out_report_cb);
  analogWrite(MOTOR_PIN, 128);

}

void hid_out_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  digitalWrite(LED_BUILTIN, HIGH); 

  if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == 1) {
    if (bufsize >= 2) {
      left_motor_val = buffer[0];
      right_motor_val = buffer[1];
    }
  }
}

void loop() {
  if (!usb_hid.ready()) return;

  // 1. Paddle (X-Axis)
  int rawValue = analogRead(POT_PIN);
  int8_t x_axis = map(rawValue, 0, 4095, -127, 127);

  // // 2. D-Pad (The Hat)
  // uint8_t hat = GAMEPAD_HAT_CENTERED;
  // if      (!digitalRead(BTN_UP))    hat = GAMEPAD_HAT_UP;
  // else if (!digitalRead(BTN_DOWN))  hat = GAMEPAD_HAT_DOWN;
  // else if (!digitalRead(BTN_LEFT))  hat = GAMEPAD_HAT_LEFT;
  // else if (!digitalRead(BTN_RIGHT)) hat = GAMEPAD_HAT_RIGHT;

  uint16_t buttons = 0;

  // D-Pad Mapping (Standard Xbox Bit Positions)
  if (!digitalRead(BTN_UP))    buttons |= (1 << 1);
  if (!digitalRead(BTN_DOWN))  buttons |= (1 << 0);
  if (!digitalRead(BTN_LEFT))  buttons |= (1 << 2);
  if (!digitalRead(BTN_RIGHT)) buttons |= (1 << 3);

  // Map your buttons to the 16-bit field
  if (!digitalRead(BTN_A))      buttons |= (1 << 0); 
  if (!digitalRead(BTN_B))      buttons |= (1 << 1);
  if (!digitalRead(BTN_SELECT)) buttons |= (1 << 8); 
  if (!digitalRead(BTN_START))  buttons |= (1 << 9);
  // (Add others as needed)

  xinput_report_t report;
  report.x = x_axis;
  report.y = 0;
  report.buttons = buttons;

  // Send on Report ID 1
  usb_hid.sendReport(1, &report, sizeof(report));

  // 3. The "Stella Match" Buttons
  // uint32_t buttons = 0;
  
  // // Use the indices that your RetroArch menu explicitly listed
  // if (!digitalRead(BTN_A))      buttons |= (1 << 4);  // Fire (4)
  // if (!digitalRead(BTN_B))      buttons |= (1 << 3);  // Trigger (3) - Usually 'Back' or 'A' in menu
  // if (!digitalRead(BTN_SELECT)) buttons |= (1 << 9);  // Select (9)
  // if (!digitalRead(BTN_START))  buttons |= (1 << 10); // Reset/Start (10)

  // 3a. rumble:

  uint8_t intensity = max(left_motor_val, right_motor_val);
  analogWrite(MOTOR_PIN, intensity);

  delay(10);
}