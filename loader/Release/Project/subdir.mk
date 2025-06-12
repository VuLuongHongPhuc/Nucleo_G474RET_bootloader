################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Project/com_task.c \
../Project/flash.c 

OBJS += \
./Project/com_task.o \
./Project/flash.o 

C_DEPS += \
./Project/com_task.d \
./Project/flash.d 


# Each subdirectory must supply rules for building sources it contributes
Project/%.o Project/%.su Project/%.cyclo: ../Project/%.c Project/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32G474xx -c -I../Core/Inc -I../Project -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Project

clean-Project:
	-$(RM) ./Project/com_task.cyclo ./Project/com_task.d ./Project/com_task.o ./Project/com_task.su ./Project/flash.cyclo ./Project/flash.d ./Project/flash.o ./Project/flash.su

.PHONY: clean-Project

