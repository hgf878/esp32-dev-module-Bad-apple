/*  Bad Apple on ESP32 + SSD1306 (I2C) + 无源蜂鸣器   —— 195 秒对齐版
 *
 *  同目录必须有三个文件：
 *    BadApple.ino
 *    frames_data.h   <- BadApple_to_Frames.py 生成（约 1948 帧 @10fps）
 *    melody.h        <- midi_to_buzzer.py 生成（USE_TRACK=3, FILL_TRACKS=[1]）
 *
 *  接线：
 *    OLED    VCC->3V3  GND->GND  SDA->GPIO21  SCL->GPIO22
 *    蜂鸣器  +->GPIO25（串 1kΩ）  -->GND
 *
 *  开发板: ESP32 Dev Module
 *  分区:   Huge APP (3MB No OTA/1MB SPIFFS)
 */
#include <Arduino.h
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "frames_data.h"
#include "melody.h"

// ================== 配置 ==================
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C     // 屏幕不亮改成 0x3D

#define BUZZER_PIN    25       // 无源蜂鸣器

// 1 = 跳过音乐开头的静音，画面第一帧就出声
//     melody.h 第一项若是 {0,14575}，跳过后续播放 195.2 秒，与 195 秒视频对齐
#define SKIP_LEAD_SILENCE  1
// ==========================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------- 蜂鸣器 ----------------
uint32_t noteStartMs = 0;
uint16_t noteIdx     = 0;
bool     playing     = false;

void buzzerOut(uint16_t f) {
  if (f == 0) noTone(BUZZER_PIN);      // 休止符
  else        tone(BUZZER_PIN, f);
}

void melodyStart() {
  noteIdx = 0;

#if SKIP_LEAD_SILENCE
  // 只跳过最前面那段连续静音，中间的休止符照旧保留
  while (noteIdx < MELODY_LEN && MELODY[noteIdx].f == 0) noteIdx++;
#endif

  noteStartMs = millis();
  playing     = (noteIdx < MELODY_LEN);
  if (playing) buzzerOut(MELODY[noteIdx].f);
  else         noTone(BUZZER_PIN);
}

void melodyUpdate() {
  if (!playing) return;

  uint32_t now = millis();
  if (now - noteStartMs < MELODY[noteIdx].d) return;

  noteStartMs += MELODY[noteIdx].d;    // 累加，避免取整误差累积
  noteIdx++;

  if (noteIdx >= MELODY_LEN) {         // 曲终
    playing = false;
    noTone(BUZZER_PIN);
    return;
  }
  buzzerOut(MELODY[noteIdx].f);
}

// ---------------- 初始化 ----------------
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  Wire.begin(21, 22);          // SDA=21, SCL=22
  Wire.setClock(400000);       // 花屏或不亮就改回 100000

  // 你的库是 1.x 签名：begin(switchvcc, i2caddr)
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("OLED init failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Playing Bad Apple..."));
  display.display();
  delay(1500);                 // 开场字幕停留

  // 串口自检
  uint32_t leadMs = 0;
  if (MELODY_LEN > 0 && MELODY[0].f == 0) leadMs = MELODY[0].d;
  uint32_t effMs = MELODY_TOTAL_MS;
#if SKIP_LEAD_SILENCE
  effMs = MELODY_TOTAL_MS - leadMs;
#endif

  Serial.print(F("Frames: "));      Serial.print(TOTAL_FRAMES);
  Serial.print(F(" @ "));           Serial.print(TARGET_FPS);
  Serial.print(F(" fps = "));       Serial.print(TOTAL_FRAMES * 1000UL / TARGET_FPS);
  Serial.println(F(" ms"));
  Serial.print(F("Melody: "));      Serial.print(MELODY_LEN);
  Serial.print(F(" events, lead ")); Serial.print(leadMs);
  Serial.print(F(" ms, play "));    Serial.print(effMs);
  Serial.println(F(" ms"));

  melodyStart();               // 字幕结束后画面与音乐同时起播
}

// ---------------- 主循环 ----------------
void loop() {
  melodyUpdate();              // 非阻塞，独立推进

  static uint32_t frame   = 0;
  static uint32_t last_us = 0;

  const uint32_t interval_us = 1000000UL / TARGET_FPS;

  uint32_t now = micros();
  if (now - last_us < interval_us) return;   // 未到下一帧，空转
  last_us = now;

  // 两色版：位为 1 画白、为 0 画黑，每帧全像素重写，无残影
  display.drawBitmap(0, 0, frames[frame], FRAME_W, FRAME_H,
                     SSD1306_WHITE, SSD1306_BLACK);
  display.display();
  frame++;
  if (frame >= TOTAL_FRAMES) {
    frame = 0;
    melodyStart();             // 画面重头再来，曲子也从头
  }
}
