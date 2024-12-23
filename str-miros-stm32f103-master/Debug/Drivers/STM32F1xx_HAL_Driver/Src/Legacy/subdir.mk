################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/STM32F1xx_HAL_Driver/Src/Legacy/stm32f1xx_hal_can.c 

C_DEPS += \
./Drivers/STM32F1xx_HAL_Driver/Src/Legacy/stm32f1xx_hal_can.d 

OBJS += \
./Drivers/STM32F1xx_HAL_Driver/Src/Legacy/stm32f1xx_hal_can.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/STM32F1xx_HAL_Driver/Src/Legacy/%.o Drivers/STM32F1xx_HAL_Driver/Src/Legacy/%.su Drivers/STM32F1xx_HAL_Driver/Src/Legacy/%.cyclo: ../Drivers/STM32F1xx_HAL_Driver/Src/Legacy/%.c Drivers/STM32F1xx_HAL_Driver/Src/Legacy/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F1 -DSTM32F103C8Tx -c -I"C:/Users/Windows 10/Documents/GitHub/Kyros/str-miros-stm32f103-master/Drivers/STM32F1xx_HAL_Driver/Inc" -I"C:/Users/Windows 10/Documents/GitHub/Kyros/str-miros-stm32f103-master/Drivers/CMSIS/Device/ST/STM32F1xx/Include" -I"C:/Users/Windows 10/Documents/GitHub/Kyros/str-miros-stm32f103-master/Drivers/CMSIS/Include" -I"C:/Users/Windows 10/Documents/GitHub/Kyros/str-miros-stm32f103-master/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-STM32F1xx_HAL_Driver-2f-Src-2f-Legacy

clean-Drivers-2f-STM32F1xx_HAL_Driver-2f-Src-2f-Legacy:
	-$(RM) ./Drivers/STM32F1xx_HAL_Driver/Src/Legacy/stm32f1xx_hal_can.cyclo ./Drivers/STM32F1xx_HAL_Driver/Src/Legacy/stm32f1xx_hal_can.d ./Drivers/STM32F1xx_HAL_Driver/Src/Legacy/stm32f1xx_hal_can.o ./Drivers/STM32F1xx_HAL_Driver/Src/Legacy/stm32f1xx_hal_can.su

.PHONY: clean-Drivers-2f-STM32F1xx_HAL_Driver-2f-Src-2f-Legacy

