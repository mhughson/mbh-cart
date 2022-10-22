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
#include "map_defs.h"
#include "maps_a.h"

const unsigned char palette[16]={ 0x0f,0x0f,0x13,0x37,0x0f,0x17,0x29,0x20,0x0f,0x13,0x23,0x33,0x0f,0x14,0x24,0x34 };
const unsigned char palette_bg[16]={ 0x0f,0x07,0x17,0x10,0x0f,0x03,0x11,0x35,0x0f,0x07,0x16,0x38,0x0f,0x09,0x19,0x29 };

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
unsigned int px;
unsigned int py;
unsigned int px_old;
unsigned int py_old;
signed int dx;
signed int dy;
unsigned char in_oam_x;
unsigned char in_oam_y;
const unsigned char *in_oam_data;
game_actor player1;
game_actor enemy1;
unsigned int temp16;
unsigned char tempFlags;
unsigned char tempFlagsUp;
unsigned char tempFlagsDown;
unsigned char ticks_since_attack;
unsigned char temp_on_ramp;
unsigned char is_jumping;
unsigned char is_on_ramp;
unsigned char current_room[240];
// Used by the anim functions to avoid passing in a parameter.
anim_info* global_working_anim;
unsigned int score;
char in_x_tile; 
char in_y_tile;
unsigned char cur_sfx_chan;
unsigned char char_state;
unsigned char cur_state;
game_actor* in_obj_a;
game_actor* in_obj_b;

// batch add
unsigned char anim_index;
unsigned char grounded;
unsigned char jump_held_count;
unsigned char can_jump;
unsigned char airtime;
unsigned char ticks_down;
unsigned char jump_count;
unsigned char on_ground;
unsigned char new_jump_btn;
unsigned int scroll_y;

#pragma rodata-name ("CODE")
#pragma code-name ("CODE")

// function prototypes
void load_current_map(unsigned int nt);
void update_player();
void update_gameplay();

// const unsigned char current_room[ROOM_WIDTH_TILES * 15] = 
// {
// 	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
// 	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
// 	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
// 	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
// 	1,26,26,11,10,26,26,26,26,26,26,42,27,26,26, 1,
// 	1, 0, 0,12, 8, 9, 0, 0, 0, 0,24,25,28, 0, 0, 1,
// 	1, 0, 0, 0,12,13,14, 0, 0,29,30,28, 0, 0, 0, 1,
// 	1,26,26,26, 6,26,26, 5,26,26,26,26,26,26,26, 1,
// 	0, 0, 0, 6, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,
// 	0, 0, 6, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0,
// 	2, 2, 2, 2, 6, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2,
// 	0, 0, 0, 6, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,
// 	0, 0, 6, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0,
// 	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
// 	3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4,
// };

#define NUM_Y_COLLISION_OFFSETS 3
const unsigned char y_collision_offsets[NUM_Y_COLLISION_OFFSETS] = { 1, 8, 15 };
#define NUM_X_COLLISION_OFFSETS 2
const unsigned char x_collision_offsets[NUM_X_COLLISION_OFFSETS] = { 4, 12 };

const unsigned char bg_banks[NUM_BG_BANKS] = { 0, 1 };


#define FLAG_WALL 			(1 << 0)
#define FLAG_FLOOR			(1 << 1)
#define FLAG_COLLECTIBLE	(1 << 2)
#define FLAG_KILL			(1 << 3)
#define FLAG_DOWN_LEFT		(1 << 4)
#define FLAG_DOWN_RIGHT		(1 << 5)
#define FLAG_ENTRY			(1 << 7)

#define P1_MOVE_SPEED (FP_WHOLE(1))

