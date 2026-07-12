/*
 * SoilPeg - 土壌水分・温度・照度センサーシステム
 *
 * ハードウェア構成 (Seeed Studio XIAO ESP32C6):
 *   - 照度センサ                 : A0 (GPIO0)
 *   - 静電容量式土壌水分センサ   : A1 (GPIO1)
 *   - 温度センサ                 : A2 (GPIO2)
 *   - PWM励振信号 (静電容量センサ用): D10 (GPIO18) 1MHz / デューティ比50%
 *
 * 動作:
 *   10秒に1回センサー値を測定し、CSV形式でシリアル出力します。
 *   出力フォーマット: 照度センサ生値,土壌水分センサ値,温度[℃]
 */

#include <Arduino.h>

// ===== ピン定義 (XIAO ESP32C6) =====
static const int PIN_LIGHT = A0;   // 照度センサ  (GPIO0)
static const int PIN_SOIL  = A1;   // 土壌水分センサ (GPIO1)
static const int PIN_TEMP  = A2;   // 温度センサ  (GPIO2)
static const int PIN_PWM   = D10;  // 静電容量センサ用励振信号 (GPIO18)

// ===== 測定パラメータ =====
static const unsigned long MEASURE_INTERVAL_MS = 10000UL;  // 10秒に1回

// PWM設定: 1MHz。
// ESP32-C6 の LEDC は自動選択されるクロックが低く、高分解能では
// 分周比を確保できず attach に失敗する（div_param=0）。
// 1MHz を確実に生成するため分解能は 3bit に抑える (2^3 = 8, 50% = 4)。
static const uint32_t PWM_FREQ_HZ   = 1000000;
static const uint8_t  PWM_RES_BITS  = 3;
static const uint32_t PWM_DUTY_50   = (1 << PWM_RES_BITS) / 2;  // = 4

// ===== 温度センサ校正 (TMP36系: 0℃で500mV, 10mV/℃) =====
static const float TEMP_MV_AT_0C   = 500.0f;  // 0℃時の出力[mV]
static const float TEMP_MV_PER_C   = 10.0f;   // 温度係数[mV/℃]
static const float TEMP_OFFSET_C   = -11.3f;  // 1点校正: 室温18℃で合わせ込み
static const int   TEMP_SAMPLES    = 16;      // 平均サンプル数
static const bool  TEMP_DEBUG      = false;   // 校正中は測定mVを出力

// ===== 土壌水分センサ校正 =====
// 校正手順: 空気中(乾燥)=min, 水中/濡れ土(湿潤)=max の生値を記録する。
// 静電容量式は「乾くと出力が高く、濡れると低い」ため、下の SOIL_RAW_DRY /
// SOIL_RAW_WET に記録した生値を入れて 0..100% に変換する。
static const int   SOIL_SAMPLES    = 16;      // 平均サンプル数
static const bool  SOIL_DEBUG      = false;   // 校正中は生データを出力
static const long  SOIL_RAW_DRY    = 39500;   // 乾燥時(0%)の生値[16bit相当]
static const long  SOIL_RAW_WET    = 30000;   // 湿潤時(100%)の生値[16bit相当]

// 温度の移動平均用バッファ（最大10個）
static const int TEMP_BUFFER_SIZE = 10;
static float tempBuffer[TEMP_BUFFER_SIZE];
static int   tempCount = 0;   // 蓄積済みサンプル数
static int   tempHead  = 0;   // 次の書き込み位置（リングバッファ）

// マッピング関数（CircuitPython版 map_fit 相当）
static long mapFit(long x, long inMin, long inMax, long outMin, long outMax) {
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

// 16bit相当のADC値を読み取る（Arduinoの12bit値を<<4で正規化）
static int readAdc16(int pin) {
  int raw12 = analogRead(pin);   // 0..4095 (12bit)
  return raw12 << 4;             // 0..65520 (16bit相当)
}

// 16bit相当のADC値を平均読み取り（ノイズ低減）
static int readAdc16Avg(int pin, int samples) {
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += (uint32_t)(analogRead(pin) << 4);
  }
  return (int)(sum / samples);
}

// 工場出荷時eFuse校正を用いて正確な電圧[mV]を平均読み取り
static float readMilliVolts(int pin, int samples) {
  uint32_t sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogReadMilliVolts(pin);
  }
  return (float)sum / samples;
}

// 温度の移動平均を計算
static float updateTempAverage(float sample) {
  tempBuffer[tempHead] = sample;
  tempHead = (tempHead + 1) % TEMP_BUFFER_SIZE;
  if (tempCount < TEMP_BUFFER_SIZE) {
    tempCount++;
  }

  float sum = 0.0f;
  for (int i = 0; i < tempCount; i++) {
    sum += tempBuffer[i];
  }
  return sum / tempCount;
}

// 1回分の測定と出力
static void measureAndReport() {
  // PWM励振を有効化し、安定するまで少し待機
  bool pwmOk = ledcAttach(PIN_PWM, PWM_FREQ_HZ, PWM_RES_BITS);
  if (pwmOk) {
    ledcWrite(PIN_PWM, PWM_DUTY_50);
  } else {
    Serial.println("# WARN: PWM attach failed");
  }
  delay(100);

  // センサー値の読み取り
  int lightRaw   = readAdc16(PIN_LIGHT);
  int soilRaw    = readAdc16Avg(PIN_SOIL, SOIL_SAMPLES);

  // 変換
  long light   = mapFit(lightRaw, 0, 12000, 0, 100);      // 照度: 0..100
  long soil    = mapFit(soilRaw, SOIL_RAW_WET, SOIL_RAW_DRY, 100, 0);  // 土壌水分: 100..0
  if (soil < 0)   soil = 0;      // 範囲外をクランプ
  if (soil > 100) soil = 100;

  if (SOIL_DEBUG) {
    float soilMv = readMilliVolts(PIN_SOIL, SOIL_SAMPLES);
    Serial.printf("# soil raw16=%d (12bit=%d, %.1fmV) -> %ld%%\n",
                  soilRaw, soilRaw >> 4, soilMv, soil);
  }

  // 温度: eFuse校正済みの実電圧[mV]から TMP36 式で算出
  float tempMv = readMilliVolts(PIN_TEMP, TEMP_SAMPLES);
  float tempC  = (tempMv - TEMP_MV_AT_0C) / TEMP_MV_PER_C + TEMP_OFFSET_C;

  // 温度の移動平均
  float tempAvg = updateTempAverage(tempC);

  if (TEMP_DEBUG) {
    Serial.printf("# temp raw=%.1fmV -> %.2fC (avg %.2fC)\n", tempMv, tempC, tempAvg);
  }

  // PWMを停止（測定時のみ有効）
  if (pwmOk) {
    ledcDetach(PIN_PWM);
  }

  // CSV形式で出力: 照度,土壌水分,温度
  Serial.printf("%ld,%ld,%.1f\n", light, soil, tempAvg);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // シリアル接続の安定待ち

  analogReadResolution(12);  // ESP32C6のADCは12bit

  Serial.println("SoilPeg started: light,soil,temp");
}

void loop() {
  measureAndReport();
  delay(MEASURE_INTERVAL_MS);  // 測定間隔待機
}
