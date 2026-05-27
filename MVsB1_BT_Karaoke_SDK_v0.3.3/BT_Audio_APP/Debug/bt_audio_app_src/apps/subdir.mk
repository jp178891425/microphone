################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../bt_audio_app_src/apps/browser_parallel.c \
../bt_audio_app_src/apps/browser_tree.c \
../bt_audio_app_src/apps/bt_hf_api.c \
../bt_audio_app_src/apps/bt_hf_mode.c \
../bt_audio_app_src/apps/bt_obex_upgrade.c \
../bt_audio_app_src/apps/bt_play_api.c \
../bt_audio_app_src/apps/bt_play_mode.c \
../bt_audio_app_src/apps/bt_record_api.c \
../bt_audio_app_src/apps/bt_record_mode.c \
../bt_audio_app_src/apps/bt_tws_app_func.c \
../bt_audio_app_src/apps/hdmi_in_api.c \
../bt_audio_app_src/apps/hdmi_in_mode.c \
../bt_audio_app_src/apps/i2s_api.c \
../bt_audio_app_src/apps/i2sin_mode.c \
../bt_audio_app_src/apps/line_api.c \
../bt_audio_app_src/apps/linein_mode.c \
../bt_audio_app_src/apps/main_task.c \
../bt_audio_app_src/apps/media_play_api.c \
../bt_audio_app_src/apps/media_play_mode.c \
../bt_audio_app_src/apps/mode_switch_api.c \
../bt_audio_app_src/apps/otg_device_audio.c \
../bt_audio_app_src/apps/radio_mode.c \
../bt_audio_app_src/apps/rest_mode.c \
../bt_audio_app_src/apps/spdif_mix_api.c \
../bt_audio_app_src/apps/spdif_mode.c \
../bt_audio_app_src/apps/tws_slave_mode.c \
../bt_audio_app_src/apps/usb_audio_api.c \
../bt_audio_app_src/apps/usb_audio_mode.c \
../bt_audio_app_src/apps/waiting_mode.c 

OBJS += \
./bt_audio_app_src/apps/browser_parallel.o \
./bt_audio_app_src/apps/browser_tree.o \
./bt_audio_app_src/apps/bt_hf_api.o \
./bt_audio_app_src/apps/bt_hf_mode.o \
./bt_audio_app_src/apps/bt_obex_upgrade.o \
./bt_audio_app_src/apps/bt_play_api.o \
./bt_audio_app_src/apps/bt_play_mode.o \
./bt_audio_app_src/apps/bt_record_api.o \
./bt_audio_app_src/apps/bt_record_mode.o \
./bt_audio_app_src/apps/bt_tws_app_func.o \
./bt_audio_app_src/apps/hdmi_in_api.o \
./bt_audio_app_src/apps/hdmi_in_mode.o \
./bt_audio_app_src/apps/i2s_api.o \
./bt_audio_app_src/apps/i2sin_mode.o \
./bt_audio_app_src/apps/line_api.o \
./bt_audio_app_src/apps/linein_mode.o \
./bt_audio_app_src/apps/main_task.o \
./bt_audio_app_src/apps/media_play_api.o \
./bt_audio_app_src/apps/media_play_mode.o \
./bt_audio_app_src/apps/mode_switch_api.o \
./bt_audio_app_src/apps/otg_device_audio.o \
./bt_audio_app_src/apps/radio_mode.o \
./bt_audio_app_src/apps/rest_mode.o \
./bt_audio_app_src/apps/spdif_mix_api.o \
./bt_audio_app_src/apps/spdif_mode.o \
./bt_audio_app_src/apps/tws_slave_mode.o \
./bt_audio_app_src/apps/usb_audio_api.o \
./bt_audio_app_src/apps/usb_audio_mode.o \
./bt_audio_app_src/apps/waiting_mode.o 

C_DEPS += \
./bt_audio_app_src/apps/browser_parallel.d \
./bt_audio_app_src/apps/browser_tree.d \
./bt_audio_app_src/apps/bt_hf_api.d \
./bt_audio_app_src/apps/bt_hf_mode.d \
./bt_audio_app_src/apps/bt_obex_upgrade.d \
./bt_audio_app_src/apps/bt_play_api.d \
./bt_audio_app_src/apps/bt_play_mode.d \
./bt_audio_app_src/apps/bt_record_api.d \
./bt_audio_app_src/apps/bt_record_mode.d \
./bt_audio_app_src/apps/bt_tws_app_func.d \
./bt_audio_app_src/apps/hdmi_in_api.d \
./bt_audio_app_src/apps/hdmi_in_mode.d \
./bt_audio_app_src/apps/i2s_api.d \
./bt_audio_app_src/apps/i2sin_mode.d \
./bt_audio_app_src/apps/line_api.d \
./bt_audio_app_src/apps/linein_mode.d \
./bt_audio_app_src/apps/main_task.d \
./bt_audio_app_src/apps/media_play_api.d \
./bt_audio_app_src/apps/media_play_mode.d \
./bt_audio_app_src/apps/mode_switch_api.d \
./bt_audio_app_src/apps/otg_device_audio.d \
./bt_audio_app_src/apps/radio_mode.d \
./bt_audio_app_src/apps/rest_mode.d \
./bt_audio_app_src/apps/spdif_mix_api.d \
./bt_audio_app_src/apps/spdif_mode.d \
./bt_audio_app_src/apps/tws_slave_mode.d \
./bt_audio_app_src/apps/usb_audio_api.d \
./bt_audio_app_src/apps/usb_audio_mode.d \
./bt_audio_app_src/apps/waiting_mode.d 


# Each subdirectory must supply rules for building sources it contributes
bt_audio_app_src/apps/%.o: ../bt_audio_app_src/apps/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Andes C Compiler'
	$(CROSS_COMPILE)gcc -DFUNC_OS_EN=1 -DCFG_APP_CONFIG -DHAVE_CONFIG_H -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/audio" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/driver/driver_api/inc/otg" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/rtc/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/fatfs/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/user/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/bluetooth/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/audio/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/driver/driver/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/driver/driver_api/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/mv_utils/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/lrc/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/rtos/rtos_api" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/rtos/freertos/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/flashfs/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/display" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/user" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/MVsB1_Base_SDK/middleware/cec/inc" -I"/cygdrive/D/2026_MV/microphone/MVsB1_BT_Karaoke_SDK_v0.3.3/BT_Audio_APP/bt_audio_app_src/ble" -O2 -mcmodel=medium -g3 -Wall -mcpu=d1088-spu -c -fmessage-length=0 -ldsp -mext-dsp -std=gnu99 -ffunction-sections -fdata-sections -mext-dsp -mext-zol -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d) $(@:%.o=%.o)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


