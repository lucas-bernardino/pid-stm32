################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (11.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/VL53L0X.c \
../Src/config_gpio.c \
../Src/main.c \
../Src/miros.c \
../Src/pid.c \
../Src/stm32.c \
../Src/stm32f1xx_hal_msp.c \
../Src/stm32f1xx_it.c \
../Src/syscalls.c \
../Src/sysmem.c \
../Src/system_stm32f1xx.c 

C_DEPS += \
./Src/VL53L0X.d \
./Src/config_gpio.d \
./Src/main.d \
./Src/miros.d \
./Src/pid.d \
./Src/stm32.d \
./Src/stm32f1xx_hal_msp.d \
./Src/stm32f1xx_it.d \
./Src/syscalls.d \
./Src/sysmem.d \
./Src/system_stm32f1xx.d 

OBJS += \
./Src/VL53L0X.o \
./Src/config_gpio.o \
./Src/main.o \
./Src/miros.o \
./Src/pid.o \
./Src/stm32.o \
./Src/stm32f1xx_hal_msp.o \
./Src/stm32f1xx_it.o \
./Src/syscalls.o \
./Src/sysmem.o \
./Src/system_stm32f1xx.o 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o Src/%.su Src/%.cyclo: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F1 -DSTM32F103C8Tx -c -I"C:/Users/Windows 10/Documents/GitHub/Kyros/str-miros-stm32f103-master/Drivers/STM32F1xx_HAL_Driver/Inc" -I"C:/Users/Windows 10/Documents/GitHub/Kyros/str-miros-stm32f103-master/Drivers/CMSIS/Device/ST/STM32F1xx/Include" -I"C:/Users/Windows 10/Documents/GitHub/Kyros/str-miros-stm32f103-master/Drivers/CMSIS/Include" -I"C:/Users/Windows 10/Documents/GitHub/Kyros/str-miros-stm32f103-master/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Src

clean-Src:
	-$(RM) ./Src/VL53L0X.cyclo ./Src/VL53L0X.d ./Src/VL53L0X.o ./Src/VL53L0X.su ./Src/config_gpio.cyclo ./Src/config_gpio.d ./Src/config_gpio.o ./Src/config_gpio.su ./Src/main.cyclo ./Src/main.d ./Src/main.o ./Src/main.su ./Src/miros.cyclo ./Src/miros.d ./Src/miros.o ./Src/miros.su ./Src/pid.cyclo ./Src/pid.d ./Src/pid.o ./Src/pid.su ./Src/stm32.cyclo ./Src/stm32.d ./Src/stm32.o ./Src/stm32.su ./Src/stm32f1xx_hal_msp.cyclo ./Src/stm32f1xx_hal_msp.d ./Src/stm32f1xx_hal_msp.o ./Src/stm32f1xx_hal_msp.su ./Src/stm32f1xx_it.cyclo ./Src/stm32f1xx_it.d ./Src/stm32f1xx_it.o ./Src/stm32f1xx_it.su ./Src/syscalls.cyclo ./Src/syscalls.d ./Src/syscalls.o ./Src/syscalls.su ./Src/sysmem.cyclo ./Src/sysmem.d ./Src/sysmem.o ./Src/sysmem.su ./Src/system_stm32f1xx.cyclo ./Src/system_stm32f1xx.d ./Src/system_stm32f1xx.o ./Src/system_stm32f1xx.su

.PHONY: clean-Src

