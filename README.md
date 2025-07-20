# SoilPeg

CircuitPythonを使用した土壌水分・温度・照度センサーシステム

<img width="248" height="368" alt="Image" src="https://github.com/user-attachments/assets/2c9b0598-1c3a-4712-9a00-4a0b5a95afde" />

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


## キット部品組み立て
<img width="320" height="490" alt="Image" src="https://github.com/user-attachments/assets/cf7577a9-ffc3-47a6-ad41-058b5f68bf5c" />

## （動画）Xiao ESP32C6にCircuitPyhtonをインストールする手順（ブラウザ使用）

https://youtu.be/3VY-Rl7ak7s
<img width="1028" height="677" alt="Image" src="https://github.com/user-attachments/assets/06dd7ba7-cb06-46bf-9e31-6d3da825fa9f" />
