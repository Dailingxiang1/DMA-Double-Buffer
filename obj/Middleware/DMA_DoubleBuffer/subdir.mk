################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middleware/DMA_DoubleBuffer/dma_double_buf.c \
../Middleware/DMA_DoubleBuffer/platform_dma.c 

C_DEPS += \
./Middleware/DMA_DoubleBuffer/dma_double_buf.d \
./Middleware/DMA_DoubleBuffer/platform_dma.d 

OBJS += \
./Middleware/DMA_DoubleBuffer/dma_double_buf.o \
./Middleware/DMA_DoubleBuffer/platform_dma.o 

DIR_OBJS += \
./Middleware/DMA_DoubleBuffer/*.o \

DIR_DEPS += \
./Middleware/DMA_DoubleBuffer/*.d \

DIR_EXPANDS += \
./Middleware/DMA_DoubleBuffer/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Middleware/DMA_DoubleBuffer/%.o: ../Middleware/DMA_DoubleBuffer/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Debug" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Core" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/User" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Peripheral/inc" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Middleware" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Middleware/DMA_DoubleBuffer" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

