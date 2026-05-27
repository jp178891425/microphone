################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../bt_audio_app_src/ble/ble_app_func.c 

OBJS += \
./bt_audio_app_src/ble/ble_app_func.o 

C_DEPS += \
./bt_audio_app_src/ble/ble_app_func.d 


# Each subdirectory must supply rules for building sources it contributes
bt_audio_app_src/ble/%.o: ../bt_audio_app_src/ble/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DCFG_APP_CONFIG -DHAVE_CONFIG_H -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/audio" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/driver/driver_api/inc/otg" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/rtc/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/fatfs/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/user/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/bluetooth/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/audio/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/driver/driver/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/driver/driver_api/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/mv_utils/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/lrc/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/rtos/rtos_api" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/rtos/freertos/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/flashfs/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/display" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/user" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/cec/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/ble" -O2 -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -ffunction-sections -fdata-sections -mext-dsp -mext-zol -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


