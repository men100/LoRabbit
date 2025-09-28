#pragma once

// 通信履歴設定
#define LORABBIT_HISTORY_SIZE        32

// TP 機能利用時のリトライ回数
#define LORABBIT_TP_RETRY_COUNT      3
// TP 機能利用時のタイムアウト時間 (msec)
#define LORABBIT_TP_ACK_TIMEOUT_MS   2000

// 割り込み機能の有効/無効
// #define LORABBIT_USE_AUX_IRQ

// デバッグモードの有効/無効
// #define LORABBIT_DEBUG_MODE

// AI モデル推論による ADR の有効/無効
// #define LORABBIT_USE_AI_ADR
