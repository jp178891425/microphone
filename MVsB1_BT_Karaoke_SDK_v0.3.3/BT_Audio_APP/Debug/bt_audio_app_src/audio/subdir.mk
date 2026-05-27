################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../bt_audio_app_src/audio/audio_adjust.c \
../bt_audio_app_src/audio/audio_aec.c \
../bt_audio_app_src/audio/audio_common.c \
../bt_audio_app_src/audio/audio_effect.c \
../bt_audio_app_src/audio/audio_effect_class.c \
../bt_audio_app_src/audio/audio_effect_mix_process.c \
../bt_audio_app_src/audio/audio_effect_process.c \
../bt_audio_app_src/audio/audio_vol.c \
../bt_audio_app_src/audio/beep.c \
../bt_audio_app_src/audio/communication.c \
../bt_audio_app_src/audio/ctrlvars.c \
../bt_audio_app_src/audio/eq_params.c 

OBJS += \
./bt_audio_app_src/audio/audio_adjust.o \
./bt_audio_app_src/audio/audio_aec.o \
./bt_audio_app_src/audio/audio_common.o \
./bt_audio_app_src/audio/audio_effect.o \
./bt_audio_app_src/audio/audio_effect_class.o \
./bt_audio_app_src/audio/audio_effect_mix_process.o \
./bt_audio_app_src/audio/audio_effect_process.o \
./bt_audio_app_src/audio/audio_vol.o \
./bt_audio_app_src/audio/beep.o \
./bt_audio_app_src/audio/communication.o \
./bt_audio_app_src/audio/ctrlvars.o \
./bt_audio_app_src/audio/eq_params.o 

C_DEPS += \
./bt_audio_app_src/audio/audio_adjust.d \
./bt_audio_app_src/audio/audio_aec.d \
./bt_audio_app_src/audio/audio_common.d \
./bt_audio_app_src/audio/audio_effect.d \
./bt_audio_app_src/audio/audio_effect_class.d \
./bt_audio_app_src/audio/audio_effect_mix_process.d \
./bt_audio_app_src/audio/audio_effect_process.d \
./bt_audio_app_src/audio/audio_vol.d \
./bt_audio_app_src/audio/beep.d \
./bt_audio_app_src/audio/communication.d \
./bt_audio_app_src/audio/ctrlvars.d \
./bt_audio_app_src/audio/eq_params.d 


# Each subdirectory must supply rules for building sources it contributes
bt_audio_app_src/audio/%.o: ../bt_audio_app_src/audio/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DCFG_APP_CONFIG -DHAVE_CONFIG_H -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/audio" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/driver/driver_api/inc/otg" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/rtc/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/fatfs/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/user/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/bluetooth/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/audio/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/driver/driver/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/driver/driver_api/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/mv_utils/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/lrc/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/rtos/rtos_api" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/rtos/freertos/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/flashfs/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/display" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/user" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/cec/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/ble" -O2 -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -ffunction-sections -fdata-sections -mext-dsp -mext-zol -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


