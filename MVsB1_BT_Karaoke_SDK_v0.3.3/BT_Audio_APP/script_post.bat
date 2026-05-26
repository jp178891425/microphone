@echo off
if exist output\main_merge.mva (del output\main_merge.mva)
grep '^#define CFG_RES_REMIND_SOUND_EN' ..\bt_audio_app_src\inc\app_config.h | grep '//' && ..\..\MVsB1_Base_SDK\tools\merge_script\Andes_MVAGenerate.exe -s 0  -e 1 || ..\..\MVsB1_Base_SDK\tools\merge_script\Andes_MVAGenerate.exe -s 1  -e 1
if not exist output\BT_Audio_APP.bin (del output\main_merge.mva)
pause

