#include "dma_double_buf.h"
#include "platform_dma.h"
#include <string.h>

static void DMA_DoubleBuf_StartNextTransfer(DMA_DoubleBuf_HandleTypeDef *hdma);

/**
 * @brief 初始化双缓冲管理器
 */
void DMA_DoubleBuf_Init(DMA_DoubleBuf_HandleTypeDef *hdma,
                        uint8_t *buf1, uint8_t *buf2,
                        uint32_t buf_size,
                        DMA_CompleteCallback complete_cb,
                        DMA_ErrorCallback error_cb)
{
    hdma->buffer1 = buf1;
    hdma->buffer2 = buf2;
    hdma->buffer_size = buf_size;
    hdma->complete_cb = complete_cb;
    hdma->error_cb = error_cb;

    /* 内部状态 */
    hdma->current_tx_buf = NULL;
    hdma->next_fill_buf = buf1;
    hdma->total_remaining = 0;
    hdma->data_ready = 0;
    hdma->state = DMA_STATE_READY;
    hdma->buf1_ready = false;
    hdma->buf2_ready = false;
    hdma->current_transfer_len = 0;

    hdma->total_size   = 0;
    hdma->total_filled = 0;
}

/**
 * @brief 启动一次完整双缓冲大传输
 */
bool DMA_DoubleBuf_Start(DMA_DoubleBuf_HandleTypeDef *hdma, uint32_t total_size)
{
    if (hdma->state == DMA_STATE_BUSY)
        return false;

    hdma->total_size      = total_size;   // ★ 新增
    hdma->total_remaining = total_size;
    hdma->total_filled    = 0;            // ★ 新增

    hdma->data_ready      = 0;
    hdma->state           = DMA_STATE_BUSY;
    hdma->buf1_ready      = false;
    hdma->buf2_ready      = false;
    hdma->next_fill_buf   = hdma->buffer1;
    hdma->current_tx_buf  = NULL;
    hdma->current_transfer_len = 0;

    return true;
}

/**
 * @brief 写入数据到下一个缓冲区
 */
bool DMA_DoubleBuf_WriteData(DMA_DoubleBuf_HandleTypeDef *hdma,
                             const uint8_t *data, uint32_t len)
{
    if (hdma->state != DMA_STATE_BUSY)
        return false;

    /* 1. 还允许写多少（全局上限） */
    uint32_t can_still_accept = 0;
    if (hdma->total_size >= hdma->total_filled)
        can_still_accept = hdma->total_size - hdma->total_filled;
    else
        return false;  // 理论上不该出现

    if (can_still_accept == 0)
        return false;  // 已经写满了，不再接受新数据

    /* 2. 当前缓冲区还能写多少（局部上限） */
    uint32_t free_space = DMA_DoubleBuf_GetFreeSpace(hdma);
    if (free_space == 0)
        return false;

    uint32_t copy_len = len;
    if (copy_len > free_space)
        copy_len = free_space;
    if (copy_len > can_still_accept)
        copy_len = can_still_accept;

    if (copy_len == 0)
        return false;

    /* 3. 把数据拷进 next_fill_buf */
    memcpy((void *)&hdma->next_fill_buf[hdma->data_ready], data, copy_len);

    hdma->data_ready    += copy_len;
    hdma->total_filled  += copy_len;   // ★ 记录这次任务一共写了多少

    /* 4. 判断是否应该把当前缓冲区标记为 ready：
          - buffer 写满了，或者
          - 这次任务所有数据都已经写完了（最后一块） */
    if (hdma->data_ready >= hdma->buffer_size ||
        hdma->total_filled == hdma->total_size)
    {
        if (hdma->next_fill_buf == hdma->buffer1) {
            hdma->buf1_ready = true;
            hdma->next_fill_buf = hdma->buffer2;
        } else {
            hdma->buf2_ready = true;
            hdma->next_fill_buf = hdma->buffer1;
        }

        hdma->data_ready = 0;

        bool dma_busy = (platform_dma.IsBusy) ? platform_dma.IsBusy() : false;

        if (hdma->current_tx_buf == NULL && !dma_busy) {
            DMA_DoubleBuf_StartNextTransfer(hdma);
        }
    }

    return true;
}

