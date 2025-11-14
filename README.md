# DMA Double Buffer Framework (Portable & Lightweight)

本仓库存放一个 **可移植的 DMA 双缓冲抽象框架**，适用于任意 MCU（RISC-V / ARM / ESP / C-Sky 等）。  
框架完全 **平台无关**，通过 `platform_dma.c/.h` 将底层 DMA 实现抽象化，只需重写该文件即可完成移植。

示例工程基于 **WCH CH32V203**，但核心框架可直接运行于其他 MCU 平台。

---

## ✨ 特性

- 双缓冲 DMA 自动调度（Double Buffer DMA）
- 自动分块传输（解决 DMA 单次传输计数限制，例如 16-bit 计数器）
- 中断驱动，完全非阻塞
- 平台无关，只依赖抽象接口 `Platform_DMA_Interface`
- API 简洁，易于移植与维护
- 适用于 SPI LCD 刷屏、高速数据流、音频推送等场景

---

## 📁 文件结构

```txt
├── dma_double_buf.c       # 双缓冲调度核心（平台无关）
├── dma_double_buf.h
├── platform_dma.c         # 平台适配层（需按 MCU 修改）
├── platform_dma.h
├── ch32v20x_it.c          # CH32 平台 DMA 中断示例
└── main.c                 # 使用示例（Demo）
```
## 🚀 使用方法
### 1️⃣ 初始化平台 DMA 层
在你的启动代码里，先调用：
```bash
Platform_DMA_Init();
```
该函数在 platform_dma.c 中实现，用于：

- 开启 DMA/SPI 时钟

- 初始化 DMA 通道

- 绑定 platform_dma 接口函数指针
### 2️⃣ 初始化双缓冲控制器
```bash
DMA_DoubleBuf_HandleTypeDef g_dma;
uint8_t buf1[BUF_SIZE];
uint8_t buf2[BUF_SIZE];

DMA_DoubleBuf_Init(&g_dma,
                   buf1,
                   buf2,
                   BUF_SIZE,
                   on_dma_complete,
                   on_dma_error);

```
参数说明：

- buf1 / buf2：两个等长的内存缓冲区

- BUF_SIZE：每个 buffer 的大小，须 ≤ 平台 DMA 单次最大传输计数（如 65535）

- on_dma_complete：完整传输结束回调（所有数据发送完）

- on_dma_error：错误回调（可选）
### 3️⃣ 启动一次完整传输任务
```bash
uint32_t total_size = 4096 * 20; // 举例：总共发送 80KB 数据

DMA_DoubleBuf_Start(&g_dma, total_size);
```
- total_size：计划本次要通过 DMA 发送的总字节数

- 实际发送时会自动按 BUF_SIZE 分块 DMA 传输
### 4️⃣ 在主循环中不断填充数据
```bash
uint32_t sent = 0;
uint8_t data_chunk[CHUNK_SIZE]; // 比如 512 字节

// 填充测试数据
for (int i = 0; i < CHUNK_SIZE; i++) {
    data_chunk[i] = (uint8_t)i;
}

while (sent < total_size) {
    uint32_t free_space = DMA_DoubleBuf_GetFreeSpace(&g_dma);
    if (free_space == 0) {
        // 当前没有空闲 buffer，DMA 正在使用；稍后再试
        continue;
    }

    uint32_t remain = total_size - sent;
    uint32_t chunk  = CHUNK_SIZE;

    if (chunk > free_space) chunk = free_space;
    if (chunk > remain)     chunk = remain;

    if (chunk == 0) continue;

    if (DMA_DoubleBuf_WriteData(&g_dma, data_chunk, chunk)) {
        sent += chunk;
    }
}

```
在这个过程中：

- 当当前填充的 buffer 满了，或已经写满 total_size 对应的数据：

- - 会自动标记该 buffer 为“就绪”

- - 如果底层 DMA 空闲，则立即启动该 buffer 的 DMA 传输

- 你只需要不断调用 DMA_DoubleBuf_WriteData 填数据即可，无需关心每一块什么时候发

### 5️⃣ 在 DMA 中断里调用调度函数
在对应的 DMA 中断服务函数里调用：
```bash
extern DMA_DoubleBuf_HandleTypeDef g_dma;

void DMA1_Channel3_IRQHandler(void)
{
    DMA_DoubleBuf_IRQHandler(&g_dma);
}
```
DMA_DoubleBuf_IRQHandler 会：

- 检测 TC（传输完成）/错误 标志

- 递减 total_remaining

- 启动下一块 DMA 传输（如果还有剩余）

- 最后一块发送完毕时调用 complete_cb()

