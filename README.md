# SoilPeg

土壌水分・温度・照度センサーシステム（PlatformIO / Arduino）

<img width="248" height="368" alt="Image" src="https://github.com/user-attachments/assets/2c9b0598-1c3a-4712-9a00-4a0b5a95afde" />

## ハードウェア構成

- マイコン: Seeed Studio XIAO ESP32C6
- センサー:
  - 照度センサ (A0 / GPIO0)
  - 静電容量式土壌水分センサ (A1 / GPIO1)
  - 温度センサ (A2 / GPIO2)
- PWM出力 (D10 / GPIO18): 静電容量センサ用励振信号（1MHz / デューティ比50%、測定時のみ有効）
<img width="803" height="586" alt="Image" src="https://github.com/user-attachments/assets/cc775021-70cc-49d0-bd2f-81cb3e0ac4cd" />
<img width="222" height="537" alt="Image" src="https://github.com/user-attachments/assets/951091b8-3fb6-430a-96b5-312cbb25f2b3" />

## 機能

- 各種センサーの定期測定（10秒間隔）
- 温度センサ値の10点移動平均処理
- PWMは測定時のみ有効
- 測定結果をCSV形式でシリアル出力（115200bps）

## 出力フォーマット

データはCSV形式でシリアル出力されます（115200bps）：
```
照度[0-100],土壌水分[0-100%],温度[℃]
```

例：
```
22,45,18.4
```

## 校正

各センサーの校正値は [`src/main.cpp`](src/main.cpp) 冒頭の定数で調整します。
校正中は `TEMP_DEBUG` / `SOIL_DEBUG` を `true` にすると、`# temp raw=...` /
`# soil raw16=...` の生データ行が追加出力されます。

### 温度

- ESP32のADCは非線形なため、eFuse工場校正を用いる `analogReadMilliVolts()` で
  実電圧を取得し、TMP36式 `(mV - 500) / 10` で℃に変換しています。
- 個体差・配線によるオフセットは `TEMP_OFFSET_C` で1点校正します
  （既知の室温に合わせ込む）。

### 土壌水分

1. `SOIL_DEBUG = true` で生データ（`# soil raw16=...`）を出力。
2. 空気中（乾燥）の生値を `SOIL_RAW_DRY`（0%）に記録。
3. 水中／十分濡れた土（湿潤）の生値を `SOIL_RAW_WET`（100%）に記録。
4. 静電容量式は通常「乾くと値が高く、濡れると低い」ので `DRY > WET` になります。
   逆の場合は2値を入れ替えます。

## 開発環境

- PlatformIO + Arduino framework
- Seeed Studio XIAO ESP32C6

ESP32-C6 は Arduino core 3.x が必要なため、[pioarduino](https://github.com/pioarduino/platform-espressif32) 版の espressif32 プラットフォームを使用しています（`platformio.ini` で自動取得）。

### ビルドと書き込み

```
pio run                 # ビルド
pio run -t upload       # 書き込み
pio device monitor      # シリアルモニタ (115200bps)
```


## キット部品組み立て
<img width="320" height="490" alt="Image" src="https://github.com/user-attachments/assets/cf7577a9-ffc3-47a6-ad41-058b5f68bf5c" />

## 参考資料

- [`test.py`](test.py): 開発初期に CircuitPython で動作確認に使用したテストコードです。現在のファームウェアは PlatformIO / Arduino（[`src/main.cpp`](src/main.cpp)）に移行しており、`test.py` はセンサー配線や校正式の参考として残しています。
