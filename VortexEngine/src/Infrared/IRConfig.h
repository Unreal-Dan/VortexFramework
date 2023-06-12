#ifndef IR_CONFIG_H
#define IR_CONFIG_H

// IR Enable
//
// Whether to enable the IR system as a whole
//
#define IR_ENABLE        1

// the size of IR blocks in bits
#define DEFAULT_IR_BLOCK_SIZE 256
#define DEFAULT_IR_BLOCK_SPACING Time::msToTicks(5)

// the max number of DWORDs that will transfer
#define MAX_DWORDS_TRANSFER 128
#define MAX_DATA_TRANSFER (MAX_DWORDS_TRANSFER * sizeof(uint32_t))

// the IR receiver buffer size in dwords
#define IR_RECV_BUF_SIZE MAX_DATA_TRANSFER

#define IR_THRESHOLD  0.5
#define THRES_UP      (1 + IR_THRESHOLD)
#define THRES_DOWN    (1 - IR_THRESHOLD)

#define IR_TIMING (uint32_t)2800
#define IR_TIMING_MIN ((uint32_t)(IR_TIMING * THRES_DOWN))
#define IR_TIMING_MAX ((uint32_t)(IR_TIMING * THRES_UP))

#define HEADER_MARK (uint32_t)(IR_TIMING * 16)
#define HEADER_SPACE (uint32_t)(IR_TIMING * 8)

#define HEADER_MARK_MIN ((uint32_t)(HEADER_MARK * THRES_DOWN))
#define HEADER_SPACE_MIN ((uint32_t)(HEADER_SPACE * THRES_DOWN))

#define HEADER_MARK_MAX ((uint32_t)(HEADER_MARK * THRES_UP))
#define HEADER_SPACE_MAX ((uint32_t)(HEADER_SPACE * THRES_UP))

#define DIVIDER_SPACE HEADER_MARK
#define DIVIDER_SPACE_MIN HEADER_MARK_MIN
#define DIVIDER_SPACE_MAX HEADER_MARK_MAX

#define IR_SEND_PWM_PIN 0
#define RECEIVER_PIN 2

#endif
