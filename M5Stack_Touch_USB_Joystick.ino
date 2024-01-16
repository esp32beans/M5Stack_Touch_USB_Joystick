/*
 * MIT License
 *
 * Copyright (c) 2024 esp32beans@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
 * For M5Stack Dial(StampS3)
 * Convert capacitive touch screen (x,y) to USB joystick (x,y).
 * The USB joystick works with the Xbox Adaptive Controller (XAC)
 * as well as a generic USB HID joystick on Windows, MacOS, and Linux.
 *
 * M5Stack Dial draws less than 100 mA with its display backlight at a low
 * level. The bottom of the ring (near the USB connector) around the
 * display is a button. The rotary ring is not used.
 *
 * M5Stack CoreS3 draws more than 100 mA even with its display backlight
 * turned off. This is more current than the XAC can provide unless it is
 * powered by a 5V 2A power supply. This is not a problem for PCs which can
 * provide up to 500 mA. The CoreS3 is tricky to use so it is not
 * recommended.
 *
 */

#define M5_DEBUG    (0)
#define M5_DISPLAY  (1)

#if defined(ARDUINO_M5STACK_CORES3)
#include <M5CoreS3.h>
#define M5Device CoreS3
#define M5_BRIGHTNESS (75)
#define M5_BUTTON (0)
#elif defined(ARDUINO_STAMP_S3)
#include <M5Dial.h>
#define M5Device M5Dial
#define M5_BRIGHTNESS (10)
#define M5_BUTTON (1)
#endif
#include "USB.h"
#include "ESP32_flight_stick.h"
ESP32_flight_stick FSJoy;

const int JOY_AXIS_MIN = 0;
const int JOY_AXIS_MAX = 1<<10;
const int JOY_AXIS_MID = JOY_AXIS_MAX / 2;

void setup() {
  auto cfg = M5.config();
  M5Device.begin(cfg);
  M5Device.Display.setBrightness(M5_BRIGHTNESS);
  FSJoy.begin();
  USB.begin();
#if M5_DEBUG
  Serial.begin(115200);
  while (!Serial && (millis() < 4000)) delay(10);
  Serial.println("M5 Touch USB Joystick");
#endif

#if M5_DISPLAY
  M5Device.Display.setTextColor(GREEN);
  M5Device.Display.setTextDatum(middle_center);
  M5Device.Display.setFont(&fonts::Orbitron_Light_24);
  M5Device.Display.setTextSize(1);

  M5Device.Display.drawString("Touch Joystick", M5Device.Display.width() / 2,
                            M5Device.Display.height() / 2);
  delay(1000);
  M5Device.Display.clear();
#endif
}

void loop() {
  static int x_min = 32;
  static int x_max = M5Device.Display.width() - 32;
  static int y_min = 32;
  static int y_max = M5Device.Display.height() - 32;
  static int x_axis_last = -2;
  static int y_axis_last = -2;
  int x_axis;
  int y_axis;
  bool joystick_change = false;

  M5Device.update();

#if M5_BUTTON
  if (M5Dial.BtnA.wasPressed()) {
    FSJoy.press(0);
    joystick_change = true;
  }
  if (M5Dial.BtnA.wasReleased()) {
    FSJoy.release(0);
    joystick_change = true;
  }
#endif

  auto t = M5Device.Touch.getDetail();
  if ((t.x == -1) && (t.y == -1)) {
    // Serial.println("no touch yet");
#if M5_DISPLAY
    M5Device.Display.clear();
#endif
    x_axis = JOY_AXIS_MID;
    y_axis = JOY_AXIS_MID;
  } else if (t.state == m5::touch_state_t::none) {
    // Serial.println("no touch");
#if M5_DISPLAY
    M5Device.Display.clear();
#endif
    x_axis = JOY_AXIS_MID;
    y_axis = JOY_AXIS_MID;
  }
  else {
    if (t.x > x_max) x_max = t.x;
    if (t.x < x_min) x_min = t.x;
    x_axis = map(t.x, x_min, x_max, JOY_AXIS_MIN, JOY_AXIS_MAX);
    if (t.y > y_max) y_max = t.y;
    if (t.y < y_min) y_min = t.y;
    y_axis = map(t.y, y_min, y_max, JOY_AXIS_MIN, JOY_AXIS_MAX);
  }

  if ((x_axis_last != x_axis) || (y_axis_last != y_axis)) {
    FSJoy.xAxis(x_axis);
    FSJoy.yAxis(y_axis);
    joystick_change = true;
    x_axis_last = x_axis;
    y_axis_last = y_axis;
#if M5_DISPLAY
    static int prev_t_x = 0;
    static int prev_t_y = 0;
    M5Device.Display.drawCircle(prev_t_x, prev_t_y, 50, BLACK);
    M5Device.Display.drawCircle(t.x, t.y, 50, GREEN);
    prev_t_x = t.x;
    prev_t_y = t.y;
#endif
  }
  if (joystick_change) {
    joystick_change = false;
    FSJoy.write();
  }
  delay(5);
}