/**
 * @brief 获取当前可写空间
 */
uint32_t DMA_DoubleBuf_GetFreeSpace(DMA_DoubleBuf_HandleTypeDef *hdma)
{
    if (hdma->state != DMA_STATE_BUSY)
        return 0;

    if (hdma->next_fill_buf == hdma->current_tx_buf)
        return 0;

    return hdma->buffer_size - hdma->data_ready;
}

/**
 * @brief 启动下一块 DMA
 */
static void DMA_DoubleBuf_StartNextTransfer(DMA_DoubleBuf_HandleTypeDef *hdma)
{
    uint8_t *next = NULL;

    if (hdma->buf1_ready && hdma->current_tx_buf != hdma->buffer1) {
        next = hdma->buffer1;
        hdma->buf1_ready = false;
    } else if (hdma->buf2_ready && hdma->current_tx_buf != hdma->buffer2) {
        next = hdma->buffer2;
        hdma->buf2_ready = false;
    }

    if (!next)
        return;

    uint32_t len = (hdma->total_remaining > hdma->buffer_size)
                    ? hdma->buffer_size
                    : hdma->total_remaining;

    hdma->current_tx_buf = next;
    hdma->current_transfer_len = len;

    if (platform_dma.StartTransfer)
        platform_dma.StartTransfer(next, len);
}

/**
 * @brief DMA 中断处理（平台中断调用）
 */
void DMA_DoubleBuf_IRQHandler(DMA_DoubleBuf_HandleTypeDef *hdma)
{
    /* 清残余中断 */
    if (hdma->state != DMA_STATE_BUSY) {
        if (platform_dma.GetTCFlag && platform_dma.GetTCFlag())
            platform_dma.ClearTCFlag();
        if (platform_dma.GetErrorFlag && platform_dma.GetErrorFlag())
            platform_dma.ClearErrorFlag();
        return;
    }

    /* 完成中断 */
    if (platform_dma.GetTCFlag && platform_dma.GetTCFlag()) {

        platform_dma.ClearTCFlag();

        uint32_t done = hdma->current_transfer_len;
        if (done > hdma->total_remaining)
            done = hdma->total_remaining;

        hdma->total_remaining -= done;

        hdma->current_tx_buf = NULL;
        hdma->current_transfer_len = 0;

        if (hdma->total_remaining == 0) {

            hdma->state = DMA_STATE_COMPLETE;
            if (platform_dma.StopTransfer)
                platform_dma.StopTransfer();

            if (hdma->complete_cb)
                hdma->complete_cb();

        } else {
            DMA_DoubleBuf_StartNextTransfer(hdma);
        }
    }

    /* 错误中断 */
    if (platform_dma.GetErrorFlag && platform_dma.GetErrorFlag()) {
        platform_dma.ClearErrorFlag();
        hdma->state = DMA_STATE_ERROR;

        if (platform_dma.StopTransfer)
            platform_dma.StopTransfer();
        if (hdma->error_cb)
            hdma->error_cb();
    }
}

DMA_State_t DMA_DoubleBuf_GetState(DMA_DoubleBuf_HandleTypeDef *hdma)
{
    return hdma->state;
}

uint32_t DMA_DoubleBuf_GetRemaining(DMA_DoubleBuf_HandleTypeDef *hdma)
{
    return hdma->total_remaining;
}

void DMA_DoubleBuf_Stop(DMA_DoubleBuf_HandleTypeDef *hdma)
{
    if (platform_dma.StopTransfer)
        platform_dma.StopTransfer();

    hdma->state = DMA_STATE_READY;
    hdma->total_remaining = 0;
    hdma->current_tx_buf  = NULL;
    hdma->buf1_ready      = false;
    hdma->buf2_ready      = false;
    hdma->data_ready      = 0;

    hdma->total_size      = 0;
    hdma->total_filled    = 0;
}