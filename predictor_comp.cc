/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"
#include <bitset>

static std::bitset<2> BasePredictor[1024];

static std::bitset<10> LocalHistory[1024];
static std::bitset<2> LocalPredictor[1024];
static std::bitset<2> LocalTag[1024];

static std::bitset<2> GlobalPredictor[1024];
static std::bitset<2> GlobalTag[1024];
static unsigned int PathHistory;

/*
 * Helper Functions
 */
template <size_t N>
inline std::bitset<N> increment ( std::bitset<N> in ) {
//  add 1 to each value, and if it was 1 already, carry the 1 to the next.
    for ( size_t i = 0; i < N; ++i ) {
        if ( in[i] == 0 ) {  // There will be no carry
            in[i] = 1;
            break;
            }
        in[i] = 0;  // This entry was 1; set to zero and carry the 1
        }
    return in;
    }

template <size_t N>
inline std::bitset<N> decrement ( std::bitset<N> in ) {
//  add 1 to each value, and if it was 1 already, carry the 1 to the next.
    for ( size_t i = 0; i < N; ++i ) {
        if ( in[i] == 1 ) {  // Flip all bits until we find 1
            in[i] = 0;
            break;
            }
        in[i] = 1;  // This entry was 1; set to zero and carry the 1
        }
    return in;
    }

/**********************************************************************
 *******************BASE PREDICTOR ************************************
 **********************************************************************/

inline bool get_base_prediction(unsigned int pc_index) {
	if((BasePredictor[pc_index])[1]) {
		return true;
	} else {
		return false;
	}
}

inline void update_base_predictor(unsigned int pc_index, bool taken) {
	if (taken) {
		if (!LocalPredictor[pc_index].all())
			LocalPredictor[pc_index] = increment(LocalPredictor[pc_index]);
	} else {
		if(LocalPredictor[pc_index].any())
			LocalPredictor[pc_index] = decrement(LocalPredictor[pc_index]);
	}
}

/**********************************************************************
 *******************LOCAL PREDICTOR ***********************************
 **********************************************************************/

inline bool get_local_prediction(unsigned int pc_index) {
	unsigned int x = LocalHistory[pc_index].to_ulong();
	if((LocalPredictor[x])[1]) {
		return true;
	} else {
		return false;
	}
}

inline void update_local_history(unsigned int pc_index, bool taken) {
	LocalHistory[pc_index] <<= 1;
	LocalHistory[pc_index][0] = taken;
}

inline void update_local_predictor(unsigned int pc_index, bool taken) {
	unsigned int x = LocalHistory[pc_index].to_ulong();
	if (taken) {
		if (!LocalPredictor[x].all())
			LocalPredictor[x] = increment(LocalPredictor[x]);
	} else {
		if(LocalPredictor[x].any())
			LocalPredictor[x] = decrement(LocalPredictor[x]);
	}
}

/**********************************************************************
 *******************GLOBAL PREDICTOR***********************************
 **********************************************************************/
inline bool get_global_prediction(void) {
	unsigned int x = PathHistory & 0x3FF;
	if (GlobalPredictor[x][1]) {
		return true;
	}
	else {
		return false;
	}
}

inline void update_global_history(bool taken) {
	PathHistory =  (PathHistory << 1) | taken;
}

inline void update_global_predictor(bool taken) {
	unsigned int x = PathHistory & 0x3FF;
	if(taken == true && !GlobalPredictor[x].all()) {
		GlobalPredictor[x] = increment(GlobalPredictor[x]);

	}
	else if(taken == false && GlobalPredictor[x].any()) {
		GlobalPredictor[x] = decrement(GlobalPredictor[x]);
	}
}

/**********************************************************************
 *******************MY PREDICTOR **************************************
 **********************************************************************/
inline bool get_actual_prediction(unsigned int pc_index) {
	unsigned int x = pc_index & 0x3;
	// get_local_prediction(pc_index)  get_global_prediction();
	if( GlobalTag[x].to_ulong())
		return get_global_prediction();
	else if( LocalTag[x].to_ulong())
		return get_local_prediction(pc_index);
	else
		return get_base_prediction(pc_index);
}

inline void update_actual_predictor(unsigned int pc_index,bool taken) {
	unsigned int x = pc_index & 0x3;
	unsigned int val = LocalHistory[pc_index].to_ulong();
	// get_local_prediction(pc_index)  get_global_prediction();


	update_base_predictor(pc_index,taken);

	if( LocalTag[x].to_ulong()) {
		if(get_local_prediction(pc_index) == taken) {
			update_local_predictor(pc_index,taken);
			update_local_history(pc_index,taken);
		} else {
			//Global = LOCAL
			GlobalPredictor[val] = LocalPredictor[val];
			GlobalTag[val] = LocalTag[val];
			update_global_history(taken);
			return;
		}

	} else {
		//LOCAL = BASE
		LocalPredictor[val] = BasePredictor[val];
		LocalPredictor[val] = x;
		update_local_history(pc_index,taken);
		return;
	}

	if( GlobalTag[x].to_ulong()) {
		update_global_predictor(taken);
		update_global_history(taken);
	}
	return;
}

    bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os)
        {

		/* replace this code with your own */
            bool prediction = false;
            unsigned int pc_index = (br->instruction_addr<<2) & 0x3FF;


            if (!br->is_conditional) {
                prediction = true;
            } else {
            	//prediction = get_base_prediction(pc_index);
            	prediction = get_actual_prediction(pc_index);
				/*if(get_choice_prediction(pc_index)) {
					prediction = get_local_prediction(pc_index);
				} else {
					prediction = get_global_prediction();
				}*/
            }
            //prediction = false;
            return prediction;   // true for taken, false for not taken
        }

    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
		/* replace this code with your own */
			unsigned int pc_index = (br->instruction_addr<<2) & 0x3FF;

			//update_base_predictor(pc_index,taken);
			update_actual_predictor(pc_index,taken);
			/*update_local_history(pc_index,taken);
			update_local_predictor(pc_index,taken);

			update_global_predictor(taken);
			update_choice_predictor(pc_index,taken);
			update_path_history(taken);*/
        }
