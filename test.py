# 必要なライブラリのインポート
from analogio import AnalogIn     # アナログ入力用
from board import *            # ピン定義用
import pwmio                  # PWM出力用
import time                     # 時間処理用
import alarm

# 温度の移動平均用バッファ（最大10個のデータを保持）
temp_buffer = []

# センサーの初期化
def initialize_sensors():
    # PWM出力の設定（静電容量センサ用）
    # 周波数: 1MHz, デューティ比: 50%
    pwm = pwmio.PWMOut(D10, frequency=1_000_000)
    pwm.duty_cycle = int(65536 * 0.5)  # 16ビットPWMの50%
    
    # センサーの初期化
    lp = AnalogIn(A0)    # 照度センサ
    soil = AnalogIn(A1)  # 土壌水分センサ（静電容量式）
    tmp = AnalogIn(A2)   # 温度センサ
    
    return pwm, lp, soil, tmp

# センサーの終了処理
def cleanup_sensors(pwm, lp, soil, tmp):
    pwm.deinit()
    lp.deinit()
    soil.deinit()
    tmp.deinit()

# メイン処理
def main():
    pwm = None
    lp = None
    soil = None
    tmp = None
    
    try:
        # センサーの初期化
        pwm, lp, soil, tmp = initialize_sensors()
        
        # PWMが正しく設定されるまで少し待機
        time.sleep(0.1)
        
        # センサー値の読み取りと変換
        light_raw = lp.value                          # 照度センサの生値
        soil_voltage = soil.value / 65536 * 3.3       # 土壌水分センサ値を電圧に変換
        temp_celsius = ((tmp.value / 65536 * 3.2) - 0.5) * 100  # 温度センサ値を摂氏に変換
        
        # 温度データを移動平均バッファに追加
        temp_buffer.append(temp_celsius)
        # バッファが10個を超えた場合、古いデータを削除
        if len(temp_buffer) > 10:
            temp_buffer.pop(0)
        
        # 移動平均の計算
        temp_avg = sum(temp_buffer) / len(temp_buffer)
        
        # データの出力（CSV形式）
        print(f"{light_raw},{soil_voltage},{temp_avg}")
        
    except Exception as e:
        print(f"Error: {str(e)}")
        
    finally:
        # センサーの終了処理
        if any([pwm, lp, soil, tmp]):  # 初期化済みのセンサーのみ終了処理
            cleanup_sensors(pwm, lp, soil, tmp)

# メインループ
while True:
    # メイン処理の実行（センサー初期化、測定、データ出力）
    main()
    
    # シリアル出力完了を待つ
    time.sleep(0.1)
    
    # 20秒間のディープスリープを設定
    # monotonic_timeは現在時刻からの相対時間（秒）
    time_alarm = alarm.time.TimeAlarm(monotonic_time=time.monotonic() + 20)
    
    # ディープスリープに入る
    # - 全てのペリフェラルが停止
    # - 指定時間後に自動的に再起動
    # - 再起動後はプログラムの最初から実行される
    alarm.exit_and_deep_sleep_until_alarms(time_alarm)
