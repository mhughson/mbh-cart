#ifndef D12B32D3_39DD_4865_9236_3AEDEF9ACEDE
#define D12B32D3_39DD_4865_9236_3AEDEF9ACEDE

// There should be no globals here; just function prototypes.
// Non constant (RAM) variables go in common header.
// Constant (ROM) variables go in this c file, so that they are no accessed by accident by other files.

void load_screen_boot();
void load_screen_title();
void load_screen_rules();
void load_screen_gameover();
void load_screen_levelcomplete();

void update_gameplay();
void draw_player();

// params: x (object to start at)
// output: loaded_obj_index
//         loaded_obj_id 
void get_obj_id();
// params: in_nametable
void load_current_map();

#endif /* D12B32D3_39DD_4865_9236_3AEDEF9ACEDE */
