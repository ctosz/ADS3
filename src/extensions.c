/* extensions.c EDITED BY CLAIRE TOSOLINI 1271302
	- Code from Line 122 onwards
	- Any comments beginning with / / * * 
*/

#include "extensions.h"
#include "options.h"

//** Definitions for Dead-End Detection: no magic numbers!
#define FREE_CELL 1
#define MIN_FREE_TO_NOT_DEADLOCK 2 //** for a cell to not be deadlocked, must have at least this number of free cells surrounding


//////////////////////////////////////////////////////////////////////
// For sorting colors

int color_features_compare(const void* vptr_a, const void* vptr_b) {

	const color_features_t* a = (const color_features_t*)vptr_a;
	const color_features_t* b = (const color_features_t*)vptr_b;

	int u = cmp(a->user_index, b->user_index);
	if (u) { return u; }

	int w = cmp(a->wall_dist[0], b->wall_dist[0]);
	if (w) { return w; }

	int g = -cmp(a->wall_dist[1], b->wall_dist[1]);
	if (g) { return g; }

	return -cmp(a->min_dist, b->min_dist);

}

//////////////////////////////////////////////////////////////////////
// Place the game colors into a set order

void game_order_colors(game_info_t* info,
                       game_state_t* state) {

	if (g_options.order_random) {
    
		srand(now() * 1e6);
    
		for (size_t i=info->num_colors-1; i>0; --i) {
			size_t j = rand() % (i+1);
			int tmp = info->color_order[i];
			info->color_order[i] = info->color_order[j];
			info->color_order[j] = tmp;
		}

	} else { // not random

		color_features_t cf[MAX_COLORS];
		memset(cf, 0, sizeof(cf));

		for (size_t color=0; color<info->num_colors; ++color) {
			cf[color].index = color;
			cf[color].user_index = MAX_COLORS;
		}
    

		for (size_t color=0; color<info->num_colors; ++color) {
			
			int x[2], y[2];
			
			for (int i=0; i<2; ++i) {
				pos_get_coords(state->pos[color], x+i, y+i);
				cf[color].wall_dist[i] = get_wall_dist(info, x[i], y[i]);
			}

			int dx = abs(x[1]-x[0]);
			int dy = abs(y[1]-y[0]);
			
			cf[color].min_dist = dx + dy;
			
		

		}


		qsort(cf, info->num_colors, sizeof(color_features_t),
		      color_features_compare);

		for (size_t i=0; i<info->num_colors; ++i) {
			info->color_order[i] = cf[i].index;
		}
    
	}

	if (!g_options.display_quiet) {

		printf("\n************************************************"
		       "\n*               Branching Order                *\n");
		if (g_options.order_most_constrained) {
			printf("* Will choose color by most constrained\n");
		} else {
			printf("* Will choose colors in order: ");
			for (size_t i=0; i<info->num_colors; ++i) {
				int color = info->color_order[i];
				printf("%s", color_name_str(info, color));
			}
			printf("\n");
		}
		printf ("*************************************************\n\n");

	}

}



//////////////////////////////////////////////////////////////////////
// Check for dead-end regions of freespace where there is no way to
// put an active path into and out of it. Any freespace node which
// has only one free neighbor represents such a dead end. For the
// purposes of this check, cur and goal positions count as "free".

