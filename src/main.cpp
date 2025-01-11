// Copyright(c) 2025 washishi
#include <M5Unified.h>
#include "fft.hpp"
#include <cinttypes>
#include "FastLED.h"
#if defined(ECHO_BASE) || defined(ECHO_BASE_S3)
#include <M5EchoBase.h>
M5EchoBase echobase(I2S_NUM_0);
#endif

#ifndef PDM_CLK
#define PDM_CLK 22
#endif
#ifndef PDM_DAT
#define PDM_DAT 19
#endif
#ifndef LED_GPIO
#define LED_GPIO 26
#endif

#define NUM_PUZZLE_UNIT 4 // Puzzle unit の数 1～16 
#define NUM_LEDS (NUM_PUZZLE_UNIT * 64) // RGB LEDの数

uint8_t angle = 0;    // 表示の向き 0:↑ 1:↓
uint8_t reverse = 0;  // 周波数レンジ方向 0:低→高 1:高→低
CRGB leds[NUM_LEDS];
// 点灯時のLEDの色指定 (緑、緑、緑、緑、黄、黄、赤、赤)
CRGB led_table[8] = {0x00FF00,0x00FF00,0x00FF00,0x00FF00,0xFFFF00,0xFFFF00,0xFF0000,0xFF0000};

// LED の順番 (Unit Puzzle)
// Puzzle unit は 8x8 64個のRGB LEDが付いている
//                      (IN)  
// LED0  LED1  LED2  LED3  LED4  LED5  LED6  LED7    → LEVEL
// LED8  LED9  LED10 LED11 LED12 LED13 LED14 LED15   ↓ FREQ
// LED16 LED17 LED18 LED19 LED20 LED21 LED22 LED23  
// LED24 LED25 LED26 LED27 LED28 LED29 LED30 LED31 
// LED32 LED33 LED34 LED35 LED36 LED37 LED38 LED39
// LED40 LED41 LED42 LED43 LED44 LED45 LED46 LED47
// LED48 LED49 LED50 LED51 LED52 LED53 LED54 LED55
// LED56 LED57 LED58 LED59 LED60 LED61 LED62 LED63
//                      (OUT)  

void turn_off_led() {
  // Now turn the LED off, then pause
  for(int i=0;i< NUM_LEDS;i++) leds[i] = CRGB::Black;
  FastLED.show();
}

void fill_led_buff(CRGB color) {
  // Now turn the LED off, then pause
  for(int i=0;i< NUM_LEDS;i++) leds[i] =  color;
}

void clear_led_buff() {
  // Now turn the LED off, then pause
  for(int i=0;i< NUM_LEDS;i++) leds[i] =  CRGB::Black;
}

void level_led(uint8_t *led_level) {
  clear_led_buff();
  for (int i=0;i<NUM_PUZZLE_UNIT * 8;i++){  
    if(led_level[i] > 8) led_level[i] = 8;
    for(int j=0;j<led_level[i];j++){
      if(angle == 0){
        if(reverse == 0){
          leds[i*8+j] = led_table[j];
        } else {
          leds[(NUM_PUZZLE_UNIT * 8 - i - 1 ) * 8 + j] = led_table[j];
        }
      } else {
        if(reverse == 0){
          leds[i * 8 + (7-j)] = led_table[j];
        } else {
          leds[(NUM_PUZZLE_UNIT * 8 - i - 1) * 8 + (7-j)] = led_table[j];
        }
      }
    }
  }
  FastLED.show();
}

  #define READ_LEN    (2 * 256)
  #define LEVEL_MAX 8.0f
  static fft_t fft;
  static constexpr size_t WAVE_SIZE = 256 * 2;
  static constexpr const size_t record_samplerate = 16000;
  static int16_t *rec_data;
  float level_max = LEVEL_MAX;
  uint32_t last_max_time_msec = 0;

