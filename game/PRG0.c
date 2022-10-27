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
#include "NES_ST/screen_levelcomplete.h"

#include "map_defs.h"
#include "maps_a.h"
#include "meta_tiles_default.h"

#include "NES_ST/meta_player.h"

#define GET_META_TILE_FLAGS(room_table_index) (metatiles_default[current_room[(room_table_index)] * META_TILE_NUM_BYTES + META_TILE_FLAGS_OFFSET])
#define NUM_Y_COLLISION_OFFSETS 3
const unsigned char y_collision_offsets[NUM_Y_COLLISION_OFFSETS] = { 1, 8, 15 };
#define NUM_X_COLLISION_OFFSETS 2
const unsigned char x_collision_offsets[NUM_X_COLLISION_OFFSETS] = { 4, 12 };

const unsigned char bg_banks[NUM_BG_BANKS] = { 0, 1 };

// BCD encoded score values:
const unsigned char SCORE_GEM_BCD[NUM_SCORE_DIGITS] =  { 0, 0, 0, 0, 5 };

typedef struct anim_def
{
	// how many frames to hold on each frame of animation.
	unsigned char ticks_per_frame;

	// how many frames are used in frames array.
	// TODO: would it be better to just use a special value (eg. 0xff)
	//		 to signify the end of the anim?
	unsigned char anim_len;

	// index into meta_sprites array
	unsigned char frames[17];
} anim_def;

const anim_def move_right 		= { 5, 3, { 0, 1, 2 } };
const anim_def move_right_down 	= { 5, 3, { 3, 4, 5 } };
const anim_def move_right_up 	= { 5, 3, { 6, 7, 8 } };

const anim_def goblin_move_right 		= { 5, 3, { 9, 10, 11 } };
const anim_def goblin_move_right_down 	= { 5, 3, { 12, 13, 14 } };
const anim_def goblin_move_right_up 	= { 5, 3, { 15, 16, 17 } };

enum
{
	ANIM_MOVE_RIGHT = 0,
	ANIM_MOVE_RIGHT_DOWN = 1,
	ANIM_MOVE_RIGHT_UP = 2, 

	ANIM_GOBLIN_MOVE_RIGHT,
	ANIM_GOBLIN_MOVE_RIGHT_DOWN,
	ANIM_GOBLIN_MOVE_RIGHT_UP, 	

	NUM_ANIMS,
};

const struct anim_def* sprite_anims[] =
{
	&move_right,
	&move_right_down,
	&move_right_up,

	&goblin_move_right,
	&goblin_move_right_down,
	&goblin_move_right_up,
};

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
void load_screen_levelcomplete()
{
    fade_to_black();
    ppu_off();
    oam_clear();
    vram_adr(NTADR_A(0, 0));
    vram_unrle(screen_levelcomplete);
	vram_adr(NTADR_A(2, 2));
	vram_write("SCORE", 5);	
    ppu_on_all();
    fade_from_black();
}

void vram_buffer_load_2x2_metatile()
{
	// Function gets called from a lot of places, so not safe to use globals.
	static unsigned char local_x;
	static unsigned char local_y;
	static unsigned char local_i;
	static unsigned int local_index16;
	static unsigned int local_att_index16;
	static unsigned char local_temp_a_8;
	static unsigned char local_temp_b_8;

	// SPRITES

	// handle which order to load meta tile rows in.
	local_temp_a_8 = 0;
	local_temp_b_8 = 2;

	local_index16 = GRID_XY_TO_ROOM_INDEX(in_x_tile, in_y_tile);
	local_att_index16 = current_room[local_index16] * META_TILE_NUM_BYTES;
	multi_vram_buffer_horz(&metatiles_default[local_att_index16+local_temp_a_8], 2, get_ppu_addr(0, in_x_tile * CELL_SIZE, in_y_tile * CELL_SIZE));
	multi_vram_buffer_horz(&metatiles_default[local_att_index16+local_temp_b_8], 2, get_ppu_addr(0, in_x_tile * CELL_SIZE, (in_y_tile * CELL_SIZE) + 8));

	// ATTRIBUTES

	// Attributes are in 2x2 meta tile chunks, so we need to round down to the nearest,
	// multiple of 2 (eg. if you pass in index 5, we want to start on 4).
	local_x = (in_x_tile / 2) * 2;
	local_y = (in_y_tile / 2) * 2;

	local_i = 0;

	// room index.
	local_index16 = GRID_XY_TO_ROOM_INDEX(local_x, local_y);
	// meta tile palette index.
	local_att_index16 = (current_room[local_index16] * META_TILE_NUM_BYTES) + 4;
	// bit shift amount
	local_i |= ((metatiles_default)[local_att_index16]);

	local_index16 = local_index16 + 1; //(local_y * 16) + (local_x + 1);
	local_att_index16 = (current_room[local_index16] * META_TILE_NUM_BYTES) + 4;
	local_i |= ((metatiles_default)[local_att_index16]) << 2;

	local_index16 = local_index16 + 15; //((local_y + 1) * 16) + (local_x);
	local_att_index16 = (current_room[local_index16] * META_TILE_NUM_BYTES) + 4;
	local_i |= ((metatiles_default)[local_att_index16]) << 4;

	local_index16 = local_index16 + 1; //((local_y + 1) * 16) + (local_x + 1);
	local_att_index16 = (current_room[local_index16] * META_TILE_NUM_BYTES) + 4;
	local_i |= ((metatiles_default)[local_att_index16]) << 6;	

	one_vram_buffer(local_i, get_at_addr(0, (local_x) * CELL_SIZE, (local_y) * CELL_SIZE));
}

