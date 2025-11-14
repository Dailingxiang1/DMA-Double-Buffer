################################################################################
# MRS Version: 2.3.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Debug/debug.c 

C_DEPS += \
./Debug/debug.d 

OBJS += \
./Debug/debug.o 

DIR_OBJS += \
./Debug/*.o \

DIR_DEPS += \
./Debug/*.d \

DIR_EXPANDS += \
./Debug/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Debug/%.o: ../Debug/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -g -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Debug" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Core" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/User" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Peripheral/inc" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Middleware" -I"d:/MyProject/mounriver-studio-projects/CH32V203C8T6/Middleware/DMA_DoubleBuffer" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

