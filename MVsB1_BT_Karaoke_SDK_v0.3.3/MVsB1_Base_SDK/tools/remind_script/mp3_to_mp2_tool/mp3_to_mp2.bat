for %%i in (mp3//*.mp3) do ( ffmpeg.exe -i mp3//%%i -ab 48000 -ac 1 mp2//%%i.mp2)

cd mp2

ren *.mp2 *.

cd ..
