#include "Adafruit_TinyUSB.h"

// -----------------------------------------------------------------------------
// Composite USB HID Descriptor
// -----------------------------------------------------------------------------
// Report ID 1: Relative Mouse (For Spinner Mode)
// Report ID 2: Full Joystick Baseline (From your working Paddle sketch)
// -----------------------------------------------------------------------------
uint8_t const desc_composite[] = { 
  // --- REPORT ID 1: Relative Mouse (Spinner Mode) ---
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x02,        // Usage (Mouse)
  0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,      //   Report ID (1)
    0x09, 0x01,      //   Usage (Pointer)
    0xA1, 0x00,      //   Collection (Physical)
      0x05, 0x01,    //     Usage Page (Generic Desktop)
      0x09, 0x30,    //     Usage (X) -> Relative Delta
      0x15, 0x81,    //     Logical Minimum (-127)
      0x25, 0x7F,    //     Logical Maximum (127)
      0x75, 0x08,    //     Report Size (8 bits)
      0x95, 0x01,    //     Report Count (1)
      0x81, 0x06,    //     Input (Data, Var, Rel)
    0xC0,            //   End Physical Collection
    // Mouse Buttons (Left, Right, Middle)
    0x05, 0x09,      //   Usage Page (Button)
    0x19, 0x01,      //   Usage Minimum (1)
    0x29, 0x03,      //   Usage Maximum (3)
    0x15, 0x00,      //   Logical Minimum (0)
    0x25, 0x01,      //   Logical Maximum (1)
    0x75, 0x01,      //   Report Size (1 bit)
    0x95, 0x03,      //   Report Count (3)
    0x81, 0x02,      //   Input (Data, Var, Abs)
    0x75, 0x05,      //   Report Size (5 bits) - Padding
    0x95, 0x01,      //   Report Count (1)
    0x81, 0x01,      //   Input (Const)
  0xC0,              // End Application Collection

  // --- REPORT ID 2: Joystick Baseline (Your Working Paddle Layout) ---
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,      //   REPORT ID (2)
    0x09, 0x01,      //   Usage (Pointer)
    0xA1, 0x00,      //   Collection (Physical)
      0x05, 0x01,    //     Usage Page (Generic Desktop)
      0x09, 0x30,    //     Usage (X)
      0x09, 0x31,    //     Usage (Y)
      0x09, 0x32,    //     Usage (Z)
      0x09, 0x33,    //     Usage (Rx)
      0x15, 0x81,    //     Logical Minimum (-127)
      0x25, 0x7f,    //     Logical Maximum (127)
      0x75, 0x08,    //     Report Size (8 bits)
      0x95, 0x04,    //     Report Count (4)
      0x81, 0x02,    //     Input (Data, Var, Abs)
    0xC0,            //   End Physical Collection
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
  0xC0,              // End Application Collection
};

Adafruit_USBD_HID usb_hid;

// -----------------------------------------------------------------------------
// Pin Definitions
// -----------------------------------------------------------------------------
#define ENCODER_PIN_A   0  // EC11 Phase A
#define ENCODER_PIN_B   1  // EC11 Phase B
#define ENCODER_BTN     2  // EC11 Push Switch
#define MODE_SWITCH_PIN 15 // Mode Toggle (HIGH = Spinner, LOW = Paddle)

// -----------------------------------------------------------------------------
// Global Variables & State
// -----------------------------------------------------------------------------
volatile int32_t encoder_ticks = 0;
int16_t virtual_paddle_pos = 0; // Absolute paddle position (-127 to 127)
uint8_t last_encoder_state = 0;
bool last_mode_state = false;

// -----------------------------------------------------------------------------
// Data Structures
// -----------------------------------------------------------------------------
typedef struct TU_ATTR_PACKED {
  int8_t  p1;         // Paddle 1 (X)
  int8_t  p2;         // Paddle 2 (Y)
  int8_t  p3;         // Paddle 3 (Z)
  int8_t  p4;         // Paddle 4 (Rx)
  uint32_t buttons;
  uint8_t hat_byte;
} dks_report_t;

typedef struct TU_ATTR_PACKED {
  int8_t dx;
  uint8_t buttons;
} mouse_report_t;

