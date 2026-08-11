#include "Adafruit_TinyUSB.h"

// -----------------------------------------------------------------------------
// Composite USB HID Descriptor
// -----------------------------------------------------------------------------
// Report ID 1: Relative Mouse (For Spinner Mode)
// Report ID 2: Joystick/Gamepad (For Virtual Paddle Mode)
// -----------------------------------------------------------------------------
uint8_t const desc_composite[] = {
  // --- REPORT ID 1: Mouse (Spinner) ---
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

  // --- REPORT ID 2: Joystick (Virtual Paddle) ---
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,      //   Report ID (2)
    0x05, 0x01,      //   Usage Page (Generic Desktop)
    0x09, 0x30,      //   Usage (X) -> Absolute Position
    0x15, 0x81,      //   Logical Minimum (-127)
    0x25, 0x7F,      //   Logical Maximum (127)
    0x75, 0x08,      //   Report Size (8 bits)
    0x95, 0x01,      //   Report Count (1)
    0x81, 0x02,      //   Input (Data, Var, Abs)
    // Joystick Button 1
    0x05, 0x09,      //   Usage Page (Button)
    0x19, 0x01,      //   Usage Minimum (1)
    0x29, 0x01,      //   Usage Maximum (1)
    0x15, 0x00,      //   Logical Minimum (0)
    0x25, 0x01,      //   Logical Maximum (1)
    0x75, 0x01,      //   Report Size (1 bit)
    0x95, 0x01,      //   Report Count (1)
    0x81, 0x02,      //   Input (Data, Var, Abs)
    0x75, 0x07,      //   Report Size (7 bits) - Padding
    0x95, 0x01,      //   Report Count (1)
    0x81, 0x01,      //   Input (Const)
  0xC0               // End Application Collection
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

// Simple sensitivity multiplier for Virtual Paddle mode movement
const int SENSITIVITY_SCALER = 4;
const int TICKS_SCALER = 10;

// -----------------------------------------------------------------------------
// Interrupt Service Routine (ISR) for Rotary Encoder Decoding
// -----------------------------------------------------------------------------
void readEncoderISR() {
  uint8_t a = digitalRead(ENCODER_PIN_A);
  uint8_t b = digitalRead(ENCODER_PIN_B);
  uint8_t current_state = (a << 1) | b;

  // Quad decoding transition check
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
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

  // Initial encoder state
  last_encoder_state = (digitalRead(ENCODER_PIN_A) << 1) | digitalRead(ENCODER_PIN_B);

  // Attach interrupts to both quadrature pins for fast response
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), readEncoderISR, CHANGE);

  // Configure USB
  //TinyUSBDevice.setID(0x239A, 0x8109);
  TinyUSBDevice.setID(0x239A, 0x810A); // Changed from 0x8109 to 0x810A
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

  // Atomically read and reset accumulated ticks from ISR
  noInterrupts();
  int32_t ticks = encoder_ticks;
  encoder_ticks = 0;
  interrupts();

  bool is_paddle_mode = (digitalRead(MODE_SWITCH_PIN) == LOW);
  bool btn_pressed = (digitalRead(ENCODER_BTN) == LOW);

  if (is_paddle_mode) {
    // -------------------------------------------------------------------------
    // MODE 1: VIRTUAL PADDLE (Absolute Joystick)
    // -------------------------------------------------------------------------
    if (ticks != 0) {
      virtual_paddle_pos += (ticks * SENSITIVITY_SCALER);

      // Clamp strictly between -127 and +127 (no rollover)
      if (virtual_paddle_pos > 127)  virtual_paddle_pos = 127;
      if (virtual_paddle_pos < -127) virtual_paddle_pos = -127;
    }

    struct TU_ATTR_PACKED {
      int8_t x;
      uint8_t buttons;
    } paddle_report;

    paddle_report.x = (int8_t)virtual_paddle_pos;
    paddle_report.buttons = btn_pressed ? 0x01 : 0x00;

    // Send Report ID 2
    usb_hid.sendReport(2, &paddle_report, sizeof(paddle_report));

  } else {
    // -------------------------------------------------------------------------
    // MODE 2: SPINNER (Relative Mouse)
    // -------------------------------------------------------------------------
    int8_t mouse_dx = (int8_t)constrain(ticks * TICKS_SCALER, -127, 127);

    struct TU_ATTR_PACKED {
      int8_t dx;
      uint8_t buttons;
    } mouse_report;

    mouse_report.dx = mouse_dx;
    mouse_report.buttons = btn_pressed ? 0x01 : 0x00; // Left click on press

    // Send Report ID 1
    usb_hid.sendReport(1, &mouse_report, sizeof(mouse_report));
  }
}