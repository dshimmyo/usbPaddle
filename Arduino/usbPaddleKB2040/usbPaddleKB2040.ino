#include "Adafruit_TinyUSB.h"

uint8_t const desc_hid_report[] = {
  // Collection 1: Standard Gamepad for Anbernic (ID 1)
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

  // Collection 2: Absolute Paddle for Steam Deck (ID 2)
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x37,        // Usage (Dial) 
  0xa1, 0x01,        // Collection (Application)
    0x85, 0x02,      //   REPORT ID (2)
    0x09, 0x30,      //   Usage (X)
    0x15, 0x81,      //   Logical Minimum (-127)
    0x25, 0x7f,      //   Logical Maximum (127)
    0x75, 0x08,      //   Report Size (8 bits)
    0x95, 0x01,      //   Report Count (1)
    0x81, 0x02,      //   Input (Data, Var, Abs)
  0xc0               // End Collection
};

Adafruit_USBD_HID usb_hid;

// Define Pins
#define POT_PIN A0
#define POT_PIN1 A1
#define POT_PIN2 A2
#define POT_PIN3 A3
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

// Hardware Pin for the Rumble Motor (via transistor/MOSFET)
// #define MOTOR_PIN 2 // Use any PWM-capable pin

// Global variables for rumble intensity
// uint8_t left_motor_val = 0;
// uint8_t right_motor_val = 0;

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
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_X, INPUT_PULLUP);
  pinMode(BTN_Y, INPUT_PULLUP);

  // pinMode(MOTOR_PIN, OUTPUT);

  analogReadResolution(12);

  // Set the callback for receiving data FROM the host (Mac/PC/Handheld)
  usb_hid.setReportCallback(NULL, hid_out_report_cb);
  // analogWrite(MOTOR_PIN, 255);
  // delay(50);                   // Very short duration
  // analogWrite(MOTOR_PIN, 0);
}

// This function is called automatically whenever 
// RetroArch or the OS sends a rumble command.
void hid_out_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  // Turn on the LED so we know the computer is actually sending data
  digitalWrite(LED_BUILTIN, HIGH); 

  // if (report_type == HID_REPORT_TYPE_OUTPUT) {
  //   if (bufsize >= 2) {
  //     left_motor_val = buffer[0];
  //     right_motor_val = buffer[1];
  //   }
  // }
}

typedef struct TU_ATTR_PACKED {
  int8_t  x;         // Byte 0
  int8_t  y;         // Byte 1
  uint32_t buttons;  // Bytes 2, 3, 4, 5
  uint8_t hat_byte; // One full byte to rule them all

} dks_report_t;

void loop() {
  if (!usb_hid.ready()) return;

  // 1. Paddle (X-Axis)
  int rawValue = analogRead(POT_PIN);
  int8_t x_axis = map(rawValue, 0, 4095, -127, 127);

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
  
  // 3a. rumble:

  // uint8_t intensity = max(left_motor_val, right_motor_val);
  // analogWrite(MOTOR_PIN, intensity);

  // Clean Report
  dks_report_t report;//new struct

  memset(&report, 0, sizeof(report));
  report.x       = x_axis; //anbernic
  report.y       = 0;       // 
  report.buttons = buttons;
  report.hat_byte = hat;
  //report.hat_byte = (hat & 0x0F);
  //report.hat_byte = (hat == PADDLE_DPAD_HAT_CENTERED) ? 0x08 : (hat & 0x0F);
  usb_hid.sendReport(1, &report, sizeof(report));// Explicitly ID 1

  // New Steam Deck Report (Report ID 2)
  // We send the same x_axis data, but under the "Absolute Dial" flag
  int8_t paddle_data = x_axis; 
  usb_hid.sendReport(2, &paddle_data, sizeof(paddle_data));

  delay(10);
}