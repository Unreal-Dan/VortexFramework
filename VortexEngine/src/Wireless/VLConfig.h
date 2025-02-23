#ifndef VL_CONFIG_H
#define VL_CONFIG_H

#include "../VortexConfig.h"

// Visible Light Enable
//
// Whether to enable the Visible Light system as a whole
//
#define VL_ENABLE_SENDER          1
#define VL_ENABLE_RECEIVER        0

// the size of IR blocks in bits
#define VL_DEFAULT_BLOCK_SIZE 256
#define VL_DEFAULT_BLOCK_SPACING MS_TO_TICKS(5)

// the max number of DWORDs that will transfer
#define VL_MAX_DWORDS_TRANSFER 128
#define VL_MAX_DATA_TRANSFER (VL_MAX_DWORDS_TRANSFER * sizeof(uint32_t))

// the IR receiver buffer size in dwords
#define VL_RECV_BUF_SIZE VL_MAX_DATA_TRANSFER

#define VL_THRESHOLD  0.5
#define VL_THRES_UP   (1 + VL_THRESHOLD)
#define VL_THRES_DOWN (1 - VL_THRESHOLD)

#define VL_TIMING (uint32_t)3230
#define VL_TIMING_MIN ((uint32_t)(VL_TIMING * VL_THRES_DOWN))

#define VL_HEADER_MARK (uint32_t)(VL_TIMING * 16)
#define VL_HEADER_SPACE (uint32_t)(VL_TIMING * 8)

#define VL_HEADER_MARK_MIN ((uint32_t)(VL_HEADER_MARK * VL_THRES_DOWN))
#define VL_HEADER_SPACE_MIN ((uint32_t)(VL_HEADER_SPACE * VL_THRES_DOWN))

#define VL_HEADER_MARK_MAX ((uint32_t)(VL_HEADER_MARK * VL_THRES_UP))
#define VL_HEADER_SPACE_MAX ((uint32_t)(VL_HEADER_SPACE * VL_THRES_UP))

#define VL_DIVIDER_SPACE VL_HEADER_MARK
#define VL_DIVIDER_SPACE_MIN VL_HEADER_MARK_MIN
#define VL_DIVIDER_SPACE_MAX VL_HEADER_MARK_MAX

#define VL_SEND_PWM_PIN 0
#define VL_RECEIVER_PIN 0

#endif
