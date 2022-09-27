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
unsigned int temp16;
unsigned char tempFlags;
unsigned char tempFlagsUp;
unsigned char tempFlagsDown;
unsigned char ticks_since_attack;
unsigned char on_ramp;
unsigned char temp_was_on_ramp;
unsigned char current_room[240];

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


#define FLAG_WALL 		(1 << 0)
#define FLAG_FLOOR		(1 << 1)
#define FLAG_UP_LEFT	(1 << 2)
#define FLAG_UP_RIGHT	(1 << 3)
#define FLAG_DOWN_LEFT	(1 << 4)
#define FLAG_DOWN_RIGHT	(1 << 5)
#define FLAG_ENTRY		(1 << 7)

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

	pal_bg(palette);
	pal_spr(palette);

	load_current_map(NAMETABLE_A);

	vram_adr(NTADR_A(7, 4));
	vram_write("NESDEV COMPO 2022", 17);

	ppu_on_all(); // turn on screen

	pal_bright(4);

	px = 128 << 8;
	py = (6 * 16) << 8;
	dx = P1_MOVE_SPEED;
	dy = 0;

	player1.pos_x = FP_WHOLE(128);
	player1.pos_y = FP_WHOLE(6 * 16);

	// infinite loop
	while (1)
	{
		++tick_count;

		ppu_wait_nmi(); // wait till beginning of the frame

		oam_clear();

		pads = pad_poll(0) | pad_poll(1); // read the first controller
		pads_new = get_pad_new(0) | get_pad_new(1); // newly pressed button. do pad_poll first

		clear_vram_buffer(); // do at the beginning of each frame

#if 1
		px_old = px;
		py_old = py;

		px += (pads & PAD_LEFT) ? dx >> 1 : dx;
		py += (pads & PAD_LEFT) ? dy >> 1 : dy;

		x = (high_byte(px)) + (dx > 0 ? 16 : 0);

		if (GET_META_TILE_FLAGS(GRID_XY_TO_ROOM_INDEX(x/16, (high_byte(py)-8)/16)) & FLAG_WALL)
		{
			dx *= -1;
		}

		index = GET_META_TILE_FLAGS(GRID_XY_TO_ROOM_INDEX((high_byte(px) + 8)/16, high_byte(py)/16));
		
		on_ramp = index & (FLAG_DOWN_LEFT | FLAG_DOWN_RIGHT);

		i = 0;
		if (((high_byte(px_old) + 8) / 16) != ((high_byte(px) + 8) / 16))
		{
			i = 1;
		}
		

		if (i && pads & PAD_DOWN)
		{
			if ((index & (FLAG_DOWN_RIGHT)) && (index & FLAG_ENTRY) && dx > 0)
			{
				dy = P1_MOVE_SPEED;
				temp_was_on_ramp = 1;
			}
			else if ((index & (FLAG_DOWN_LEFT)) && (index & FLAG_ENTRY) && dx < 0)
			{
				dy = P1_MOVE_SPEED;
				temp_was_on_ramp = 1;
			}
		}
		grounded = 0;
		if (dy >= 0 && index & FLAG_FLOOR && !on_ramp)
		{
			dy = 0;
			py = FP_WHOLE((((high_byte(py)) / 16)) * 16);
			grounded = 1;
		}

		if (!on_ramp)
		{
			dy += GRAVITY_LOW;
		}

		// if (index == 0)
		// {
		// 	dy = 0;
		// 	py = FP_WHOLE((((high_byte(py)) / 16)) * 16);
		// }

		index = GET_META_TILE_FLAGS(GRID_XY_TO_ROOM_INDEX((high_byte(px) + 8)/16, (high_byte(py)-16)/16));
		if (i && pads & PAD_UP)
		{
			if ((index & (FLAG_DOWN_RIGHT)) && (index & FLAG_ENTRY) && dx < 0)
			{
				dy = -P1_MOVE_SPEED;
				temp_was_on_ramp = 1;
			}
			else if ((index & (FLAG_DOWN_LEFT)) && (index & FLAG_ENTRY) && dx > 0)
			{
				dy = -P1_MOVE_SPEED;
				temp_was_on_ramp = 1;
			}
		}

		if (dx < 0)
		{
			in_oam_x = high_byte(px);
			in_oam_y = high_byte(py) - 16;
			in_oam_data = meta_player_list[0];
			c_oam_meta_spr_flipped();
		}
		else
		{
			oam_meta_spr(high_byte(px), high_byte(py) - 16, meta_player_list[0]);
		}

		if (pads_new & PAD_A && grounded)
		{
			sfx_play(0, 0);
			dy = -(FP_WHOLE(1) + FP_0_25);
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

		// if (pads & PAD_LEFT) --px;
		// if (pads & PAD_RIGHT) ++px;
		// if (pads & PAD_UP) --py;
		// if (pads & PAD_DOWN) ++py;
#else
	update_player();

		if (player1.facing_left)
		{
			in_oam_x = high_byte(player1.pos_x);
			in_oam_y = high_byte(player1.pos_y) - 1;
			in_oam_data = meta_player_list[0];
			c_oam_meta_spr_flipped();
		}
		else
		{
			oam_meta_spr(high_byte(player1.pos_x), high_byte(player1.pos_y) - 1, meta_player_list[0]);
		}	
#endif		
	}
}

void load_current_map(unsigned int nt)
{
	//static unsigned char* _current_room;
	
	// "const_cast"
	
	// NUM_CUSTOM_PROPS because the level data starts after the custom props
	memcpy(current_room, &rooms_maps_a[0/*cur_room_index*/][NUM_CUSTOM_PROPS], 240);	
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
	temp_was_on_ramp = on_ramp;
	on_ramp = 0;

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
				

				if (tempFlags & FLAG_DOWN_LEFT && (temp_was_on_ramp || player1.vel_x > 0))
				{
					jump_count = 0;
					grounded = 1;
					player1.pos_y = (((y + 1) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
					player1.vel_y = 0;
					airtime = 0;
					hit_kill_box = 0;
					on_ramp = 1;
					//player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
					//break;
				}
				else if (tempFlagsUp & FLAG_DOWN_LEFT && (temp_was_on_ramp || player1.vel_x > 0))
				{
					jump_count = 0;
					grounded = 1;
					player1.pos_y = (((y) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
					player1.vel_y = 0;
					airtime = 0;
					hit_kill_box = 0;
					on_ramp = 1;
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
					on_ramp = 1;
					//player1.pos_y = FP_WHOLE((((high_byte(player1.pos_y) - 1) / 16) * 16) + (16 - (high_byte(player1.pos_x) % 16)));
					//break;
				}
				else if ((tempFlags & FLAG_DOWN_LEFT) && (temp_was_on_ramp && player1.vel_x < 0))
				{
					jump_count = 0;
					grounded = 1;
					player1.pos_y = (((y + 1) << 4) - PLAYER_HEIGHT - 1 - (((high_byte(player1.pos_x) + 8) % 16))) << HALF_POS_BIT_COUNT;
					player1.vel_y = 0;
					airtime = 0;
					hit_kill_box = 0;
					on_ramp = 1;
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