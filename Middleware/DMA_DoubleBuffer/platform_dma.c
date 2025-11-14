#include "platform_dma.h"
#include "ch32v20x_dma.h"
#include "ch32v20x_rcc.h"
#include "ch32v20x_spi.h"

/*------------------------------------------------------------
 * CH32V203 平台 DMA 绑定的全局接口实例
 *------------------------------------------------------------*/
Platform_DMA_Interface platform_dma;

/*------------------------------------------------------------
 * 用户可根据实际外设修改以下定义
 *------------------------------------------------------------*/

/* SPI1 → 默认 TX DMA 通道：DMA1 Channel 3 */
#define DMA_TX_CH       DMA1_Channel3
#define DMA_TC_FLAG     DMA1_FLAG_TC3
#define DMA_TE_FLAG     DMA1_FLAG_TE3
#define DMA_TC_IT       DMA1_IT_TC3
#define DMA_TE_IT       DMA1_IT_TE3

/* SPI1 data register address */
#define SPIx            SPI1
#define SPIx_DR_Address (uint32_t)(&SPI1->DATAR)

/* 记录当前一次 DMA 的长度（用于剩余计算）*/
static uint32_t g_dma_current_len = 0;

/*------------------------------------------------------------
 * 下面是平台实现函数
 *------------------------------------------------------------*/

static bool CH32_DMA_GetTCFlag(void)
{
    return (DMA_GetITStatus(DMA_TC_IT) == SET);
}

static void CH32_DMA_ClearTCFlag(void)
{
    DMA_ClearITPendingBit(DMA_TC_IT);
}

static bool CH32_DMA_GetErrorFlag(void)
{
    return (DMA_GetITStatus(DMA_TE_IT) == SET);
}

static void CH32_DMA_ClearErrorFlag(void)
{
    DMA_ClearITPendingBit(DMA_TE_IT);
}
/** 获取 DMA 剩余计数 */
static uint32_t CH32_DMA_GetRemainingCount(void)
{
    return DMA_GetCurrDataCounter(DMA_TX_CH);
}

/** 判断 DMA 通道是否 busy */
static bool CH32_DMA_IsBusy(void)
{
    return (DMA_TX_CH->CFGR & DMA_CFGR1_EN) != 0;
}

/** 停止 DMA */
static void CH32_DMA_StopTransfer(void)
{
    DMA_Cmd(DMA_TX_CH, DISABLE);
    SPI_I2S_DMACmd(SPIx, SPI_I2S_DMAReq_Tx, DISABLE);
}

/** 启动一次 DMA 传输 */
static void CH32_DMA_StartTransfer(uint8_t *data, uint32_t len)
{
    g_dma_current_len = len;

    /* 1. 禁止 DMA 通道 */
    DMA_Cmd(DMA_TX_CH, DISABLE);

    /* 2. 初始化 DMA */
    DMA_InitTypeDef DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure);

    DMA_InitStructure.DMA_PeripheralBaseAddr = SPIx_DR_Address;
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)data;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize         = len;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;

    DMA_Init(DMA_TX_CH, &DMA_InitStructure);

    /* 清除旧中断 */
    DMA_ClearITPendingBit(DMA_TC_IT | DMA_TE_IT);
    
    DMA_ClearFlag(DMA_TC_FLAG | DMA_TE_FLAG);

    /* 开启 DMA 完成/错误中断 */
    DMA_ITConfig(DMA_TX_CH, DMA_IT_TC | DMA_IT_TE, ENABLE);

    /* 3. 启动 DMA 通道 */
    SPI_I2S_DMACmd(SPIx, SPI_I2S_DMAReq_Tx, ENABLE);
    DMA_Cmd(DMA_TX_CH, ENABLE);
}

/** 获取本次传输已经完成的字节（上层不会直接用）*/
uint32_t Platform_DMA_GetTransferredCount(void)
{
    uint32_t remain = CH32_DMA_GetRemainingCount();
    if (remain > g_dma_current_len)
        remain = 0;

    return g_dma_current_len - remain;
}

/*------------------------------------------------------------
 * DMA 模块初始化（时钟 + SPI1 + DMA 通道）
 *------------------------------------------------------------*/
void Platform_DMA_ModuleInit(void)
{
    /* RCC */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB , ENABLE);
    /* 清除并关闭 DMA 通道 */
    DMA_DeInit(DMA_TX_CH);
    /* GPIO配置 */
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    //SCL
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    //MOSI
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    //BLK
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    //RST
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    //RS
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    //CS
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    /* SPI1 配置（你可按需求改为自己的 SPI 初始化） */
    SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
    SPI_Init(SPIx, &SPI_InitStructure);

    SPI_Cmd(SPIx, ENABLE);

    // 使能NVIC中断
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*------------------------------------------------------------
 * 绑定接口函数
 *------------------------------------------------------------*/
void Platform_DMA_InitInterface(Platform_DMA_Interface *iface)
{
    iface->GetTCFlag         = CH32_DMA_GetTCFlag;
    iface->ClearTCFlag       = CH32_DMA_ClearTCFlag;
    iface->GetErrorFlag      = CH32_DMA_GetErrorFlag;
    iface->ClearErrorFlag    = CH32_DMA_ClearErrorFlag;
    iface->GetRemainingCount = CH32_DMA_GetRemainingCount;
    iface->StartTransfer     = CH32_DMA_StartTransfer;
    iface->StopTransfer      = CH32_DMA_StopTransfer;
    iface->IsBusy            = CH32_DMA_IsBusy;
}

/*------------------------------------------------------------
 * 用户在 main() 里调用：初始化 DMA 平台层
 *------------------------------------------------------------*/
void Platform_DMA_Init(void)
{
    Platform_DMA_ModuleInit();
    Platform_DMA_InitInterface(&platform_dma);
}