int game_check_deadends(const game_info_t* info, const game_state_t* state) {

	// FILL CODE TO DETECT DEAD-ENDS

	//** get original coordinates of move just made in search.c
	//** following 2 lines from game_make_move in engine.c 
	int orig_x, orig_y;
	pos_get_coords(state->pos[state->last_color], &orig_x, &orig_y);

	for (int dir1 = DIR_LEFT; dir1 <= DIR_DOWN; dir1++) {

		//** get coords of new move 
		pos_t new_pos = pos_offset_pos(info, state->pos[state->last_color], dir1);
		int new_x, new_y;
		pos_get_coords(new_pos, &new_x, &new_y);

		int vmove = viable_move(info, state, new_x, new_y);

		if (vmove) {
			if (vmove == FREE_CELL) {

				//** check if current free (empty) cell is a dead end
				if (game_num_free_cell(state->last_color, info, state, new_x, new_y) < MIN_FREE_TO_NOT_DEADLOCK) {
					return 1;
				}

				//** check if any of its neighbours are dead ends 
				for (int dir2 = DIR_LEFT; dir2 <= DIR_DOWN; dir2++) {

						pos_t next_pos = pos_offset_pos(info, new_pos, dir2);
						int next_x, next_y;
						pos_get_coords(next_pos, &next_x, &next_y);	

						int vmove_2 = viable_move(info, state, next_x, next_y);

						if (vmove_2) {
							if (vmove_2 == FREE_CELL) {

								//** check if current free (empty) cell is a dead end 
								if (game_num_free_cell(state->last_color, info, state, next_x, next_y) < MIN_FREE_TO_NOT_DEADLOCK) {
									return 1;
								}
							}
						}
					}
			}

			//** check all other viable, but non-empty cells' neighbours for dead ends 
			for (int dir3 = DIR_LEFT; dir3 <= DIR_DOWN; dir3++) {

				pos_t next_pos = pos_offset_pos(info, new_pos, dir3);
				int next_x, next_y;
				pos_get_coords(next_pos, &next_x, &next_y);	

				int vmove_3 = viable_move(info, state, next_x, next_y);

				if (vmove_3) {
					if (vmove_3 == FREE_CELL) {
						
						//** check if current free (empty) cell is a dead end 
						if (game_num_free_cell(state->last_color, info, state, next_x, next_y) < MIN_FREE_TO_NOT_DEADLOCK) {
							return 1;
						}
					}
				}
			}
		}

		else {
			//** Move is not viable
		}
	}

	//** Dead-End not detected 
	return 0;
}
	

//** Function that determines whether a cell is 'viable'.
//** If the cell is not within the game board, do not need to check it NOR any of its neighbours 
//** If the cell is an empty cell, it AND its neighbours need to be checked 
//** If the cell is an initial, goal or current head, only its neighbours need to be checked 
//** Else: invalid 
//** Thank you to Yong En Foo and Grady Fitzpatrick for your explanations and clarifications on the 12-cell check!
int viable_move(const game_info_t *info, const game_state_t *state, int x, int y) {

	if (!coords_valid(info, x, y)) {
		return 0;
	}

	if (state->cells[pos_from_coords(x, y)] == TYPE_FREE) {
		return FREE_CELL;
	}

	for (int i = 0; i < info->num_colors; i++) {
		if (info->init_pos[i] == pos_from_coords(x, y)) {
			return 2;
		}
		if (info->goal_pos[i] == pos_from_coords(x, y)) {
			return 2;
		}
	}

	//** colour of current position:
	int col = cell_get_color(state->cells[pos_from_coords(x,y)]); 
	if ((state->completed & (1 << col))) { 
		return 0;
	}

	if (is_current_head_pos_of_colour(state->last_color, state, info, pos_from_coords(x,y))) {
		return 2;
	}

	return 0;

}


//** Function that checks if the cell input is 'free'. Used to determine is a cell is deadlocked 
int cell_free_check(const game_info_t *info, const game_state_t *state, pos_t pos, int move_col) {

	int x, y;
	pos_get_coords(pos, &x, &y);

	if (!coords_valid(info, x, y)) {
		return 0;
	}

	if (state->cells[pos] == TYPE_FREE) { 
		return FREE_CELL;
	}

	if (is_any_head_or_goal_pos(state, info, pos)) {
		return 1;
	}

	return 0;
}


//** Function that counts the number of free cells surrounding a given cell.
//** Modelled from game_num_free_coords in engine.c. 
int game_num_free_cell(int orig_col, const game_info_t* info, const game_state_t* state, int x, int y) {

	int num_free = 0;
  
	for (int dir = DIR_LEFT; dir <= DIR_DOWN; ++dir) {

		pos_t neighbor_pos = offset_pos(info, x, y, dir);
		int neighbor_x, neighbor_y;
		pos_get_coords(neighbor_pos, &neighbor_x, &neighbor_y);

		if (cell_free_check(info, state, neighbor_pos, orig_col)) {
			++num_free;
		}
	}

	return num_free;

}


//** Function that determines if the given position is the current head position of the given colour
int is_current_head_pos_of_colour(int colour, const game_state_t *state, const game_info_t *info, pos_t pos) {
	if ((state->pos[colour] == pos)) {
		return 1;
	}

	return 0;
}


//** Function that determines if the given position is any head or goal position of an UNCOMPLETED colour 
int is_any_head_or_goal_pos(const game_state_t *state, const game_info_t *info, pos_t pos) {

	if (((state->pos[cell_get_color(state->cells[pos])] == pos) || (info->goal_pos[cell_get_color(state->cells[pos])] == pos)) && (!(state->completed & (1 << cell_get_color(state->cells[pos])))) ) {
		return 1;
	}

	return 0;

}



                                         
