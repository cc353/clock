# ESP32-C3 Wi-Fi 时钟 Demo

使用合宙的 ESP32-C3 开发板，ST7735 128x160 TFT屏，开发的简单 Wi-Fi 时钟 Demo。

## 编译

1. 安装`esp-idf v4.4`开发环境
2. `git clone` 本项目
3. `git submodule update --init` 拉取lvgl
4. 使用`idf menuconfig`配置 WiFi SSID 和 Password
5. 编译、烧录

## 接线

```c
#define ST77XX_CS_PIN       7
#define ST77XX_SCK_PIN      2
#define ST77XX_MOSI_PIN     3
#define ST77XX_RES_PIN      10
#define ST77XX_DC_PIN       6
#define ST77XX_BL_PIN       11
```