void load_current_map()
{
	//static unsigned char* _current_room;
	
	// "const_cast"
	
	// NUM_CUSTOM_PROPS because the level data starts after the custom props
	memcpy(current_room, &rooms_maps_a[cur_room_index][NUM_CUSTOM_PROPS], 240);	
	//_current_room = (unsigned char*)(current_room);

    gems_remaining = 0;

	// Load all the of the tiles data into the specified nametable.
	for (y = 0; y < 15; ++y)
	{
		for (x = 0; x < 16; ++x)
		{
			index16 = GRID_XY_TO_ROOM_INDEX(x, y);
		    if (GET_META_TILE_FLAGS(index16) & FLAG_COLLECTIBLE)
            {
                ++gems_remaining;
            }
			index16 = current_room[index16] * META_TILE_NUM_BYTES;
			vram_adr(NTADR(in_nametable,x*2,y*2));	
			vram_write(&metatiles_default[index16], 2);
			vram_adr(NTADR(in_nametable,x*2,(y*2)+1));	
			vram_write(&metatiles_default[index16+2], 2);
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
			index16 = (current_room[index16] * META_TILE_NUM_BYTES) + 4;
			// bit shift amount
			i |= (metatiles_default[index16]);

			index16 = (y * ROOM_WIDTH_TILES) + (x + 1);
			index16 = (current_room[index16] * META_TILE_NUM_BYTES) + 4;
			i |= (metatiles_default[index16]) << 2;

			index16 = ((y + 1) * ROOM_WIDTH_TILES) + (x);
			index16 = (current_room[index16] * META_TILE_NUM_BYTES) + 4;
			i |= (metatiles_default[index16]) << 4;

			index16 = ((y + 1) * ROOM_WIDTH_TILES) + (x + 1);
			index16 = (current_room[index16] * META_TILE_NUM_BYTES) + 4;
			i |= (metatiles_default[index16]) << 6;	

			vram_adr(in_nametable + 960 + index);	
			vram_write(&i, 1);
			++index;
		}
	}
}

