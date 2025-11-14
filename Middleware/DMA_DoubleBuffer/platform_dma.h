#ifndef __PLATFORM_DMA_H__
#define __PLATFORM_DMA_H__

#include <stdint.h>
#include <stdbool.h>

// 平台DMA接口结构体
typedef struct {
    bool (*GetTCFlag)(void);                    // 获取传输完成标志
    void (*ClearTCFlag)(void);                  // 清除传输完成标志
    bool (*GetErrorFlag)(void);                 // 获取错误标志
    void (*ClearErrorFlag)(void);               // 清除错误标志
    uint32_t (*GetRemainingCount)(void);        // 获取剩余传输计数
    void (*StartTransfer)(uint8_t *data, uint32_t len); // 启动传输
    void (*StopTransfer)(void);                 // 停止传输
    bool (*IsBusy)(void);                       // 检查是否忙碌
} Platform_DMA_Interface;

// 全局平台接口实例
extern Platform_DMA_Interface platform_dma;

// 函数声明
void Platform_DMA_Init(void);
void Platform_DMA_InitInterface(Platform_DMA_Interface *iface);
void Platform_DMA_ModuleInit(void);
uint32_t Platform_DMA_GetTransferredCount(void);

#endif /* __PLATFORM_DMA_H__ */