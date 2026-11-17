@echo off
set project_name=CH559_CDPlayer
set xram_size=0x0800
set xram_loc=0x0600
set code_size=0xEFFF
set dfreq_sys=48000000

if not exist "config.h" echo // project config > config.h

SDCC\bin\sdcc -c -V -mmcs51 --model-large --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I. -DFREQ_SYS=%dfreq_sys% main.c
if errorlevel 1 goto fail
SDCC\bin\sdcc -c -V -mmcs51 --model-large --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I. -DFREQ_SYS=%dfreq_sys% util.c
if errorlevel 1 goto fail
SDCC\bin\sdcc -c -V -mmcs51 --model-large --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I. -DFREQ_SYS=%dfreq_sys% USBHost.c
if errorlevel 1 goto fail
SDCC\bin\sdcc -c -V -mmcs51 --model-large --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I. -DFREQ_SYS=%dfreq_sys% uart.c
if errorlevel 1 goto fail
SDCC\bin\sdcc -c -V -mmcs51 --model-large --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I. -DFREQ_SYS=%dfreq_sys% msc.c
if errorlevel 1 goto fail
SDCC\bin\sdcc -c -V -mmcs51 --model-large --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I. -DFREQ_SYS=%dfreq_sys% mmc.c
if errorlevel 1 goto fail
SDCC\bin\sdcc main.rel util.rel USBHost.rel uart.rel msc.rel mmc.rel -V -mmcs51 --model-large --xram-size %xram_size% --xram-loc %xram_loc% --code-size %code_size% -I. -DFREQ_SYS=%dfreq_sys% -o %project_name%.ihx
if errorlevel 1 goto fail
SDCC\bin\packihx %project_name%.ihx > %project_name%.hex
if errorlevel 1 goto fail
SDCC\bin\hex2bin -c %project_name%.hex
if errorlevel 1 goto fail

echo.
echo Build completed: %project_name%.hex and %project_name%.bin
echo Flash only after reviewing the hardware and test notes.
goto end
:fail
echo BUILD FAILED
:end
pause
