#ifndef __DMA_DOUBLE_BUF_H__
#define __DMA_DOUBLE_BUF_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief DMA 传输完成/错误回调
 */
typedef void (*DMA_CompleteCallback)(void);
typedef void (*DMA_ErrorCallback)(void);

/**
 * @brief DMA 双缓冲状态
 */
typedef enum {
    DMA_STATE_READY = 0,     // 空闲，可启动新任务
    DMA_STATE_BUSY,          // 正在分块传输
    DMA_STATE_COMPLETE,      // 完整传输已结束
    DMA_STATE_ERROR          // 传输出错
} DMA_State_t;

/**
 * @brief 双缓冲控制器句柄（平台完全无关）
 */
typedef struct {
    /* 用户配置 */
    uint8_t *buffer1;
    uint8_t *buffer2;
    uint32_t buffer_size;
    DMA_CompleteCallback complete_cb;
    DMA_ErrorCallback error_cb;

    /* 内部状态 */
    volatile uint8_t *current_tx_buf;
    volatile uint8_t *next_fill_buf;
    volatile uint32_t total_remaining;
    volatile uint32_t data_ready;
    volatile DMA_State_t state;
    volatile bool buf1_ready;
    volatile bool buf2_ready;
    volatile uint32_t current_transfer_len;

} DMA_DoubleBuf_HandleTypeDef;

/* API */
void DMA_DoubleBuf_Init(DMA_DoubleBuf_HandleTypeDef *hdma,
                        uint8_t *buf1, uint8_t *buf2,
                        uint32_t buf_size,
                        DMA_CompleteCallback complete_cb,
                        DMA_ErrorCallback error_cb);

bool DMA_DoubleBuf_Start(DMA_DoubleBuf_HandleTypeDef *hdma,
                         uint32_t total_size);

bool DMA_DoubleBuf_WriteData(DMA_DoubleBuf_HandleTypeDef *hdma,
                             const uint8_t *data, uint32_t len);

uint32_t DMA_DoubleBuf_GetFreeSpace(DMA_DoubleBuf_HandleTypeDef *hdma);

void DMA_DoubleBuf_IRQHandler(DMA_DoubleBuf_HandleTypeDef *hdma);

DMA_State_t DMA_DoubleBuf_GetState(DMA_DoubleBuf_HandleTypeDef *hdma);

uint32_t DMA_DoubleBuf_GetRemaining(DMA_DoubleBuf_HandleTypeDef *hdma);

void DMA_DoubleBuf_Stop(DMA_DoubleBuf_HandleTypeDef *hdma);

#endif
