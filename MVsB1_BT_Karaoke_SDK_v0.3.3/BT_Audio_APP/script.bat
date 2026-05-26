cd ..
if exist mp2 (rd /s /q mp2)
md mp2
for %%i in (remind_file//*.mp3) do (..\MVsB1_Base_SDK\tools\remind_script\ffmpeg.exe -i remind_file//%%i -ab 48000 -ac 1 mp2//%%i.mp2 >> info.txt 2>&1)
del info.txt
cd mp2
ren *.mp2 *.
..\..\MVsB1_Base_SDK\tools\remind_script\MergeAudio2BinNew.exe -a 0x0 -i ..\..\..\BT_Audio_APP\mp2
fc ..\..\MVsB1_Base_SDK\tools\remind_script\sound_remind_item.h .\..\bt_audio_app_src\inc\remind_sound_item.h
if %errorlevel%==1 (copy ..\..\MVsB1_Base_SDK\tools\remind_script\sound_remind_item.h .\..\bt_audio_app_src\inc\remind_sound_item.h)
..\..\MVsB1_Base_SDK\tools\libs_copy_script\copyLibs.exe -s ..\..\..\BT_Audio_APP\script_copy.ini
cd ..
rd /s /q mp2
