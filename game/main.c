/** (C) Matt Hughson 2020 */

#include "LIB/neslib.h"
#include "LIB/nesdoug.h"
#include "A53/bank_helpers.h"
#include "../include/stdlib.h"

#include "main.h"
#include "PRG0.h"

#pragma rodata-name ("CODE")
#pragma code-name ("CODE")

#include "NES_ST/meta_player.h"
#include "meta_tiles.h"

const unsigned char palette[16]={ 0x0f,0x05,0x23,0x37,0x0f,0x01,0x21,0x31,0x0f,0x06,0x16,0x26,0x0f,0x09,0x19,0x29 };

// Initalized RAM variables
//

unsigned char i;
unsigned char index;
unsigned int index16;
unsigned char x;
unsigned char y;
unsigned char tick_count;
unsigned char pads;
unsigned char pads_new;
unsigned char px;
unsigned char py;
signed char dx;
signed char dy;
unsigned char in_oam_x;
unsigned char in_oam_y;
const unsigned char *in_oam_data;

#pragma rodata-name ("CODE")
#pragma code-name ("CODE")

// function prototypes
void load_current_map(unsigned int nt);

const unsigned char current_room[ROOM_WIDTH_TILES * 15] = 
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4,
};

// It seems like main() MUST be in fixed back!
void main (void) 
{
	ppu_off(); // screen off

	set_vram_buffer(); // do at least once, sets a pointer to a buffer
	clear_vram_buffer();

	set_chr_bank_0(0);
	bank_bg(0);
	bank_spr(1);

	pal_bg(palette);
	pal_spr(palette);

	load_current_map(NAMETABLE_A);

	vram_adr(NTADR_A(7, 4));
	vram_write("NESDEV COMPO 2022", 17);

	ppu_on_all(); // turn on screen

	pal_bright(4);

	px = 128;
	py = 120;
	dx = 1;
	dy = 0;

	// infinite loop
	while (1)
	{
		++tick_count;

		ppu_wait_nmi(); // wait till beginning of the frame

		oam_clear();

		pads = pad_poll(0) | pad_poll(1); // read the first controller
		pads_new = get_pad_new(0) | get_pad_new(1); // newly pressed button. do pad_poll first

		clear_vram_buffer(); // do at the beginning of each frame


		px += dx;
		px += dy;

		x = (px) + (dx > 0 ? 16 : 0);

		if (GET_META_TILE_FLAGS(GRID_XY_TO_ROOM_INDEX(x/16, py/16)) == 1)
		{
			dx *= -1;
		}

		if (dx < 0)
		{
			in_oam_x = px;
			in_oam_y = py;
			in_oam_data = meta_player_list[0];
			c_oam_meta_spr_flipped();
		}
		else
		{
			oam_meta_spr(px, py, meta_player_list[0]);
		}

		if (pads_new & PAD_A)
		{
			sfx_play(0, 0);
		}
		if (pads_new & PAD_B)
		{
			sfx_play(2, 0);
		}
		if (pads_new & PAD_START)
		{
			music_play(0);
		}
		if (pads_new & PAD_SELECT)
		{
			music_stop();
		}

		if (pads & PAD_LEFT) --px;
		if (pads & PAD_RIGHT) ++px;
		if (pads & PAD_UP) --py;
		if (pads & PAD_DOWN) ++py;
	}
}

void load_current_map(unsigned int nt)
{
	static unsigned char* _current_room;
	
	// "const_cast"
	_current_room = (unsigned char*)(current_room);

	// Load all the of the tiles data into the specified nametable.
	for (y = 0; y < 15; ++y)
	{
		for (x = 0; x < 16; ++x)
		{
			index16 = GRID_XY_TO_ROOM_INDEX(x, y);
			index16 = _current_room[index16] * META_TILE_NUM_BYTES;
			vram_adr(NTADR(nt,x*2,y*2));	
			vram_write(&metatiles[index16], 2);
			vram_adr(NTADR(nt,x*2,(y*2)+1));	
			vram_write(&metatiles[index16+2], 2);
		}
	}

	index = 0;
	// Write attribute data to VRAM in 32x32 chunks.
	for (y = 0; y < 15; y+=2)
	{
		for (x = 0; x < 16; x+=2)
		{
			i = 0;

			// room index.
			index16 = (y * ROOM_WIDTH_TILES) + (x);
			// meta tile palette index.
			index16 = (_current_room[index16] * META_TILE_NUM_BYTES) + 4;
			// bit shift amount
			i |= (metatiles[index16]);

			index16 = (y * ROOM_WIDTH_TILES) + (x + 1);
			index16 = (_current_room[index16] * META_TILE_NUM_BYTES) + 4;
			i |= (metatiles[index16]) << 2;

			index16 = ((y + 1) * ROOM_WIDTH_TILES) + (x);
			index16 = (_current_room[index16] * META_TILE_NUM_BYTES) + 4;
			i |= (metatiles[index16]) << 4;

			index16 = ((y + 1) * ROOM_WIDTH_TILES) + (x + 1);
			index16 = (_current_room[index16] * META_TILE_NUM_BYTES) + 4;
			i |= (metatiles[index16]) << 6;	

			vram_adr(nt + 960 + index);	
			vram_write(&i, 1);
			++index;
		}
	}
}

void c_oam_meta_spr_flipped(void)
{
	static const unsigned char* _elem;
	static unsigned char _x;
	static unsigned char _y;
	static unsigned char _sprite;
	static unsigned char _att;

	if (in_oam_data == NULL)
	{
		return;
	}

	_elem = in_oam_data;

	while (_elem[0] != END_OF_META)
	{
		_x = 8 - _elem[0];
		_y = _elem[1];
		_sprite = _elem[2];
		_att = _elem[3] | OAM_FLIP_H;
		_elem += 4;
		oam_spr(in_oam_x + _x, in_oam_y + _y, _sprite, _att);
	}
}