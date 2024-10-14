/* extensions.h EDITED BY CLAIRE TOSOLINI 1271302
	- Code from Lines 23 - 33: function prototypes for dead-end detection 
*/

#ifndef __EXTENSIONS__
#define __EXTENSIONS__


#include "utils.h"
#include "node.h"

//////////////////////////////////////////////////////////////////////
// Place the game colors into a set order
void game_order_colors(game_info_t* info, game_state_t* state);

//////////////////////////////////////////////////////////////////////
// Check for dead-end regions of freespace where there is no way to
// put an active path into and out of it. Any freespace node which
// has only one free neighbor represents such a dead end. For the
// purposes of this check, cur and goal positions count as "free".

int game_check_deadends(const game_info_t* info, const game_state_t* state);

int viable_move(const game_info_t *info, const game_state_t *state, int x, int y);

int cell_free_check(const game_info_t *info, const game_state_t *state, pos_t pos, int move_col);

int game_num_free_cell(int orig_col, const game_info_t* info, const game_state_t* state, int x, int y);

int is_current_head_pos_of_colour(int colour, const game_state_t *state, const game_info_t *info, pos_t pos);

int is_any_head_or_goal_pos(const game_state_t *state, const game_info_t *info, pos_t pos);


#endif
