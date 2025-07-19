# SoilPeg

CircuitPythonを使用した土壌水分・温度・照度センサーシステム

## ハードウェア構成

- マイコン: ESP32C6
- センサー:
  - 照度センサ (A0)
  - 静電容量式土壌水分センサ (A1)
  - 温度センサ (A2)
- PWM出力 (D10): 静電容量センサ用励振信号
<img width="803" height="586" alt="Image" src="https://github.com/user-attachments/assets/cc775021-70cc-49d0-bd2f-81cb3e0ac4cd" />
<img width="222" height="537" alt="Image" src="https://github.com/user-attachments/assets/951091b8-3fb6-430a-96b5-312cbb25f2b3" />

## 機能

- 各種センサーの定期測定（10秒間隔）
- 温度センサ値の10点移動平均処理
- 省電力動作（測定間隔中はディープスリープ）
- PWMは測定時のみ有効

## 出力フォーマット

データはCSV形式でシリアル出力されます：
```
照度センサ生値,土壌水分センサ値[V],温度[℃]
```

## 開発環境

- CircuitPython
- ESP32C6開発ボード