// It seems like main() MUST be in fixed back!
void main (void) 
{
	ppu_off(); // screen off

	set_vram_buffer(); // do at least once, sets a pointer to a buffer
	clear_vram_buffer();

	set_chr_bank_0(0);
	bank_bg(0);
	bank_spr(1);

	pal_bg(palette_bg);
	pal_spr(palette);

	cur_state = 0xff;
	go_to_state(STATE_BOOT);

	// infinite loop
	while (1)
	{
		++tick_count;

		ppu_wait_nmi(); // wait till beginning of the frame

		oam_clear();

		pads = pad_poll(0) | pad_poll(1); // read the first controller
		pads_new = get_pad_new(0) | get_pad_new(1); // newly pressed button. do pad_poll first

		clear_vram_buffer(); // do at the beginning of each frame

		switch (cur_state)
		{
			case STATE_BOOT:
			{
				if (pads_new & PAD_ANY_CONFIRM_BUTTON)
				{
					go_to_state(STATE_TITLE);
				}
				break;
			}

			case STATE_TITLE:
			{
				if (pads_new & PAD_ANY_CONFIRM_BUTTON)
				{
					go_to_state(STATE_GAMEPLAY);
				}				
				break;
			}

			case STATE_GAMEPLAY:
			{
				update_gameplay();
				break;
			}

			case STATE_GAMEOVER:
			{
				if (pads_new & PAD_ANY_CONFIRM_BUTTON)
				{
					go_to_state(STATE_TITLE);
				}				
				break;
			}
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
			++score;
			display_score();
			//one_vram_buffer('0' + score, get_ppu_addr(0, 16, 0));
			in_x_tile = (high_byte(player1.pos_x) + 8)/16;
			in_y_tile = (high_byte(player1.pos_y) + 8)/16;
			current_room[GRID_XY_TO_ROOM_INDEX(in_x_tile, in_y_tile)] = 0;
			vram_buffer_load_2x2_metatile();
			sfx_play(2, ++cur_sfx_chan);
			//multi_vram_buffer_horz(const char * data, unsigned char len, int ppu_address);
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

	if (pads_new & PAD_START)
	{
		music_play(0);
	}
	if (pads_new & PAD_SELECT)
	{
		music_stop();
	}


	// goblin test
	//

	oam_meta_spr(
		high_byte(enemy1.pos_x), 
		high_byte(enemy1.pos_y), 
		meta_player_list[sprite_anims[enemy1.sprite.anim.anim_current]->frames[enemy1.sprite.anim.anim_frame]]);

	//
	// goblin test
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
	multi_vram_buffer_horz(&metatiles[local_att_index16+local_temp_a_8], 2, get_ppu_addr(0, in_x_tile * CELL_SIZE, in_y_tile * CELL_SIZE));
	multi_vram_buffer_horz(&metatiles[local_att_index16+local_temp_b_8], 2, get_ppu_addr(0, in_x_tile * CELL_SIZE, (in_y_tile * CELL_SIZE) + 8));

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
	local_i |= ((metatiles)[local_att_index16]);

	local_index16 = local_index16 + 1; //(local_y * 16) + (local_x + 1);
	local_att_index16 = (current_room[local_index16] * META_TILE_NUM_BYTES) + 4;
	local_i |= ((metatiles)[local_att_index16]) << 2;

	local_index16 = local_index16 + 15; //((local_y + 1) * 16) + (local_x);
	local_att_index16 = (current_room[local_index16] * META_TILE_NUM_BYTES) + 4;
	local_i |= ((metatiles)[local_att_index16]) << 4;

	local_index16 = local_index16 + 1; //((local_y + 1) * 16) + (local_x + 1);
	local_att_index16 = (current_room[local_index16] * META_TILE_NUM_BYTES) + 4;
	local_i |= ((metatiles)[local_att_index16]) << 6;	

	one_vram_buffer(local_i, get_at_addr(0, (local_x) * CELL_SIZE, (local_y) * CELL_SIZE));
}

void load_current_map(unsigned int nt)
{
	//static unsigned char* _current_room;
	
	// "const_cast"
	
	// NUM_CUSTOM_PROPS because the level data starts after the custom props
	memcpy(current_room, &rooms_maps_a[1/*cur_room_index*/][NUM_CUSTOM_PROPS], 240);	
	//_current_room = (unsigned char*)(current_room);

	// Load all the of the tiles data into the specified nametable.
	for (y = 0; y < 15; ++y)
	{
		for (x = 0; x < 16; ++x)
		{
			index16 = GRID_XY_TO_ROOM_INDEX(x, y);
			index16 = current_room[index16] * META_TILE_NUM_BYTES;
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
			index16 = (current_room[index16] * META_TILE_NUM_BYTES) + 4;
			// bit shift amount
			i |= (metatiles[index16]);

			index16 = (y * ROOM_WIDTH_TILES) + (x + 1);
			index16 = (current_room[index16] * META_TILE_NUM_BYTES) + 4;
			i |= (metatiles[index16]) << 2;

			index16 = ((y + 1) * ROOM_WIDTH_TILES) + (x);
			index16 = (current_room[index16] * META_TILE_NUM_BYTES) + 4;
			i |= (metatiles[index16]) << 4;

			index16 = ((y + 1) * ROOM_WIDTH_TILES) + (x + 1);
			index16 = (current_room[index16] * META_TILE_NUM_BYTES) + 4;
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

void update_player()
{
	static unsigned char hit_kill_box;
	// static unsigned int high_x;
	// static unsigned int high_y;
	static const unsigned char high_walk_speed = (WALK_SPEED >> HALF_POS_BIT_COUNT);

#define PLAYER_HEIGHT (16)

	PROFILE_POKE(PROF_G);

	// high_x = high_2byte(player1.pos_x);
	// high_y = high_2byte(player1.pos_y);

	if (pads & PAD_LEFT )
//	&& 
		//((cam.pos_x) / 256) <= (( high_2byte((player1.pos_x)) - (WALK_SPEED >> 16)) / 256) && 
//		high_2byte(player1.pos_x) - cam.pos_x >= (high_walk_speed + 8) &&
//		player1.pos_x >= WALK_SPEED + FP_WHOLE(8))
	{
		temp16 = player1.pos_x;

		// move the player left.
		player1.pos_x -= WALK_SPEED;// + FP_0_5;
		player1.vel_x = -WALK_SPEED;

		// track if the player hit a spike.
		hit_kill_box = 0;

		for (i = 0; i < NUM_Y_COLLISION_OFFSETS; ++i)
		{
			// take the player position, offset it a little
			// so that the arms overlap the wall a bit (+8),
			// convert to metatile space (16x16) (>>4).
			// The position is stored using fixed point math, 
			// where the high byte is the whole part, and the
			// low byte is the fraction.
			x = (high_byte(player1.pos_x) + x_collision_offsets[0] - 1) >> 4;

			// Player is 24 pixels high, so +12 checks middle of them.
			// >>4 to put the position into metatile-space (16x16 chunks).
			y = (high_byte(player1.pos_y) + y_collision_offsets[i]) >> 4;

			// Prevent player for colliding with data outside the map, but still
			// allow them to travel above (and below) the top of the map.
			// This hard coded 15 will obviously need to change if we add vertical
			// levels.
			if (y < 15)
			{
				// Convert the x,y into an index into the room data array.
				index16 = GRID_XY_TO_ROOM_INDEX(x, y);

				tempFlags = GET_META_TILE_FLAGS(index16);

				// Check if that point is in a solid metatile
//				if (tempFlags & FLAG_WALL || (grounded && tempFlags & FLAG_KILL))
//				{
//					// Hit a wall, shift back to the edge of the wall.
//					player1.pos_x = temp16;
//					hit_kill_box = 0;
//					break;
//				}
			}
		}
	}
	// Is the right side of the sprite, after walking, going to be passed the end of the map?
	else if (pads & PAD_RIGHT)
//	 && (player1.pos_x + WALK_SPEED + FP_WHOLE(16) ) <= FP_WHOLE(ROOM_WIDTH_PIXELS))
	{

		temp16 = player1.pos_x;
		player1.pos_x += WALK_SPEED;// + FP_0_5;
		player1.vel_x = WALK_SPEED;

		hit_kill_box = 0;

		for (i = 0; i < NUM_Y_COLLISION_OFFSETS; ++i)
		{
			// take the player position, offset it a little
			// so that the arms overlap the wall a bit (+16),
			// convert to metatile space (16x16) (>>4).
			// The position is stored using fixed point math, 
			// where the high byte is the whole part, and the
			// low byte is the fraction.
			x = (high_byte(player1.pos_x) + x_collision_offsets[NUM_X_COLLISION_OFFSETS - 1] + 1) >> 4;

			// Player is 24 pixels high, so +12 checks middle of them.
			// >>4 to put the position into metatile-space (16x16 chunks).
			y = (high_byte(player1.pos_y) + y_collision_offsets[i]) >> 4;

			if (y < 15)
			{
				// Convert the x,y into an index into the room data array.
				//temp16 = GRID_XY_TO_ROOM_NUMBER(x, y);
				index16 = GRID_XY_TO_ROOM_INDEX(x, y);

				tempFlags = GET_META_TILE_FLAGS(index16);

				// Check if that point is in a solid metatile.
				// Treat spikes like walls if the player is on the floor. It feels lame to 
				// walk into a spike and die; you should need to fall into them.
//				if (tempFlags & FLAG_WALL || (grounded && tempFlags & FLAG_KILL))
//				{
//					// Hit a wall, shift back to the edge of the wall.
//					player1.pos_x = temp16; //(unsigned long)((x << 4) - 17) << HALF_POS_BIT_COUNT;
//					hit_kill_box = 0;
//
//					break;
//				}
				// if (tempFlags & FLAG_DOWN_LEFT)
				// {
				// 	player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
				// }
			}
		}
	}
	else
	{
		player1.vel_x = 0;
	}


	if (pads & PAD_UP)
//	 && (player1.pos_x + WALK_SPEED + FP_WHOLE(16) ) <= FP_WHOLE(ROOM_WIDTH_PIXELS))
	{

		temp16 = player1.pos_x;

		hit_kill_box = 0;

		// take the player position, offset it a little
		// so that the arms overlap the wall a bit (+16),
		// convert to metatile space (16x16) (>>4).
		// The position is stored using fixed point math, 
		// where the high byte is the whole part, and the
		// low byte is the fraction.
		x = (high_byte(player1.pos_x) + 8) >> 4;

		// Player is 24 pixels high, so +12 checks middle of them.
		// >>4 to put the position into metatile-space (16x16 chunks).
		y = (high_byte(player1.pos_y) + PLAYER_HEIGHT - 2) >> 4;

		if (y < 15)
		{
			// Convert the x,y into an index into the room data array.
			//temp16 = GRID_XY_TO_ROOM_NUMBER(x, y);
			index16 = GRID_XY_TO_ROOM_INDEX(x, y);

			tempFlags = GET_META_TILE_FLAGS(index16);

			// Check if that point is in a solid metatile.
			// Treat spikes like walls if the player is on the floor. It feels lame to 
			// walk into a spike and die; you should need to fall into them.
			if (tempFlags & FLAG_DOWN_LEFT || tempFlags & FLAG_DOWN_RIGHT)
			{
				player1.pos_x += WALK_SPEED;// + FP_0_5;
				player1.pos_y -= WALK_SPEED;
				player1.vel_y = 0;
				hit_kill_box = 0;
			}
		}
	}	

	if (pads & PAD_A)
	{
		++ticks_down;

		//is player on ground recently.
		//allow for jump right after 
		//walking off ledge.
		on_ground = (grounded != 0 || airtime < JUMP_COYOTE_DELAY);
		//was btn presses recently?
		//allow for pressing right before
		//hitting ground.
		new_jump_btn = ticks_down < 10;

		//is player continuing a jump
		//or starting a new one?
		if (jump_held_count > 0)
		{
			++jump_held_count;
			//keep applying jump velocity
			//until max jump time.
			if (jump_held_count < JUMP_HOLD_MAX)
			{
				player1.vel_y = -(JUMP_VEL);//keep going up while held
			}
		}
		else if ((on_ground && new_jump_btn && jump_count == 0))
		{
			++jump_held_count;
			++jump_count;
			player1.vel_y = -(JUMP_VEL);
		}
		else if (jump_count < 1 && pads_new & PAD_A)
		{
			++jump_held_count;
			++jump_count;
			player1.vel_y = -(JUMP_VEL);
		}
	}
	else
	{
		// if (ticks_down > 0 && ticks_down < 5)
		// {
		// 	player1.vel_y >>= 2;
		// }
		ticks_down = 0;
		jump_held_count = 0;
	}

	if (ticks_since_attack >= ATTACK_LEN)
	{
		if (pads_new & PAD_B)
		{
			ticks_since_attack = 0;
		}
	}
	else
	{
		++ticks_since_attack;
	}

	player1.vel_y += GRAVITY;
	player1.pos_y += player1.vel_y;

	// Assume not on the ground each frame, until we detect we hit it.
	grounded = 0;
	is_on_ramp = temp_on_ramp;
	temp_on_ramp = 0;

	// Roof check
	if (player1.vel_y < 0 && 0)
	{
		for (i = 0; i < NUM_X_COLLISION_OFFSETS; ++i)
		{
			x = (high_byte(player1.pos_x) + x_collision_offsets[i]) >> 4;
			y = (high_byte(player1.pos_y)) >> 4; // head
			if (y < 15)
			{
				index16 = GRID_XY_TO_ROOM_INDEX(x, y);
				if (GET_META_TILE_FLAGS(index16) & FLAG_FLOOR)
				{
					player1.pos_y = (unsigned long)((y << 4) + CELL_SIZE) << HALF_POS_BIT_COUNT;
					player1.vel_y = 0;
					// prevent hovering against the roof.
					jump_held_count = JUMP_HOLD_MAX;
					break;
				}
			}
		}					
	}
	else // floor check
	{
		hit_kill_box = 0;
//		for (i = 0; i < NUM_X_COLLISION_OFFSETS; ++i)
		{
			x = (high_byte(player1.pos_x) + 8) >> 4; //x_collision_offsets[i]) >> 4;
			y = (high_byte(player1.pos_y) + PLAYER_HEIGHT - 1) >> 4; // feet

			if (y > 15 && y < 20)
			{
				hit_kill_box = 1;
			}
			else if (y < 15)
			{
				//x = (high_byte(player1.pos_x) + 8) >> 4;
				//y = (high_byte(player1.pos_y) + PLAYER_HEIGHT - 1) >> 4;
				index16 = GRID_XY_TO_ROOM_INDEX(x, y);
				tempFlags = GET_META_TILE_FLAGS(index16);
				index16 = GRID_XY_TO_ROOM_INDEX(x, y + 1);
				tempFlagsDown = GET_META_TILE_FLAGS(index16);
				index16 = GRID_XY_TO_ROOM_INDEX(x, y - 1);
				tempFlagsUp = GET_META_TILE_FLAGS(index16);
				

				if (tempFlags & FLAG_DOWN_LEFT && (is_on_ramp || player1.vel_x > 0))
				{
					jump_count = 0;
					grounded = 1;
					player1.pos_y = (((y + 1) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
					player1.vel_y = 0;
					airtime = 0;
					hit_kill_box = 0;
					temp_on_ramp = 1;
					//player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
					//break;
				}
				else if (tempFlagsUp & FLAG_DOWN_LEFT && (is_on_ramp || player1.vel_x > 0))
				{
					jump_count = 0;
					grounded = 1;
					player1.pos_y = (((y) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
					player1.vel_y = 0;
					airtime = 0;
					hit_kill_box = 0;
					temp_on_ramp = 1;
					//player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
					//break;
				}
				// else if (tempFlags & FLAG_DOWN_LEFT && (temp_was_on_ramp || player1.vel_x < 0))
				// {
				// 	jump_count = 0;
				// 	grounded = 1;
				// 	player1.pos_y = (((y + 1) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
				// 	player1.vel_y = 0;
				// 	airtime = 0;
				// 	hit_kill_box = 0;
				// 	on_ramp = 1;
				// 	//player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
				// 	//break;
				// }
				else if ((tempFlagsDown & FLAG_DOWN_LEFT) && (player1.vel_x < 0))
				{
					jump_count = 0;
					grounded = 1;
					player1.pos_y = (((y + 2) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
					player1.vel_y = 0;
					airtime = 0;
					hit_kill_box = 0;
					temp_on_ramp = 1;
					//player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
					//break;
				}
				else if ((tempFlags & FLAG_DOWN_LEFT) && (is_on_ramp && player1.vel_x < 0))
				{
					jump_count = 0;
					grounded = 1;
					player1.pos_y = (((y + 1) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
					player1.vel_y = 0;
					airtime = 0;
					hit_kill_box = 0;
					temp_on_ramp = 1;
					//player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
					//break;
				}
				// index16 = GRID_XY_TO_ROOM_INDEX(x, y);
				// tempFlags = GET_META_TILE_FLAGS(index16);
				else if (tempFlags & FLAG_FLOOR)
				{
					jump_count = 0;
					grounded = 1;
					player1.pos_y = (((y) << 4) - PLAYER_HEIGHT) << HALF_POS_BIT_COUNT;
					player1.vel_y = 0;
					airtime = 0;
					hit_kill_box = 0;
					//break;
				}

				// Were we on a ramp last frame, but not are not? Check above
				// if (temp_was_on_ramp && !on_ramp)
				// {
				// 	//x = (high_byte(player1.pos_x) + 8) >> 4;
				// 	//y = (high_byte(player1.pos_y) + PLAYER_HEIGHT - 1) >> 4;
				// 	index16 = GRID_XY_TO_ROOM_INDEX(x, y - 1);
				// 	tempFlags = GET_META_TILE_FLAGS(index16);
				// 	if (tempFlags & FLAG_DOWN_LEFT)
				// 	{
				// 		jump_count = 0;
				// 		grounded = 1;
				// 		player1.pos_y = (((y) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
				// 		player1.vel_y = 0;
				// 		airtime = 0;
				// 		hit_kill_box = 0;
				// 		on_ramp = 1;
				// 		//player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
				// 		//break;
				// 	}
				// 	index16 = GRID_XY_TO_ROOM_INDEX(x, y + 1);
				// 	tempFlags = GET_META_TILE_FLAGS(index16);
				// 	if (tempFlags & FLAG_DOWN_LEFT)
				// 	{
				// 		jump_count = 0;
				// 		grounded = 1;
				// 		player1.pos_y = (((y + 2) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
				// 		player1.vel_y = 0;
				// 		airtime = 0;
				// 		hit_kill_box = 0;
				// 		on_ramp = 1;
				// 		//player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
				// 		//break;
				// 	}
				// }

				//todo:
				// If we were grounded last frame (including on a ramp) check for ramps above and below us
				// to handle case where we clip through ramps at the top due to moving more than
				// one pixel at a time.

				// allow jumping off slopes.

				// only snap to slop if below the slope line

				// We want the kill check to be more forgiving.
				y = (high_byte(player1.pos_y) + (PLAYER_HEIGHT - 8)) >> 4; // feet
				// Make sure the new y value is also on screen.
				if (y < 15)
				{
					index16 = GRID_XY_TO_ROOM_INDEX(x, y);
					tempFlags = GET_META_TILE_FLAGS(index16);
//					if (tempFlags & FLAG_KILL)
//					{
//						hit_kill_box = 1;
//					}
				}
			}
		}

		if (hit_kill_box == 1)
		{
//			kill_player();
		}	
	}

	if (grounded == 0)
	{
		++airtime;
		if (airtime >= JUMP_COYOTE_DELAY && jump_count == 0)
		{
			// We fell of a ledge. eat a jump so that you can't fall->jump->jump to get further.
			jump_count++;
		}
	}	

	if (pads & PAD_RIGHT)
	{
		player1.facing_left = 0;
	}
	else if (pads & PAD_LEFT)
	{
		player1.facing_left = 1;
	}

//	if (!grounded)
//	{
//		if (player1.vel_y > 0)
//		{
//			anim_index = player1.facing_left ? ANIM_PLAYER_FALL_LEFT : ANIM_PLAYER_FALL_RIGHT;
//			global_working_anim = &player1.sprite.anim;
//			queue_next_anim(anim_index);
//			commit_next_anim();
//		}
//		else
//		{
//			anim_index = player1.facing_left ? ANIM_PLAYER_JUMP_LEFT : ANIM_PLAYER_JUMP_RIGHT;
//			global_working_anim = &player1.sprite.anim;
//			queue_next_anim(anim_index);
//			commit_next_anim();
//		}
//	}
//	else if (pad_all & PAD_RIGHT)
//	{
//		//player1.facing_left = 0;
//		anim_index = ANIM_PLAYER_RUN_RIGHT;
//		global_working_anim = &player1.sprite.anim;
//		queue_next_anim(anim_index);
//		commit_next_anim();
//	}
//	else if (pad_all & PAD_LEFT)
//	{
//		//player1.facing_left = 1;
//		anim_index = ANIM_PLAYER_RUN_LEFT;
//		global_working_anim = &player1.sprite.anim;
//		queue_next_anim(anim_index);
//		commit_next_anim();
//	}
//	else if (pad_all & PAD_DOWN)
//	{
//		anim_index = player1.facing_left ? ANIM_PLAYER_IDLE_CROUCH_LEFT : ANIM_PLAYER_IDLE_CROUCH_RIGHT;
//		global_working_anim = &player1.sprite.anim;
//		queue_next_anim(anim_index);
//		commit_next_anim();
//	}
//	else
//	{		
//		anim_index = (ticks_since_attack < ATTACK_LEN) ? ANIM_PLAYER_IDLE_ATTACK_RIGHT : (player1.facing_left ? ANIM_PLAYER_IDLE_LEFT : ANIM_PLAYER_IDLE_RIGHT);
//		global_working_anim = &player1.sprite.anim;
//		queue_next_anim(anim_index);
//		commit_next_anim();
//	}

	//draw_player();

	PROFILE_POKE(PROF_R);
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

void queue_next_anim(unsigned char next_anim_index)
{
	global_working_anim->anim_queued = next_anim_index;
}

void commit_next_anim()
{
	if (global_working_anim->anim_queued != 0xff && global_working_anim->anim_queued != global_working_anim->anim_current)
	{
		global_working_anim->anim_current = global_working_anim->anim_queued;
		global_working_anim->anim_frame = 0;
		global_working_anim->anim_ticks = 0;
	}

	global_working_anim->anim_queued = 0xff;
}

void go_to_state(unsigned char next_state)
{
	if (next_state == cur_state)
	{
		return;
	}

	switch (next_state)
	{	
		default:
		{
			break;
		}
	}

	cur_state = next_state;

	switch (cur_state)
	{
		case STATE_BOOT:
		{
			banked_call(BANK_0, load_screen_boot);
			break;
		}

		case STATE_TITLE:
		{
			banked_call(BANK_0, load_screen_title);
			break;
		}

		case STATE_GAMEPLAY:
		{
			ppu_off();

			load_current_map(NAMETABLE_A);

			// vram_adr(NTADR_A(7, 2));
			// vram_write("NESDEV COMPO 2022", 17);

			vram_adr(NTADR_A(2, 1));
			vram_write("SCORE", 5);

			ppu_on_all(); // turn on screen

			// todo: non-vram version!
			score = 0;
			display_score();

			pal_bright(4);

			px = 128 << 8;
			py = (6 * 16) << 8;
			dx = P1_MOVE_SPEED;
			dy = 0;
			is_jumping = 0;

			player1.pos_x = FP_WHOLE(4 * 16);
			player1.pos_y = FP_WHOLE(9 * 16);
			player1.vel_x = P1_MOVE_SPEED;
			player1.vel_y = 0;
			player1.is_dead = 0;

			enemy1.pos_x = FP_WHOLE(2 * 16);
			enemy1.pos_y = FP_WHOLE(9 * 16);
			enemy1.vel_x = P1_MOVE_SPEED;
			enemy1.vel_y = 0;
			enemy1.is_dead = 0;

			break;
		}

		case STATE_GAMEOVER:
		{
			banked_call(BANK_0, load_screen_gameover);
			break;
		}

		default:
		{
			break;
		}
	}
}

// box to box intersection

unsigned char intersects_box_box()
{
	static signed int _pos_delta;
	static signed int _dist_sum;
	static signed int _width_half = 8;
	static signed int _height_half = 8;

	_pos_delta = (high_byte(in_obj_a->pos_x) + _width_half) - (high_byte(in_obj_b->pos_x) + _width_half);
	_dist_sum = _width_half + _width_half;
	if( abs(_pos_delta) >= _dist_sum ) return 0;

	_pos_delta = (high_byte(in_obj_a->pos_y) + _height_half) - (high_byte(in_obj_b->pos_y) + _height_half);
	_dist_sum = _height_half + _height_half;
	if( abs(_pos_delta) >= _dist_sum ) return 0;
	
	return 1;
}

void kill_player()
{
	music_stop();
	sfx_play(21, 0);

	player1.vel_x = 0;
	player1.vel_y = -FP_WHOLE(3);
	player1.is_dead = 1;
	//go_to_state(STATE_GAMEOVER);
}

#define FADE_DELAY 2
void fade_to_black()
{
	pal_bright(3);
	delay(FADE_DELAY);
	pal_bright(2);
	delay(FADE_DELAY);
	pal_bright(1);
	delay(FADE_DELAY);
	pal_bright(0);
//	delay(FADE_DELAY);
}

void fade_from_black()
{
	pal_bright(1);
	delay(FADE_DELAY);
	pal_bright(2);
	delay(FADE_DELAY);
	pal_bright(3);
	delay(FADE_DELAY);
	pal_bright(4);
//	delay(FADE_DELAY);
}

void display_score()
{
	static unsigned long temp_score;
	static unsigned char i;

	temp_score = score;

	// clear out any old score.
	multi_vram_buffer_horz("0000000", 7, get_ppu_addr(0, 2<<3, 2<<3));

	i = 0;
	while(temp_score != 0)
    {
        unsigned char digit = temp_score % 10;
        one_vram_buffer('0' + digit, get_ppu_addr(0, (9<<3) - (i << 3), 2<<3 ));

        temp_score = temp_score / 10;
		++i;
    }
}