// -----------------------------------------------------------------------------
// Interrupt Service Routine (ISR) for Rotary Encoder Decoding
// -----------------------------------------------------------------------------
void readEncoderISR() {
  uint8_t a = digitalRead(ENCODER_PIN_A);
  uint8_t b = digitalRead(ENCODER_PIN_B);
  uint8_t current_state = (a << 1) | b;

  if (last_encoder_state != current_state) {
    if ((last_encoder_state == 0b00 && current_state == 0b01) ||
        (last_encoder_state == 0b01 && current_state == 0b11) ||
        (last_encoder_state == 0b11 && current_state == 0b10) ||
        (last_encoder_state == 0b10 && current_state == 0b00)) {
      encoder_ticks++;
    } 
    else if ((last_encoder_state == 0b00 && current_state == 0b10) ||
             (last_encoder_state == 0b10 && current_state == 0b11) ||
             (last_encoder_state == 0b11 && current_state == 0b01) ||
             (last_encoder_state == 0b01 && current_state == 0b00)) {
      encoder_ticks--;
    }
    last_encoder_state = current_state;
  }
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

  last_encoder_state = (digitalRead(ENCODER_PIN_A) << 1) | digitalRead(ENCODER_PIN_B);

  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), readEncoderISR, CHANGE);

  delay(50); // Debounce

  // Set PID to 0x810B to force host OS to flush descriptor cache
  TinyUSBDevice.setID(0x239A, 0x810B);
  usb_hid.setReportDescriptor(desc_composite, sizeof(desc_composite));
  
  TinyUSBDevice.setManufacturerDescriptor("Open-Source");
  TinyUSBDevice.setProductDescriptor("Spinner Paddle Hybrid");

  usb_hid.setPollInterval(2);
  usb_hid.begin();
}

// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------
void loop() {
  if (!usb_hid.ready()) return;

  bool is_paddle_mode = (digitalRead(MODE_SWITCH_PIN) == LOW);

  // Print state change to Serial Monitor
  if (is_paddle_mode != last_mode_state) {
    last_mode_state = is_paddle_mode;
    if (is_paddle_mode) {
      Serial.println(">> MODE CHANGED: Virtual Paddle Mode (Joystick Report ID 2)");
    } else {
      Serial.println(">> MODE CHANGED: Spinner Mode (Mouse Report ID 1)");
    }
  }

  // Atomically grab and reset accumulated encoder ticks
  noInterrupts();
  int32_t ticks = encoder_ticks;
  encoder_ticks = 0;
  interrupts();

  bool btn_pressed = (digitalRead(ENCODER_BTN) == LOW);

  if (is_paddle_mode) {
    // -------------------------------------------------------------------------
    // MODE 1: VIRTUAL PADDLE (Absolute Joystick on Report ID 2)
    // -------------------------------------------------------------------------
    if (ticks != 0) {
      virtual_paddle_pos += (ticks * 10); // Scaled up 10x for responsiveness

      if (virtual_paddle_pos > 127)  virtual_paddle_pos = 127;
      if (virtual_paddle_pos < -127) virtual_paddle_pos = -127;
    }

    dks_report_t report;
    memset(&report, 0, sizeof(report));
    report.p1       = (int8_t)virtual_paddle_pos; // Encoder maps to Paddle 1 (Axis 0)
    report.p2       = 0;
    report.p3       = 0;
    report.p4       = 0;
    report.buttons  = btn_pressed ? (1 << 0) : 0; // Encoder press = Button 1
    report.hat_byte = 8;                          // Centered D-Pad

    usb_hid.sendReport(2, &report, sizeof(report));

  } else {
    // -------------------------------------------------------------------------
    // MODE 2: SPINNER (Relative Mouse on Report ID 1)
    // -------------------------------------------------------------------------
    int8_t mouse_dx = (int8_t)constrain(ticks * 10, -127, 127);

    mouse_report_t report;
    report.dx      = mouse_dx;
    report.buttons = btn_pressed ? 0x01 : 0x00; // Encoder press = Left Click

    usb_hid.sendReport(1, &report, sizeof(report));
  }
}