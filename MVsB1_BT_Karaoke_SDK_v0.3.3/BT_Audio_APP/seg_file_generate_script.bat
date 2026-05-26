copy  ..\res\MV_SDKConfig_ap10.ini MV_SDKConfig_ap10.ini

copy  ..\res\MV_SDKConfig_bp10.ini MV_SDKConfig_bp10.ini

copy  ..\res\interrupt_ap10.h interrupt_ap10.h

copy  ..\res\interrupt_bp10.h interrupt_bp10.h

copy  ..\res\nds32-ae210p_bp10.sag nds32-ae210p_bp10.sag

copy  ..\res\nds32-ae210p_ap10.sag  nds32-ae210p_ap10.sag

copy  ..\res\flash_boot_bp10.c flash_boot_bp10.c

copy  ..\res\flash_boot_ap10.c flash_boot_ap10.c

copy  ..\res\nds32_template.txt  nds32_template.txt

copy  ..\seg_file_generate_script.exe  seg_file_generate_script.exe

seg_file_generate_script.exe

copy  nds32-ae210p.sag  ..\nds32-ae210p.sag

nds_ldsag.exe -t nds32_template.txt nds32-ae210p.sag -o nds32-ae210p.ld

copy  nds32-ae210p.ld  ..\nds32-ae210p.ld

copy  flash_boot.c  ..\bt_audio_app_src\device\flash_boot.c

copy  interrupt.h  ..\..\MVsB1_Base_SDK\driver\driver\inc\interrupt.h

copy  MV_SDKConfig.ini  ..\..\MVsB1_Base_SDK\tools\merge_script\MV_SDKConfig.ini

del MV_SDKConfig.ini

del MV_SDKConfig_ap10.ini

del MV_SDKConfig_bp10.ini

del interrupt.h

del interrupt_ap10.h

del interrupt_bp10.h

del flash_boot.c

del nds32-ae210p.sag

del nds32-ae210p_ap10.sag

del nds32-ae210p_bp10.sag

del seg_file_generate_script.exe

del nds32-ae210p.ld

del nds32_template.txt

del flash_boot_ap10.c

del flash_boot_bp10.c