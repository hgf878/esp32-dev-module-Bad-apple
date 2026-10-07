# Bad Apple on ESP32

用一块 ESP32 和一块 128×64 的 SSD1306 OLED，把 Bad Apple 的黑白剪影动画跑起来，
再用一只无源蜂鸣器同步演奏主旋律。画面数据编译进 Flash（PROGMEM），不需要 SD 卡。

## 效果

- OLED（I2C，0x3C）播放 1948 帧 1-bit 画面，10 fps，约 195 秒
- 无源蜂鸣器演奏从 MIDI 自动提取的主旋律，与画面同步起止
- 全部数据打包进固件，接线只有 4 根线

## 硬件

| 器件 | 型号 / 说明 |
|---|---|
| 主控 | ESP32 Dev Module（4 MB Flash） |
| 显示 | SSD1306 128×64 OLED，I2C 接口 |
| 发声 | 无源蜂鸣器（有源蜂鸣器不可用） |

接线：

| ESP32 | OLED | 蜂鸣器 |
|---|---|---|
| 3V3 | VCC | — |
| GND | GND | -（负极） |
| GPIO 21 | SDA | — |
| GPIO 22 | SCL | — |
| GPIO 25 | — | +（可串电阻，也可不串电阻） |

OLED 必须用 3.3 V 供电，接 5 V 会烧屏。

## 依赖

Arduino IDE + ESP32 开发板支持包，另需两个库：

- Adafruit SSD1306
- Adafruit GFX Library
