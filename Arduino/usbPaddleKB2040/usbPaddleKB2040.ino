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

// Hardware Pin for the Rumble Motor (via transistor/MOSFET)
#define MOTOR_PIN 2 // Use any PWM-capable pin

// Global variables for rumble intensity
uint8_t left_motor_val = 0;
uint8_t right_motor_val = 0;

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
  pinMode(MOTOR_PIN, OUTPUT);

  analogReadResolution(12);

  // Set the callback for receiving data FROM the host (Mac/PC/Handheld)
  usb_hid.setReportCallback(NULL, hid_out_report_cb);
  analogWrite(MOTOR_PIN, 128);

}

// This function is called automatically whenever 
// RetroArch or the OS sends a rumble command.
void hid_out_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  // Standard Gamepad Output Report is usually 2 or 4 bytes
  // Format: [Strong Motor Intensity] [Weak Motor Intensity]
  if (report_type == HID_REPORT_TYPE_OUTPUT) {
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

  // 3a. rumble:

  uint8_t intensity = max(left_motor_val, right_motor_val);
  analogWrite(MOTOR_PIN, intensity);

  // 4. Clean Report
  hid_gamepad_report_t report;
  memset(&report, 0, sizeof(report));
  report.x       = x_axis;
  report.hat     = hat;
  report.buttons = buttons;

  usb_hid.sendReport(0, &report, sizeof(report));
  delay(10);
}