# BME280と電子ペーパーを使った自室の環境モニタリングデバイス

![電子ペーパの画像](e-paper.png)
![基板の画像](pcb.png)

## 構成

- Bosh BME280
- Waveshare 4.2inch e-Paper Module
- ESP32-DevKitC-32E

## 仕様

- 10mごとにデータ取得・更新
- データをInfluxDBに送信
- e-Paperの表示更新

- 毎日定時(0:00)にNTPから時刻同期
