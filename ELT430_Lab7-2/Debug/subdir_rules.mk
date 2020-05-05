################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/opt/ti/compiler/ti-cgt-c2000_18.12.5.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla0 --float_support=softlib --vcu_support=vcu0 --include_path="/home/dinera/ti/workspace_v8/ELT430_Lab7-2" --include_path="/home/dinera/ti/C2000Ware_2_01_00_00_Software/device_support/f2806x/headers/include" --include_path="/home/dinera/ti/C2000Ware_2_01_00_00_Software/device_support/f2806x/common/include" --include_path="/opt/ti/compiler/ti-cgt-c2000_18.12.5.LTS/include" -g --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


