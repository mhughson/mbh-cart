/** (C) Matt Hughson 2020 */

// Note: all variables need extern and should be defined in main.c

#ifndef ONCE_MAIN_H
#define ONCE_MAIN_H

#define DEBUG_ENABLED 0

#if DEBUG_ENABLED
#define PROFILE_POKE(val) //POKE((0x2001),(val));
#else
#define PROFILE_POKE(val)
#endif

#define PROF_CLEAR 0x1e // none
#define PROF_R_TINT 0x3e // red
#define PROF_G_TINT 0x5e // green
#define PROF_B_TINT 0x9e // blue
#define PROF_W_TINT 0x1e // white
#define PROF_R 0x3f // red + grey
#define PROF_G 0x5f // green + grey
#define PROF_B 0x9f // blue + grey
#define PROF_W 0x1f // white + grey

#define ROOM_WIDTH_PAGES (1)
#define ROOM_WIDTH_PIXELS (256*ROOM_WIDTH_PAGES)
#define ROOM_WIDTH_TILES (16*ROOM_WIDTH_PAGES)
#define GRID_XY_TO_ROOM_INDEX(x,y) (((y) * ROOM_WIDTH_TILES) + (x))
#define CELL_SIZE (16)

#define META_TILE_FLAGS_OFFSET (5)
#define META_TILE_NUM_BYTES (6)
#define END_OF_META (128)
#define GET_META_TILE_FLAGS(room_table_index) metatiles[current_room[(room_table_index)] * META_TILE_NUM_BYTES + META_TILE_FLAGS_OFFSET]


// Fixed Point Math Helpers
#define FP_0_05 (13)
#define FP_0_10 (26)
#define FP_0_15 (38)
#define FP_0_20 (51)
#define FP_0_25 (64)
#define FP_0_50 (128)
#define FP_0_75 (192)
#define HALF_POS_BIT_COUNT (8)
#define FP_WHOLE(x) ((x)<<HALF_POS_BIT_COUNT)

// Tunables
#define JUMP_VEL (FP_WHOLE(2) + 0) // 2
#define JUMP_HOLD_MAX (10) // 10
#define GRAVITY (FP_0_25) // .25
#define GRAVITY_LOW (FP_0_05)
#define WALK_SPEED (FP_WHOLE(1) + FP_0_50)
#define JUMP_COYOTE_DELAY (8)
#define ATTACK_LEN (5)

typedef struct anim_info
{
	// index into sprite_anims array.
	unsigned char anim_current;
	unsigned char anim_queued;

	// how many ticks have passed since the last frame change.
	unsigned char anim_ticks;

	// the currently displaying frame of the current anim.
	unsigned char anim_frame;
} anim_info;

// data speciic to player game objects.
typedef struct animated_sprite
{
	// Was the last horizontal move *attempted* in the left direction?
	unsigned char facing_left;

	// Stores all of the active animation info.
	anim_info anim;

} animated_sprite;

typedef struct game_actor
{
	animated_sprite sprite;

	unsigned int pos_x;
	unsigned int pos_y;

    signed int vel_x;
	signed int vel_y;

	unsigned char facing_left;
} game_actor;

#pragma bss-name(push, "ZEROPAGE")

extern unsigned char i;
extern unsigned char index;
extern unsigned int index16;
extern unsigned char x;
extern unsigned char y;
extern unsigned char tick_count;
extern unsigned char pads;
extern unsigned char pads_new;
extern unsigned int px_old;
extern unsigned int py_old;
extern unsigned char in_oam_x;
extern unsigned char in_oam_y;
extern const unsigned char *in_oam_data;
extern game_actor player1;
extern unsigned int temp16;
extern unsigned char tempFlags;
extern unsigned char tempFlagsDown;
extern unsigned char tempFlagsUp;
extern unsigned char ticks_since_attack;
extern unsigned char temp_on_ramp;
extern unsigned char is_jumping;
extern unsigned char is_on_ramp;
extern unsigned char current_room[240];
// Used by the anim functions to avoid passing in a parameter.
extern anim_info* global_working_anim;
extern unsigned char score;
extern char in_x_tile; 
extern char in_y_tile;

// batch add
extern unsigned char anim_index;
extern unsigned char grounded;
extern unsigned char jump_held_count;
extern unsigned char can_jump;
extern unsigned char airtime;
extern unsigned char ticks_down;
extern unsigned char jump_count;
extern unsigned char on_ground;
extern unsigned char new_jump_btn;
extern unsigned int scroll_y;

#pragma bss-name(pop)

void c_oam_meta_spr_flipped(void);
unsigned char update_anim();
void queue_next_anim(unsigned char next_anim_index);
void commit_next_anim();
void vram_buffer_load_2x2_metatile();

// TODO: Where is non-zero page? Is this just starting at zero page?

#endif // ONCE_MAIN_H