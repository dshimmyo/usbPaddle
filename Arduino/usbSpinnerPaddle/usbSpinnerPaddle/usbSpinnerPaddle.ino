#include "Adafruit_TinyUSB.h"
#include "hardware/watchdog.h" // Needed for RP2040 hardware reset

// -----------------------------------------------------------------------------
// USB HID Descriptors
// -----------------------------------------------------------------------------

// Standard HID Mouse Descriptor (No Report ID - Native Windows/Mac/Linux)
uint8_t const desc_mouse[] = {
  TUD_HID_REPORT_DESC_MOUSE()
};

// Standard HID Joystick Descriptor (Report ID 1)
uint8_t const desc_joystick[] = { 
  TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(1))
};

Adafruit_USBD_HID usb_hid;

// -----------------------------------------------------------------------------
// Pin Definitions
// -----------------------------------------------------------------------------
#define ENCODER_PIN_A   0  // EC16 Phase A
#define ENCODER_PIN_B   1  // EC16 Phase B
#define ACTION_BTN1     2  // External Fire 1 (GP2)
#define ACTION_BTN2     3  // External Fire 2 (GP3)

#define MODE_SWITCH_PIN 15 // Mode Toggle sampled ONCE at startup
#define TRIM_PIN        26 // Sensitivity Potentiometer (GP26 / A0)

#define MAX_TICKS_REFERENCE 600.0f
#define MIN_TICKS_REFERENCE 20.0f
#define ENCODER_TICKS_PER_ROTATION 20 .0f // 24.0f for EC16
#define SENSITIVITY_MULTIPLIER_MAX ((MAX_TICKS_REFERENCE / ENCODER_TICKS_PER_ROTATION) * 2.0f)
#define SENSITIVITY_MULTIPLIER_MIN (MIN_TICKS_REFERENCE / ENCODER_TICKS_PER_ROTATION)
#define PADDLE_SENSITIVITY_MULTIPLIER 0.83f

// -----------------------------------------------------------------------------
// Global Variables & State
// -----------------------------------------------------------------------------
volatile int32_t encoder_ticks = 0;
float virtual_paddle_pos = 0;
uint8_t last_encoder_state = 0;
bool is_paddle_mode = false; 
int last_mode_switch_state = HIGH; 

// 4-bit Quadrature Lookup Table
static const int8_t encoder_table[16] = {
     0,  1, -1,  0,
    -1,  0,  0,  1,
     1,  0,  0, -1,
     0, -1,  1,  0
};

// -----------------------------------------------------------------------------
// Interrupt Service Routine (ISR)
// -----------------------------------------------------------------------------
void readEncoderISR() {
  uint8_t a = digitalRead(ENCODER_PIN_A);
  uint8_t b = digitalRead(ENCODER_PIN_B);

  last_encoder_state = ((last_encoder_state << 2) | (a << 1) | b) & 0x0F;

  int8_t delta = encoder_table[last_encoder_state];
  if (delta != 0) {
    encoder_ticks += delta;
  }
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ACTION_BTN1, INPUT_PULLUP);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(ACTION_BTN2, INPUT_PULLUP);
  analogReadResolution(12); // RP2040 12-bit ADC
  delay(50); 

  last_mode_switch_state = digitalRead(MODE_SWITCH_PIN);
  is_paddle_mode = (last_mode_switch_state == LOW);

  last_encoder_state = (digitalRead(ENCODER_PIN_A) << 1) | digitalRead(ENCODER_PIN_B);

  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), readEncoderISR, CHANGE);

  TinyUSBDevice.setManufacturerDescriptor("Open-Source");

  if (is_paddle_mode) {
    TinyUSBDevice.setID(0x239A, 0x8110);
    TinyUSBDevice.setProductDescriptor("Virtual Paddle Controller");
    usb_hid.setPollInterval(2);
    usb_hid.setReportDescriptor(desc_joystick, sizeof(desc_joystick));
    usb_hid.begin();
  } else {
    TinyUSBDevice.setID(0x239A, 0x8111);
    TinyUSBDevice.setProductDescriptor("Spinner Controller");
    usb_hid.setPollInterval(2);
    usb_hid.setBootProtocol(HID_ITF_PROTOCOL_MOUSE);
    usb_hid.setReportDescriptor(desc_mouse, sizeof(desc_mouse));
    usb_hid.begin();
  }
}

// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------
void loop() {
  int current_switch_state = digitalRead(MODE_SWITCH_PIN);
  if (current_switch_state != last_mode_switch_state) {
    delay(20); 
    if (digitalRead(MODE_SWITCH_PIN) == current_switch_state) {
      watchdog_reboot(0, 0, 0); 
    }
  }

  if (!usb_hid.ready()) return;

  int raw_trim = analogRead(TRIM_PIN);
  float pMult = is_paddle_mode ? PADDLE_SENSITIVITY_MULTIPLIER : 1.0f;
  float max_mult = SENSITIVITY_MULTIPLIER_MAX * pMult;
  float min_mult = SENSITIVITY_MULTIPLIER_MIN;

  float norm_trim = raw_trim / 4095.0f;
  float sensitivity = min_mult * powf(max_mult / min_mult, norm_trim);

  noInterrupts();
  int32_t ticks = encoder_ticks;
  encoder_ticks = 0;
  interrupts();

  bool fire1_pressed = (digitalRead(ACTION_BTN1) == LOW);
  bool fire2_pressed = (digitalRead(ACTION_BTN2) == LOW);

  if (is_paddle_mode) {
    if (ticks != 0) {
      virtual_paddle_pos += ((float)ticks * sensitivity);
      if (virtual_paddle_pos > 127.0f)  virtual_paddle_pos = 127.0f;
      if (virtual_paddle_pos < -127.0f) virtual_paddle_pos = -127.0f;
    }

    uint32_t buttons = 0;
    if (fire1_pressed) buttons |= (1 << 0); // Button 1
    if (fire2_pressed) buttons |= (1 << 1); // Button 2

    hid_gamepad_report_t report = {
      .x       = (int8_t)virtual_paddle_pos,
      .y       = 0,
      .z       = 0,
      .rz      = 0,
      .rx      = 0,
      .ry      = 0,
      .hat     = 0,
      .buttons = buttons
    };

    usb_hid.sendReport(1, &report, sizeof(report));

  } else {
    static float mouse_x_accumulator = 0.0f;

    mouse_x_accumulator += ((float)ticks * sensitivity);
    int8_t mouse_dx = (int8_t)mouse_x_accumulator;
    mouse_x_accumulator -= (float)mouse_dx;

    mouse_dx = (int8_t)constrain(mouse_dx, -127, 127);

    uint8_t buttons = 0;
    if (fire1_pressed) buttons |= MOUSE_BUTTON_LEFT;
    if (fire2_pressed) buttons |= MOUSE_BUTTON_MIDDLE;

    usb_hid.mouseReport(0, buttons, mouse_dx, 0, 0, 0);
  }
}