/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2021/06/06
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *USART Print debugging routine:
 *USART1_Tx(PA9).
 *This example demonstrates using USART1(PA9) as a print debug port output.
 *
 */

#include "debug.h"
#include "dma_double_buf.h"
#include "platform_dma.h"
/* Global typedef */

/* Global define */

/* Global Variable */
DMA_DoubleBuf_HandleTypeDef g_dma_double_buf;

int intr_into_cnt = 0;
//8KB用于显示缓冲区
#define BUFFER_SIZE 4096
static uint8_t spi_dma_buf1[BUFFER_SIZE];
static uint8_t spi_dma_buf2[BUFFER_SIZE];

const uint8_t temp_buff[4096] = {1,2,3,4};
volatile int g_dma_cb_cnt = 0;

void my_dma_completeCallback();
void my_dma_errorCallback();
/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());
    printf("This is printf example\r\n");

    Platform_DMA_Init();  // ? 一定要用这个，包含函数指针绑定:

    DMA_DoubleBuf_Init(&g_dma_double_buf,
                       spi_dma_buf1,
                       spi_dma_buf2,
                       BUFFER_SIZE,
                       my_dma_completeCallback,
                       my_dma_errorCallback);

    // 先做一个 100 字节的最小实验
    if (DMA_DoubleBuf_Start(&g_dma_double_buf, 40960)) {
        printf("DMA transfer init successed, total_size =40KB\r\n");
    } else {
        printf("DMA transfer init failed\n");
    }

    /* 准备一个小块测试数据，反复塞入双缓冲 */
    uint8_t data_chunk[512];
    for (int i = 0; i < 512; i++) {
        data_chunk[i] = (uint8_t)i;   // 简单花样数据
    }

    uint32_t sent = 0;
   /* 持续往双缓冲里塞数据，直到把 TEST_TOTAL_SIZE 都喂完 */
    while (sent < 40960) {
        uint32_t free_space = DMA_DoubleBuf_GetFreeSpace(&g_dma_double_buf);
        if (free_space == 0) {
            // 当前没有空闲 buffer，DMA 还在跑，等一下再试
            continue;
        }

        uint32_t remain = 40960 - sent;
        uint32_t chunk  = 512;

        if (chunk > free_space) {
            chunk = free_space;
        }
        if (chunk > remain) {
            chunk = remain;
        }

        if (chunk == 0) {
            continue;
        }

        if (DMA_DoubleBuf_WriteData(&g_dma_double_buf, data_chunk, chunk)) {
            sent += chunk;
        } else {
            // 理论上不会到这里，打个调试信息
            printf("WriteData failed, free_space=%lu, remain=%lu\r\n",
                   (unsigned long)free_space,
                   (unsigned long)remain);
        }
    }

    printf("All data fed to double buffer, waiting for DMA to finish...\r\n");

    /* 主循环里只观察回调次数（DMA 完成） */
    int last = -1;

    while (1) {
        if (g_dma_cb_cnt != last) {
            last = g_dma_cb_cnt;
            printf("cb_cnt in main = %d (one full TEST_TOTAL_SIZE done)\r\n", g_dma_cb_cnt);
            printf("get into interrupt %d times\r\n",intr_into_cnt);
        }

        // 这里以后可以加别的逻辑，比如再次启动下一轮大容量传输
        // if (DMA_DoubleBuf_GetState(&g_dma_double_buf) == DMA_STATE_COMPLETE) { ... }
    }
}

void my_dma_completeCallback()
{
    g_dma_cb_cnt++;
}

void my_dma_errorCallback()
{
    printf("Dma Transfer Error\r\n");
}