void level_check() {
  uint64_t level;
  uint8_t led_level[NUM_PUZZLE_UNIT * 8];
  uint8_t segment_size = 128 / (NUM_PUZZLE_UNIT * 8);
  float ratio;
  float ratio_max = 0;
#if defined(ECHO_BASE) || defined(ECHO_BASE_S3)
  size_t bytes_read = 0;
  if (i2s_read(I2S_NUM_0, rec_data, WAVE_SIZE, &bytes_read, echobase.getDuration(WAVE_SIZE)) == ESP_OK){
#else
  if ( M5.Mic.record(rec_data, WAVE_SIZE, record_samplerate)){
#endif
    fft.exec(rec_data);
    for (uint8_t i=0;i<8*NUM_PUZZLE_UNIT;i++){
      level = 0;
      for (uint8_t j=i*segment_size;j<(i+1)*segment_size;j++){
        int32_t f = fft.get(j);
        level += abs(f);
      }
      uint32_t temp_level = level / segment_size;
      ratio = (float)(temp_level / level_max);
      if (ratio > ratio_max) ratio_max = ratio;
      if (ratio > 8.0f) ratio = 8.0f;
      led_level[i] = (uint8_t)(ratio);
//    led_level[i] = 8; // LEDの色調整用 コメントを外すと全点灯します
    }
    level_led(led_level);
    
// 自動レベル調整
    if (ratio_max < 8.0f) {      
      if ((lgfx::v1::millis() - last_max_time_msec) > 3000) {
        // 3秒以上一定以下の場合は上限を下げる
        if (level_max > 0){
          level_max -= 10.0f;
          last_max_time_msec = lgfx::v1::millis();
        } 
      }
    } else {
      if (ratio_max > 8.0f){
        // 上限を大きく超えている場合は上限を上げる
        level_max += 10.0f;
      }      
      last_max_time_msec = lgfx::v1::millis(); 
    }
    //M5_LOGI("%f %f %d\n",ratio_max,level_max,lgfx::v1::millis() - last_max_time_msec);
  }
}

void setup()
{
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO);
  M5.Log.setEnableColor(m5::log_target_serial, false);
  #ifdef ECHO_BASE
  echobase.init(16000 /*Sample Rate*/, 25 /*I2C SDA*/, 21 /*I2C SCL*/, 23 /*I2S DIN*/, 19 /*I2S WS*/, 22 /*I2S DOUT*/, 33 /*I2S BCK*/, Wire);
  #endif
  #ifdef ECHO_BASE_S3
  echobase.init(16000 /*Sample Rate*/, 38 /*I2C SDA*/, 39 /*I2C SCL*/, 7 /*I2S DIN*/, 6 /*I2S WS*/, 5 /*I2S DOUT*/, 8 /*I2S BCK*/, Wire);
  #endif
  #if defined(ECHO_BASE) || defined(ECHO_BASE_S3)
  i2s_set_clk(I2S_NUM_0,16000,I2S_BITS_PER_CHAN_16BIT,I2S_CHANNEL_MONO);  // EchoBaseのライブラリのチャンネル形式が異なるため変更
  echobase.setMicGain(ES8311_MIC_GAIN_6DB);  // Set microphone gain to 6dB.
  #else
  auto mic_cfg = M5.Mic.config();
  mic_cfg.sample_rate = 16000;
  mic_cfg.pin_ws = PDM_CLK;
  mic_cfg.pin_data_in = PDM_DAT;
  M5.Mic.config(mic_cfg);
  M5.Mic.begin();
  #endif
  rec_data = (typeof(rec_data))heap_caps_malloc(WAVE_SIZE * sizeof(int16_t), MALLOC_CAP_8BIT);
  memset(rec_data, 0 , WAVE_SIZE * sizeof(int16_t));
  int num_leds = NUM_LEDS;
  digitalWrite(LED_GPIO,LOW); // 1つ目のLEDが正常に動かないことがあるため追加
  FastLED.addLeds<WS2812, LED_GPIO, GRB>(leds, num_leds);  // GRB ordering is typical
  FastLED.setBrightness(10);
  turn_off_led();
  FastLED.show();
  delay(500);
}

void loop()
{
  M5.update();
  if (M5.BtnA.wasReleasedAfterHold()) {
    reverse = ! reverse;
  } else {
    if (M5.BtnA.wasClicked()) {
      angle = ! angle;
    }
  }
  if (M5.BtnPWR.wasClicked()) {
#ifdef ARDUINO
    esp_restart();
#endif
  }
  level_check();
  lgfx::v1::delay(1);
}
