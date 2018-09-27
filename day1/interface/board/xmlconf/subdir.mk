################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../xmlconf/xmlconf.cpp 

OBJS += \
./xmlconf/xmlconf.o 

CPP_DEPS += \
./xmlconf/xmlconf.d 


# Each subdirectory must supply rules for building sources it contributes
xmlconf/%.o: ../xmlconf/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	/home/zekunyao/armgcc/bin/arm-vfp-linux-gnu-g++ -I"/home/zekunyao/Projects/corsinterface/headers" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


