// This sketch generates the classic smooth RGB "rainbow wave"
// effect using sine wave mathematical functions on an ESP32's onboard
// RGB LED, commonly found on boards like the Cheap Yellow Display (CYD).

#include <cmath> // Required for sin() function
#include <Arduino.h>

// --- Pin Definitions for ESP32 Cheap Yellow Display (CYD) RGB LED ---
// NOTE: These are typically ACTIVE-LOW, meaning 0 is ON (full brightness) and 255 is OFF.
#define LED_R_PIN 2
#define LED_G_PIN 4
#define LED_B_PIN 16

// --- LEDC Setup Constants ---
const int freq = 5000;      // PWM frequency (Hz)
const int resolution = 8;   // PWM resolution (8-bit = 0-255)
const int R_CHANNEL = 0;    // LEDC channel for Red
const int G_CHANNEL = 1;    // LEDC channel for Green
const int B_CHANNEL = 2;    // LEDC channel for Blue

// --- RGB Sine Wave Constants (These remain global as they are constants) ---
const float FREQUENCY = 0.015; // Controls the speed of the cycle. Smaller number = slower cycle.
const float SCALING_FACTOR = 127.5; // Scales sine wave output [-1, 1] to [0, 255]

// Phase Shifts (in radians): 120 and 240 degrees for R, G, B
const float PHASE_SHIFT_G = (2.0 * M_PI) / 3.0; // 120 degrees
const float PHASE_SHIFT_B = (4.0 * M_PI) / 3.0; // 240 degrees

// Function to calculate the inverted duty cycle for active-low LEDs
int calculateDuty(int colorValue) {
  // Duty cycle is inverted: 0 (max brightness) to 255 (off)
  // The calculated colorValue (0-255) must be inverted before writing to the LEDC.
  return 255 - colorValue;
}

/**
 * Writes the color to the LEDC channels.
 * @param r Red component (0-255). 0=off, 255=max brightness.
 * @param g Green component (0-255). 0=off, 255=max brightness.
 * @param b Blue component (0-255). 0=off, 255=max brightness.
 */
void writeRGBLED(int r, int g, int b) {
  // Write the inverted duty cycle to each channel
  ledcWrite(R_CHANNEL, calculateDuty(r));
  ledcWrite(G_CHANNEL, calculateDuty(g));
  ledcWrite(B_CHANNEL, calculateDuty(b));
}

/**
 * @function setRGBColor (parameterless version)
 * Calculates the next Red, Green, and Blue values based on a STATIC time variable
 * using phase-shifted sine waves, and then sets the LED color.
 */
void setRGBColor() {
  // 'staticTimeCounter' is now correctly declared as STATIC inside the function.
  // It retains its value between calls but is not accessible outside this function.
  static float staticTimeCounter = 0.0; // Initialization happens only once

  // R(t) = floor(127.5 * (1 + sin(F * t)))
  const int r = (int) (SCALING_FACTOR * (1.0 + sin(FREQUENCY * staticTimeCounter)));

  // G(t) = floor(127.5 * (1 + sin(F * t + 2pi/3)))
  const int g = (int) (SCALING_FACTOR * (1.0 + sin(FREQUENCY * staticTimeCounter + PHASE_SHIFT_G)));

  // B(t) = floor(127.5 * (1 + sin(F * t + 4pi/3)))
  const int b = (int) (SCALING_FACTOR * (1.0 + sin(FREQUENCY * staticTimeCounter + PHASE_SHIFT_B)));

  // Write the calculated values to the LED
  writeRGBLED(r, g, b);

  // Increment the static time variable for the next iteration
  staticTimeCounter += 1.0; 
  
  // Prevent float from getting too large (though it takes a very long time)
  if (staticTimeCounter > 65535.0) {
      staticTimeCounter = 0.0;
  }

  // Debugging output
  Serial.printf("t=%.0f | R:%d, G:%d, B:%d\n", staticTimeCounter, r, g, b);
}

// ---------------------------
// Setup Function
// ---------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\n--- ESP32 RGB Sine Wave Color Cycler Started ---");

  // Configure LEDC channels
  ledcSetup(R_CHANNEL, freq, resolution);
  ledcSetup(G_CHANNEL, freq, resolution);
  ledcSetup(B_CHANNEL, freq, resolution);

  // Attach LEDC channels to the respective GPIO pins and channels
  ledcAttachPin(LED_R_PIN, R_CHANNEL);
  ledcAttachPin(LED_G_PIN, G_CHANNEL);
  // FIX: Ensure LED_B_PIN is attached to the B_CHANNEL, not itself.
  ledcAttachPin(LED_B_PIN, B_CHANNEL); 

  Serial.println("Initial Cycle Phase Set to Red. Check LED for smooth rainbow effect!");
}

// ---------------------------
// Main Loop Function
// ---------------------------
void loop() {
  // Continuously calculate and set the next color in the sine wave cycle
  setRGBColor();
  
  // Delay slightly to control the frame rate and visual speed.
  delay(10); 
}