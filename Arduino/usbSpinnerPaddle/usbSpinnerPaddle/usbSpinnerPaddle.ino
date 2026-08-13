#include "Adafruit_TinyUSB.h"
#include "hardware/watchdog.h" // Needed for RP2040 hardware reset

// -----------------------------------------------------------------------------
// USB HID Descriptors
// -----------------------------------------------------------------------------

// Mouse Descriptor (Spinner Mode)
uint8_t const desc_mouse[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x02,        // Usage (Mouse)
  0xA1, 0x01,        // Collection (Application)
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
  0xC0               // End Application Collection
};

// Joystick Descriptor (Virtual Paddle Mode)
uint8_t const desc_joystick[] = { 
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x04,        // Usage (Joystick)
  0xa1, 0x01,        // Collection (Application)
    0x09, 0x01,      //   Usage (Pointer)
    0xa1, 0x00,      //   Collection (Physical)
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
  0xc0               // End Collection (Application)
};

Adafruit_USBD_HID usb_hid;

// -----------------------------------------------------------------------------
// Pin Definitions
// -----------------------------------------------------------------------------
#define ENCODER_PIN_A   0  // EC11 Phase A
#define ENCODER_PIN_B   1  // EC11 Phase B
#define ACTION_BTN1     2  // Ext button gp2
#define ACTION_BTN2     3 // External Fire Button (GP3 to GND)

#define MODE_SWITCH_PIN 15 // Mode Toggle sampled ONCE at startup (and monitored for reboot)
#define TRIM_PIN 26        // Connect wiper to GP26 (A0)

#define MAX_TICKS_REFERENCE 600.0f
#define MIN_TICKS_REFERENCE 20.0f
#define ENCODER_TICKS_PER_ROTATION 20.0f
#define SENSITIVITY_MULTIPLIER_MAX ((MAX_TICKS_REFERENCE / ENCODER_TICKS_PER_ROTATION) * 2.0f)
#define SENSITIVITY_MULTIPLIER_MIN (MIN_TICKS_REFERENCE / ENCODER_TICKS_PER_ROTATION)
#define PADDLE_SENSITIVITY_MULTIPLIER 0.75f

// -----------------------------------------------------------------------------
// Global Variables & State
// -----------------------------------------------------------------------------
volatile int32_t encoder_ticks = 0;
float virtual_paddle_pos = 0;
uint8_t last_encoder_state = 0;
bool is_paddle_mode = false; 
int last_mode_switch_state = HIGH; // Tracks physical pin state for reboot trigger

// 4-bit Quadrature Lookup Table: Index = [Old_A, Old_B, New_A, New_B]
static const int8_t encoder_table[16] = {
     0,  1, -1,  0,
    -1,  0,  0,  1,
     1,  0,  0, -1,
     0, -1,  1,  0
};

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
  Serial.begin(115200);

  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ACTION_BTN1, INPUT_PULLUP);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(ACTION_BTN2, INPUT_PULLUP);  // Add pull-up for GP3
  analogReadResolution(12); // RP2040 uses 12-bit ADC (0 to 4095)
  delay(50); // Debounce hardware boot

  // Read initial state of the switch for mode decision & transition tracking
  last_mode_switch_state = digitalRead(MODE_SWITCH_PIN);
  is_paddle_mode = (last_mode_switch_state == LOW);

  // Seed the initial state variable with current pin values
  last_encoder_state = (digitalRead(ENCODER_PIN_A) << 1) | digitalRead(ENCODER_PIN_B);

  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), readEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), readEncoderISR, CHANGE);

  TinyUSBDevice.setManufacturerDescriptor("Open-Source");

  if (is_paddle_mode) {
    // Boot as dedicated Joystick Gamepad
    TinyUSBDevice.setID(0x239A, 0x810C);
    TinyUSBDevice.setProductDescriptor("Virtual Paddle Controller");
    usb_hid.setReportDescriptor(desc_joystick, sizeof(desc_joystick));
    Serial.println(">> BOOT MODE: Virtual Paddle (Joystick PID 0x810C)");
  } else {
    // Boot as dedicated Spinner Mouse
    TinyUSBDevice.setID(0x239A, 0x810B);
    TinyUSBDevice.setProductDescriptor("Spinner Controller");
    usb_hid.setReportDescriptor(desc_mouse, sizeof(desc_mouse));
    Serial.println(">> BOOT MODE: Spinner (Mouse PID 0x810B)");
  }

  usb_hid.setPollInterval(2);
  usb_hid.begin();
}