void update_gameplay()
{
	if ((tick_count % 32) == 0)
	{
		++char_state;
		char_state = char_state & (NUM_BG_BANKS - 1); // %NUM_BG_BANKS assumes power of 2
		set_chr_bank_0(bg_banks[char_state]); // switch the BG bank
	}

	if (!player1.is_dead)
	{
		px_old = player1.pos_x;
		py_old = player1.pos_y;

#define SPEED_BRAKES
#ifdef SPEED_LEFTRIGHT
		if (pads & PAD_LEFT)
		{
			// going right
			if (player1.vel_x > 0)
			{
				player1.pos_x += player1.vel_x >> 1;
				player1.pos_y += player1.vel_y >> 1;				
			}
			// going left
			else if (player1.vel_x < 0)
			{
				player1.pos_x += player1.vel_x << 1;
				player1.pos_y += player1.vel_y << 1;				
			}
		}
		else if (pads & PAD_RIGHT)
		{
			// going right
			if (player1.vel_x > 0)
			{
				player1.pos_x += player1.vel_x << 1;
				player1.pos_y += player1.vel_y << 1;				
			}
			// going left
			else if (player1.vel_x < 0)
			{
				player1.pos_x += player1.vel_x >> 1;
				player1.pos_y += player1.vel_y >> 1;				
			}
		}
		else
		{
			player1.pos_x += player1.vel_x;
			player1.pos_y += player1.vel_y;
		}
#endif // SPEED_LEFTRIGHT
#ifdef SPEED_BRAKES
		player1.pos_x += (pads & PAD_B) ? player1.vel_x >> 1 : player1.vel_x;
		player1.pos_y += (pads & PAD_B) ? player1.vel_y >> 1 : player1.vel_y;
#endif

		x = (high_byte(player1.pos_x)) + (player1.vel_x > 0 ? 16 : 0);

		if (GET_META_TILE_FLAGS(GRID_XY_TO_ROOM_INDEX(x/16, (high_byte(player1.pos_y) + 8)/16)) & FLAG_WALL)
		{
			player1.vel_x *= -1;
		}

		index = GET_META_TILE_FLAGS(GRID_XY_TO_ROOM_INDEX((high_byte(player1.pos_x) + 8)/16, (high_byte(player1.pos_y) + 16)/16));
		
		temp_on_ramp = index & (FLAG_DOWN_LEFT | FLAG_DOWN_RIGHT);

		i = 0;
		if (((high_byte(px_old) + 8) / 16) != ((high_byte(player1.pos_x) + 8) / 16))
		{
			i = 1;
		}
		

		if (i && pads & PAD_DOWN)
		{
			if ((index & (FLAG_DOWN_RIGHT)) && (index & FLAG_ENTRY) && player1.vel_x > 0)
			{
				player1.vel_y = P1_MOVE_SPEED;
				is_on_ramp = 1;
			}
			else if ((index & (FLAG_DOWN_LEFT)) && (index & FLAG_ENTRY) && player1.vel_x < 0)
			{
				player1.vel_y = P1_MOVE_SPEED;
				is_on_ramp = 1;
			}
		}

		if (!temp_on_ramp || is_jumping)
		{
			player1.vel_y += GRAVITY_LOW;
		}

		grounded = 0;
		if (player1.vel_y >= 0 && index & FLAG_FLOOR && (!is_on_ramp || !temp_on_ramp))// (!on_ramp || ((pads & PAD_DOWN) == 0)))
		{
			player1.vel_y = 0;
			player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y)) / 16)) * 16);
			grounded = 1;
			is_jumping = 0;
			is_on_ramp = 0;
		}

		// if (index == 0)
		// {
		// 	dy = 0;
		// 	py = FP_WHOLE((((high_byte(py)) / 16)) * 16);
		// }

		index = GET_META_TILE_FLAGS(GRID_XY_TO_ROOM_INDEX((high_byte(player1.pos_x) + 8)/16, (high_byte(player1.pos_y))/16));
		if (i && pads & PAD_UP)
		{
			if ((index & (FLAG_DOWN_RIGHT)) && (index & FLAG_ENTRY) && player1.vel_x < 0)
			{
				player1.vel_y = -P1_MOVE_SPEED;
				is_on_ramp = 1;
			}
			else if ((index & (FLAG_DOWN_LEFT)) && (index & FLAG_ENTRY) && player1.vel_x > 0)
			{
				player1.vel_y = -P1_MOVE_SPEED;
				is_on_ramp = 1;
			}
		}

		if (pads_new & PAD_A && grounded)
		{
			sfx_play(5, ++cur_sfx_chan);
			player1.vel_y = -(FP_WHOLE(1) + FP_0_25);
			is_jumping = 1;
		}		

		index = GET_META_TILE_FLAGS(GRID_XY_TO_ROOM_INDEX((high_byte(player1.pos_x) + 8)/16, (high_byte(player1.pos_y) + 8)/16));
		if (index & FLAG_COLLECTIBLE)
		{
			in_points = SCORE_GEM_BCD;
			add_bcd_score();
            --gems_remaining;
			display_score();
			//one_vram_buffer('0' + score, get_ppu_addr(0, 16, 0));
			in_x_tile = (high_byte(player1.pos_x) + 8)/16;
			in_y_tile = (high_byte(player1.pos_y) + 8)/16;
			current_room[GRID_XY_TO_ROOM_INDEX(in_x_tile, in_y_tile)] = 0;
			vram_buffer_load_2x2_metatile();
			sfx_play(2, ++cur_sfx_chan);
			//multi_vram_buffer_horz(const char * data, unsigned char len, int ppu_address);

            if (gems_remaining == 0 
#if DEBUG_ENABLED			
			|| (pads & PAD_SELECT)
#endif // DEBUG_ENABLED		
			)
            {
                go_to_state(STATE_LEVEL_COMPLETE);
                return;
            }
		}
		if (index & FLAG_KILL)
		{
			kill_player();
		}

		if ((player1.vel_y >> 8) < (signed int)0)
		{
			i = ANIM_MOVE_RIGHT_UP;
		}
		else if (high_byte(player1.vel_y) > 0)
		{
			i = ANIM_MOVE_RIGHT_DOWN;
		}
		else
		{
			i = ANIM_MOVE_RIGHT;
		}

		// goblin test
		//

		enemy1.pos_x += enemy1.vel_x;
		enemy1.pos_y += enemy1.vel_y;

		in_obj_a = &player1;
		in_obj_b = &enemy1;
		if (intersects_box_box())
		{
			if ((enemy1.pos_y + FP_WHOLE(8)) > (player1.pos_y + FP_WHOLE(16)))
			{
				// if downward, bounce
				if (player1.vel_y > 0)
				{
					sfx_play(5, ++cur_sfx_chan);
					player1.vel_y = -(FP_WHOLE(1) + FP_0_25);
					is_jumping = 1;
					enemy1.vel_x = FP_0_25;
				}
			}
			else
			{
				kill_player();
			}
		}

		global_working_anim = &enemy1.sprite.anim;
		queue_next_anim(ANIM_GOBLIN_MOVE_RIGHT);
		commit_next_anim();
		update_anim();

		//
		// goblin test		
	}
	else
	{
		player1.vel_y += GRAVITY_LOW;
		player1.pos_x += player1.vel_x;
		player1.pos_y += player1.vel_y;

		if (player1.pos_y > FP_WHOLE(240))
		{
			go_to_state(STATE_GAMEOVER);
			return;
		}
	}

	global_working_anim = &player1.sprite.anim;
	queue_next_anim(i);
	commit_next_anim();
	update_anim();

	draw_player();

	if (!player1.is_dead)
	{
		draw_cur_time();
	}

	// if (pads_new & PAD_START)
	// {
	// 	music_play(0);
	// }
	// if (pads_new & PAD_SELECT)
	// {
	// 	music_stop();
	// }


	// goblin test
	//

	oam_meta_spr(
		high_byte(enemy1.pos_x), 
		high_byte(enemy1.pos_y), 
		meta_player_list[sprite_anims[enemy1.sprite.anim.anim_current]->frames[enemy1.sprite.anim.anim_frame]]);

	//
	// goblin test
}

