/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"
#include <bitset>

static std::bitset<10> LocalHistory[1024];
static std::bitset<3> LocalPredictor[1024];
static std::bitset<2> GlobalPredictor[4096];
static std::bitset<2> ChoicePredictor[4096];
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


inline bool get_local_prediction(unsigned int pc_index) {
	unsigned int x = LocalHistory[pc_index].to_ulong();
	if((LocalPredictor[x])[2]) {
		return true;
	} else {
		return false;
	}
}

inline bool get_global_prediction(void) {
	unsigned int x = PathHistory & 0xFFF;
	if (GlobalPredictor[x][1]) {
		return true;
	}
	else {
		return false;
	}
}

inline bool get_choice_prediction(void) {
	unsigned int x = PathHistory & 0xFFF;
	if (ChoicePredictor[x][1]) {
		return true;
	}
	else {
		return false;
	}
}

inline void update_local_history(unsigned int pc_index, bool taken) {
	LocalHistory[pc_index] <<= 1;
	LocalHistory[pc_index][0] = taken;
}

inline void update_path_history(bool taken) {
	PathHistory =  (PathHistory << 1) | taken;
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

inline void update_global_predictor(bool taken) {
	unsigned int x = PathHistory & 0xFFF;
	if(taken == true && !GlobalPredictor[x].all()) {
		GlobalPredictor[x] = increment(GlobalPredictor[x]);

	}
	else if(taken == false && GlobalPredictor[x].any()) {
		GlobalPredictor[x] = decrement(GlobalPredictor[x]);
	}
}

inline void update_choice_predictor(bool taken, unsigned int pc_index) {
	unsigned int x = PathHistory & 0xFFF;
	if(get_local_prediction(pc_index) == taken && get_global_prediction() != taken ) {
		if(!ChoicePredictor[x].all())
			ChoicePredictor[x] = increment(ChoicePredictor[x]);
	}
	else if(get_local_prediction(pc_index) != taken && get_global_prediction() == taken ) {
		if(ChoicePredictor[x].any())
			ChoicePredictor[x] = decrement(ChoicePredictor[x]);
	}
}

    bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os)
        {

		/* replace this code with your own */
            bool prediction = false;
            unsigned int pc_index = (br->instruction_addr>>2) & 0x3FF;


            if (!br->is_conditional) {
                prediction = true;
                printf("CONDITIONAL: %0x %0x %1d %1d %1d %1d ",br->instruction_addr,
                			                                br->branch_target,br->is_indirect,br->is_conditional,
                											br->is_call,br->is_return);
            } else {
            	if (br->is_call) {   // Call push return address in stack
            		prediction = true;	// For now defaulting true
            		//Stores the return address <- ra
            		// No change in br->branch_target
            		printf("CALL: %0x %0x %1d %1d %1d %1d ",br->instruction_addr,
            		                			                                br->branch_target,br->is_indirect,br->is_conditional,
            		                											br->is_call,br->is_return);
            	} else if(br->is_return) {   //If pop restore return address
            		prediction = true;	// For now defaulting true
            		// Restores the target address -> br->branch_target
            		// Direct or indirect
            		// The processor also trains the line predictor with the address of jumps and subroutine calls that use direct register addressing.
            		printf("RETURN: %0x %0x %1d %1d %1d %1d ",br->instruction_addr,
            		                			                                br->branch_target,br->is_indirect,br->is_conditional,
            		                											br->is_call,br->is_return);
            	} else {				// Its branch predicting as taken or not taken
            		printf("BRANCH: %0x %0x %1d %1d %1d %1d ",br->instruction_addr,
            		                			                                br->branch_target,br->is_indirect,br->is_conditional,
            		                											br->is_call,br->is_return);
            		if(get_choice_prediction())
						prediction = get_local_prediction(pc_index);
					else
						prediction = get_global_prediction();

            		//If not taken branch_taget address is pc+4
            	}
            }

            return prediction;   // true for taken, false for not taken
        }

    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
		/* replace this code with your own */
			printf("%1d\n",taken);
			unsigned int pc_index = (br->instruction_addr>>2) & 0x3FF;
			update_local_predictor(pc_index,taken);
			update_local_history(pc_index,taken);

			update_global_predictor(taken);
			update_choice_predictor(pc_index,taken);
			update_path_history(taken);
        }