### 6️⃣ 在主循环里检测完成状态（可选）
```bash
if (DMA_DoubleBuf_GetState(&g_dma) == DMA_STATE_COMPLETE) {
    // 本次 total_size 对应的数据全部发送完毕
}
```
或使用回调计数：
```bash
volatile int g_dma_cb_cnt = 0;

void on_dma_complete(void)
{
    g_dma_cb_cnt++;
}
```
---
## ⏱ 工作原理（简述）
应用层通过 DMA_DoubleBuf_WriteData 往双缓冲（buffer1 / buffer2）写数据

当某个 buffer 填满或达到 total_size 剩余上限时：

- 标记为“就绪”

- 如果当前无 DMA 在发，则立刻调用 platform_dma.StartTransfer 开始 DMA

DMA 完成中断（TC）触发：

- 调用 DMA_DoubleBuf_IRQHandler

- 减少剩余字节数 total_remaining

- 如果还有剩余数据，取下一个 ready buffer 再发

- 如果 total_remaining == 0，则：

- - 状态置为 DMA_STATE_COMPLETE

- - 调用 complete_cb 通知应用层
---
## 🔧 移植指南（platform_dma.c/.h）
双缓冲框架不直接访问硬件寄存器，而是通过 Platform_DMA_Interface 抽象访问。
你需要在 platform_dma.c/.h 中实现以下接口（不同 MCU 只需重写这里）：
```bash
typedef struct {
    bool     (*GetTCFlag)(void);          // DMA 完成中断标志
    void     (*ClearTCFlag)(void);        // 清除完成标志
    bool     (*GetErrorFlag)(void);       // DMA 错误标志（可选）
    void     (*ClearErrorFlag)(void);     // 清除错误标志（可选）
    uint32_t (*GetRemainingCount)(void);  // 硬件剩余计数（可选）
    void     (*StartTransfer)(uint8_t *data, uint32_t len);  // 启动一次 DMA 传输
    void     (*StopTransfer)(void);       // 停止 DMA
    bool     (*IsBusy)(void);             // DMA 当前是否忙
} Platform_DMA_Interface;

extern Platform_DMA_Interface platform_dma;
```
典型实现流程（伪代码）：
```bash
void Platform_DMA_Init(void)
{
    // 1. 开启 DMA/SPI 时钟
    // 2. 初始化 DMA 通道寄存器
    // 3. 配置 DMA 中断（NVIC）
    // 4. 绑定 platform_dma 接口函数指针
}
```
StartTransfer(data, len) 内部一般做：

- 配置 DMA 源地址 = data

- 配置 DMA 传输长度 = len

- 使能 DMA 通道

- 打开 TC/TE 中断

GetTCFlag / ClearTCFlag 内部一般使用厂商库函数（如 CH32 的 DMA_GetITStatus / DMA_ClearITPendingBit）。

这样，整个双缓冲框架就可在任何平台上运行，而无需修改上层逻辑。

---
## 📘 简化 Demo（main.c 思路）
```bash
int main(void)
{
    System_Init();        // 时钟 / SysTick / 串口等
    Platform_DMA_Init();  // DMA & SPI & NVIC

    static uint8_t buf1[4096];
    static uint8_t buf2[4096];

    DMA_DoubleBuf_Init(&g_dma,
                       buf1,
                       buf2,
                       sizeof(buf1),
                       on_dma_complete,
                       on_dma_error);

    uint32_t total_size = sizeof(buf1) * 10; // 比如 40KB
    DMA_DoubleBuf_Start(&g_dma, total_size);

    uint8_t chunk[512];
    for (int i = 0; i < sizeof(chunk); i++) {
        chunk[i] = (uint8_t)i;
    }

    uint32_t sent = 0;
    while (sent < total_size) {
        uint32_t free = DMA_DoubleBuf_GetFreeSpace(&g_dma);
        if (free == 0) continue;

        uint32_t remain = total_size - sent;
        uint32_t len = sizeof(chunk);
        if (len > free)   len = free;
        if (len > remain) len = remain;

        DMA_DoubleBuf_WriteData(&g_dma, chunk, len);
        sent += len;
    }

    while (DMA_DoubleBuf_GetState(&g_dma) != DMA_STATE_COMPLETE) {
        // 可以在这里做其他任务
    }

    for (;;);
}
```
---
## 👍 适用场景
- SPI LCD 屏幕刷新（高分辨率 / 高帧率）

- 频输出（I2S / DAC 连续推流）

- 串口大数据发送（Log、上位机协议）

- 摄像头数据流 / FFT 数据推送

- 任意“源数据量 >> 单次 DMA 计数”的场景

## 💬 备注
- 中断中不建议使用 printf，容易造成栈溢出或输出异常（建议只改标志，主循环中打印）

- buffer_size 必须 ≤ 底层 DMA 单次最大传输长度（多数 MCU 为 65535）

- total_size 可以远大于 buffer_size，框架会自动多次分块发送