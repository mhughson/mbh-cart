#include "PRG0.h"
#include "main.h"
#include "LIB/neslib.h"
#include "LIB/nesdoug.h"
#include "A53/bank_helpers.h"
#include "../include/stdlib.h"

#pragma rodata-name ("BANK0")
#pragma code-name ("BANK0")

#include "NES_ST/screen_boot.h"
#include "NES_ST/screen_title.h"
#include "NES_ST/screen_gameover.h"

void load_screen_boot()
{
    ppu_off();
    vram_adr(NTADR_A(0, 0));
    vram_unrle(screen_boot);
    ppu_on_all();
}
void load_screen_title()
{
    ppu_off();
    vram_adr(NTADR_A(0, 0));
    vram_unrle(screen_title);
    ppu_on_all();
}
void load_screen_gameover()
{
    fade_to_black();
    ppu_off();
    oam_clear();
    vram_adr(NTADR_A(0, 0));
    vram_unrle(screen_gameover);
    ppu_on_all();
    fade_from_black();
}