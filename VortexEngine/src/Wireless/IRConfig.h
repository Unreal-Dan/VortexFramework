#ifndef IR_CONFIG_H
#define IR_CONFIG_H

// Infrared Enable
//
// Whether to enable the Infrared system as a whole
//
#define IR_ENABLE_SENDER          1
#define IR_ENABLE_RECEIVER        1

// the size of IR blocks in bits
#define IR_DEFAULT_BLOCK_SIZE 32
#define IR_DEFAULT_BLOCK_SPACING MS_TO_TICKS(300)

// the max number of DWORDs that will transfer
#define IR_MAX_DWORDS_TRANSFER 1024
#define IR_MAX_DATA_TRANSFER (IR_MAX_DWORDS_TRANSFER * sizeof(uint32_t))

// the IR receiver buffer size in dwords
#define IR_RECV_BUF_SIZE IR_MAX_DATA_TRANSFER

#define IR_THRESHOLD  0.25
#define IR_THRES_UP   (1 + IR_THRESHOLD)
#define IR_THRES_DOWN (1 - IR_THRESHOLD)

#define IR_TIMING (uint32_t)562
#define IR_TIMING_MIN ((uint32_t)(IR_TIMING * IR_THRES_DOWN))

#define IR_HEADER_MARK (uint32_t)(IR_TIMING * 16)
#define IR_HEADER_SPACE (uint32_t)(IR_TIMING * 8)

#define IR_HEADER_MARK_MIN ((uint32_t)(IR_HEADER_MARK * IR_THRES_DOWN))
#define IR_HEADER_SPACE_MIN ((uint32_t)(IR_HEADER_SPACE * IR_THRES_DOWN))

#define IR_HEADER_MARK_MAX ((uint32_t)(IR_HEADER_MARK * IR_THRES_UP))
#define IR_HEADER_SPACE_MAX ((uint32_t)(IR_HEADER_SPACE * IR_THRES_UP))

#define IR_DIVIDER_SPACE IR_HEADER_MARK
#define IR_DIVIDER_SPACE_MIN IR_HEADER_MARK_MIN
#define IR_DIVIDER_SPACE_MAX IR_HEADER_MARK_MAX

#define IR_SEND_PWM_PIN 3
#define IR_RECEIVER_PIN 4

#endif