// -----------------------------------------------------------------------------
// Main Loop
// -----------------------------------------------------------------------------
void loop() {
  // 1. Check for Mode Switch State Transition -> Trigger Reboot
  int current_switch_state = digitalRead(MODE_SWITCH_PIN);
  if (current_switch_state != last_mode_switch_state) {
    delay(20); // Filter contact bounce
    if (digitalRead(MODE_SWITCH_PIN) == current_switch_state) {
      // Force hardware reset on RP2040 to re-enumerate USB HID descriptor
      watchdog_reboot(0, 0, 0); 
    }
  }

  if (!usb_hid.ready()) return;

  // Map ADC (0 - 4095) to exponential sensitivity float range
  int raw_trim = analogRead(TRIM_PIN);
  float pMult = is_paddle_mode ? PADDLE_SENSITIVITY_MULTIPLIER : 1.0f;
  float max_mult = SENSITIVITY_MULTIPLIER_MAX * pMult;
  float min_mult = SENSITIVITY_MULTIPLIER_MIN;

  // Normalize ADC to 0.0 - 1.0 range
  float norm_trim = raw_trim / 4095.0f;

  // Exponential scaling calculation
  float sensitivity = min_mult * powf(max_mult / min_mult, norm_trim);

  // Atomically grab ticks
  noInterrupts();
  int32_t ticks = encoder_ticks;
  encoder_ticks = 0;
  interrupts();

// Read both physical buttons (Active LOW)
  bool fire1_pressed = (digitalRead(ACTION_BTN1) == LOW);
  bool fire2_pressed = (digitalRead(ACTION_BTN2) == LOW);

  if (is_paddle_mode) {
    // -------------------------------------------------------------------------
    // VIRTUAL PADDLE MODE
    // -------------------------------------------------------------------------
    if (ticks != 0) {
      virtual_paddle_pos += ((float)ticks * sensitivity);
      if (virtual_paddle_pos > 127.0f)  virtual_paddle_pos = 127.0f;
      if (virtual_paddle_pos < -127.0f) virtual_paddle_pos = -127.0f;
    }

  // 3. Standard Xbox/XInput Mapping
  uint32_t buttons = 0;
  
  if (fire1_pressed)      buttons |= (1 << 0);  // A (Bottom)
  if (fire2_pressed)      buttons |= (1 << 1);  // B (Right)
  // if (!digitalRead(BTN_X))      buttons |= (1 << 2);  // X (Left)
  // if (!digitalRead(BTN_Y))      buttons |= (1 << 3);  // Y (Top)
  // if (!digitalRead(BTN_L1))     buttons |= (1 << 4);  // Left Bumper
  // if (!digitalRead(BTN_R1))     buttons |= (1 << 5);  // Right Bumper
  // if (!digitalRead(BTN_SELECT)) buttons |= (1 << 6);  // Select/Back
  // if (!digitalRead(BTN_START))  buttons |= (1 << 7);  // Start
  // if (!digitalRead(BTN_L2))     buttons |= (1 << 8);  // Left Trigger
  // if (!digitalRead(BTN_R2))     buttons |= (1 << 9);  // Right Trigger
  // if (!digitalRead(BTN_MENU))   buttons |= (1 << 10); // Guide/Menu

    dks_report_t report;
    memset(&report, 0, sizeof(report));
    report.p1       = (int8_t)virtual_paddle_pos;
    report.p2       = 0;
    report.p3       = 0;
    report.p4       = 0;
    report.buttons  = buttons;
    report.hat_byte = 8; // Centered

    usb_hid.sendReport(0, &report, sizeof(report));

  } else {
    // -------------------------------------------------------------------------
    // SPINNER MODE
    // -------------------------------------------------------------------------
    static float mouse_x_accumulator = 0.0f;

    // Add new movement to accumulator
    mouse_x_accumulator += ((float)ticks * sensitivity);

    // Extract whole integer mouse steps to send
    int8_t mouse_dx = (int8_t)mouse_x_accumulator;

    // Keep remainder in the accumulator for the next frame
    mouse_x_accumulator -= (float)mouse_dx;

    // Constrain for HID report limits (-127 to 127)
    mouse_dx = (int8_t)constrain(mouse_dx, -127, 127);

    uint32_t buttons = 0;
    if (fire1_pressed)      buttons |= (1 << 0);  // Left Mouse
    if (fire2_pressed)      buttons |= (1 << 2);  // Middle Mouse (based on Tempest button defaults)
    mouse_report_t report;
    report.dx      = mouse_dx;
    report.buttons = buttons;//fire1_pressed ? 0x01 : 0x00; // Left Mouse Click
    usb_hid.sendReport(0, &report, sizeof(report));
  }
}