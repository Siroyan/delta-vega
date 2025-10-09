# CLAUDE.md

このファイルはClaude Code (claude.ai/code) がこのリポジトリで作業する際のガイダンスを提供します。

## 言語設定

**重要: このリポジトリでユーザーとコミュニケーションする際は、必ず日本語で回答してください。**

## プロジェクト概要

**delta-vega** は、ホンダエコマイレッジチャレンジ2024のためのESP32-S3組み込みシステムプロジェクトです。より大きな"DELTA"テレメトリプロジェクトの一部として、レースカー用コックピット表示システム"VEGA"を実装しています。ドライバーにリアルタイム情報を提供し、セキュアMQTTを介してピットクルーにテレメトリデータを送信します。

## 開発環境

これはESP32-S3マイクロコントローラーを対象とするESP-IDF (Espressif IoT Development Framework) プロジェクトです。

**必要なツール:**
- ESP-IDF v5.1.5 または v5.1.6
- CMakeビルドシステム
- ハードウェア: ESP32-S3開発ボード

## よく使うコマンド

```bash
# プロジェクト設定 (menuconfigを開く)
idf.py menuconfig

# プロジェクトビルド
idf.py build

# ESP32-S3デバイスにファームウェアを書き込み
idf.py flash

# シリアル出力をモニタ (再起動/クラッシュログを含む)
idf.py monitor

# 書き込みとモニタを同時実行
idf.py flash monitor

# ビルド成果物をクリーン
idf.py clean

# 完全クリーン (sdkconfigとbuildディレクトリを削除)
idf.py fullclean
```

## アーキテクチャ概要

システムは3つの主要な状態を持つマルチタスクFreeRTOSアプリケーションとして動作します：
- **STATE_INITIALIZATION**: システム起動とタスク作成
- **STATE_STANDBY**: レース開始を待つ準備状態
- **STATE_RACING**: 完全なテレメトリを持つアクティブレーシングモード

### 主要コンポーネント

- **GPSモジュール** (`main/gps/`): 位置追跡用NMEAパーサー
- **速度モジュール** (`main/spd/`): パルスベースの速度計算
- **UIシステム** (`main/ui/`): LovyanGFXを使用したLCD表示管理
  - `io_board/`: 外部I/OボードのI2C制御
  - `lcd/`: ドライバー向けリアルタイム情報表示
- **MQTTクライアント** (`main/mqtt/`): AWS IoTへのセキュアテレメトリ送信
- **LovyanGFXコンポーネント** (`components/LovyanGFX/`): グラフィックスライブラリ

### システム通信

- **タスク間通信**: モジュール間でのFreeRTOSキュー
- **外部通信**: ピットクルーシステムへのWiFi経由セキュアMQTT
- **ハードウェアインターフェース**: I2C、SPI、UART、GPIO

## 重要ファイル

- `main/app_main.cpp`: アプリケーションエントリポイントと状態管理
- `sdkconfig`: ESP-IDFプロジェクト設定 (自動生成)
- `CMakeLists.txt`: 組み込みSSL証明書を含むビルド設定
- `credentials/`: セキュアMQTT用SSL/TLS証明書 (client.crt, client.key, AmazonRootCA1.pem)

## コードパターン

- ESP-IDF C APIを使用したC++アプリケーションコード
- FreeRTOSプリミティブを使用したタスクベースアーキテクチャ
- コンポーネントごとに分離されたヘッダー/実装のモジュラー設計
- スレッドセーフティのためのタスク間キューベースメッセージング
- ESP-IDFドライバーを通じたハードウェア抽象化

## テスト

この組み込みシステムは実際のESP32-S3ハードウェアを使用したハードウェア・イン・ザ・ループテストに依存しています。正式なユニットテストフレームワークは設定されていません。

## ブランチ構造

- **main**: 本番ブランチ
- **dev**: 開発統合ブランチ
- **feature/***: 機能開発ブランチ (現在: feature/refactoring)