@echo off

:: Options
:: audio
:: code
:: run
 
::echo %audio%
::echo %code%
::echo %run%

set name="main"
set name_no_quote=main

set path=..\bin\;%path%

set CC65_HOME=..\

set a53enable=0
set vs_on=1

IF DEFINED audio (
	MUSIC\text2vol5.exe MUSIC\songs.txt -ca65
	MUSIC\nsf2data5.exe MUSIC\sounds.nsf -ca65
)

IF DEFINED maps (
	python NES_ST/meta.py test_meta_tiles.nam ..\maps\nametable.json default
	python MAPS\generate_maps_header.py
)

IF DEFINED code (
	REM -g adds debug information, but the end result .nes file is not
	REM affected, so leave it in all the time.
	cc65 -g -Oirs -D VS_SYS_ENABLED=%vs_on% %name%.c --add-source
	cc65 -g -Oirs -D VS_SYS_ENABLED=%vs_on% PRG0.c --add-source
	cc65 -g -Oirs -D VS_SYS_ENABLED=%vs_on% PRG1.c --add-source
	cc65 -g -Oirs -D VS_SYS_ENABLED=%vs_on% PRG2.c --add-source
	ca65 -D VS_SYS_ENABLED=%vs_on% crt0.s
	ca65 %name%.s -g
	ca65 PRG0.s -g
	ca65 PRG1.s -g
	ca65 PRG2.s -g

	REM -dbgfile does not impact the resulting .nes file.
	ld65 -C nrom_32k_vert.cfg --dbgfile %name%.dbg -o %name%.nes crt0.o %name%.o PRG0.o PRG1.o PRG2.o nes.lib -Ln labels.txt -m map.txt

	del *.o

	mkdir BUILD\
	move /Y %name%.nes BUILD\ 
	move /Y %name%.dbg BUILD\ 
	move /Y labels.txt BUILD\ 
	move /Y %name%.s BUILD\ 
	move /Y PRG0.s BUILD\ 
	move /Y PRG1.s BUILD\ 
	move /Y PRG2.s BUILD\
)

if DEFINED run (
	BUILD\%name%.nes
)

if DEFINED deploy (
	N8\edlink-n8 ..\game\BUILD\%name_no_quote%.nes
)

::set audio
::set code
::set run