void draw_player()
{
	if (player1.vel_x < 0)
	{
		in_oam_x = high_byte(player1.pos_x);
		in_oam_y = high_byte(player1.pos_y);
		in_oam_data = meta_player_list[sprite_anims[player1.sprite.anim.anim_current]->frames[player1.sprite.anim.anim_frame]];
		c_oam_meta_spr_flipped();
	}
	else
	{
		oam_meta_spr(high_byte(player1.pos_x), high_byte(player1.pos_y), meta_player_list[sprite_anims[player1.sprite.anim.anim_current]->frames[player1.sprite.anim.anim_frame]]);
	}
}

unsigned char update_anim()
{
	static const struct anim_def* cur_anim;
	cur_anim = sprite_anims[global_working_anim->anim_current];

	// Note: In WnW this was done in each draw function manually, and I'm not sure why. Perhaps
	//		 to ensure that the first frame got played before advancing?
	++global_working_anim->anim_ticks;

	if (global_working_anim->anim_ticks >= cur_anim->ticks_per_frame)
	{
		global_working_anim->anim_ticks = 0;
		++global_working_anim->anim_frame;
		// todo: don't always loop.
		if (global_working_anim->anim_frame >= cur_anim->anim_len)
		{
			global_working_anim->anim_frame = 0;
			return 1;
		}
	}
	return 0;
}