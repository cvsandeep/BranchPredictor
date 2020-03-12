/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"
#include <bitset>

static std::bitset<2> BasePredictor[1024];

static std::bitset<10> LocalHistory[1024];
static std::bitset<2> LocalPredictor[1024];
static std::bitset<2> LocalTag[1024];

static std::bitset<2> GlobalPredictor[4096];
static std::bitset<2> GlobalTag[1024];
static std::bitset<12> PathHistory;

/**********************************************************************
 *******************HELPER FUNCTIONS***********************************
 **********************************************************************/
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
		if (!BasePredictor[pc_index].all())
			BasePredictor[pc_index] = increment(BasePredictor[pc_index]);
	} else {
		if(BasePredictor[pc_index].any())
			BasePredictor[pc_index] = decrement(BasePredictor[pc_index]);
	}
}

/**********************************************************************
 *******************LOCAL PREDICTOR ***********************************
 **********************************************************************/

inline bool get_local_prediction(unsigned int pc_index) {
	unsigned int x = LocalHistory[pc_index].to_ulong();
	return LocalPredictor[x][1];
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
	unsigned int x = PathHistory.to_ulong();
	return GlobalPredictor[x][1];
}

inline void update_global_history(bool taken) {
	PathHistory =  (PathHistory << 1);
	PathHistory[0] = taken;
}

inline void update_global_predictor(bool taken) {
	unsigned int x = PathHistory.to_ulong();
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
	// get_local_prediction(pc_index)  get_global_prediction();
 	unsigned int l_val = LocalHistory[pc_index].to_ulong();
	unsigned int g_val = PathHistory.to_ulong();
	if( GlobalTag[pc_index].to_ulong())
		return get_global_prediction();
	else if( LocalTag[l_val].to_ulong())
		return get_local_prediction(pc_index);
	else
		return get_base_prediction(pc_index);
}

inline void update_actual_predictor(unsigned int pc_index,bool taken, unsigned int pc_tag) {
	unsigned int l_val = LocalHistory[pc_index].to_ulong();
	unsigned int g_val = PathHistory.to_ulong();
	// get_local_prediction(pc_index)  get_global_prediction();
 
	//update_base_predictor(pc_index,taken);
	if(BasePredictor[pc_index].to_ulong()){
		if( get_base_prediction(pc_index) != taken) {
		LocalPredictor[l_val] = BasePredictor[pc_index];
		LocalTag[pc_index] = pc_tag;
		update_local_history(pc_index,taken);
		return;
		}
		else{
			update_base_predictor(pc_index,taken);
			return;
		}

	}
	else if( LocalTag[pc_index].to_ulong()) {
		if(get_local_prediction(l_val) == taken) {
			update_local_predictor(l_val,taken);
			update_local_history(pc_index,taken);
			return;
		} else {
			//Global = LOCAL
			GlobalPredictor[g_val] = LocalPredictor[l_val];
			GlobalTag[pc_index] = LocalTag[pc_index];
			LocalTag[pc_index][0] = 0; LocalTag[pc_index][1] = 0;
			update_global_predictor(taken);
			update_global_history(taken);
			return;
		}

	}
	else if(GlobalTag[pc_index].to_ulong()) {
		update_global_predictor(taken);
		update_global_history(taken);
		return;
	}
	else{
		update_base_predictor(pc_index,taken);
	}
	return;
}

    bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os)
        {

		/* replace this code with your own */
            bool prediction = false;
            unsigned int pc_index = (br->instruction_addr>>2) & 0x3FF;


            if (!br->is_conditional) {
                prediction = true;
            } else {
            	if (br->is_call) {   // Call push return address in stack
            		prediction = true;	// For now defaulting true
            	} else if(br->is_return) {   //If pop restore return address
            		prediction = true;	// For now defaulting true
            	} else {				// Its branch predicting as taken or not taken
            		 //Determine to use global or local predictors
					        prediction =  get_actual_prediction(pc_index);
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
			unsigned int pc_index = (br->instruction_addr>>2) & 0x3FF;
            unsigned int pc_tag = (br->instruction_addr>>22) & 0x3;
		//printf("IA: %0x\n", br->instruction_addr);
			update_actual_predictor(pc_index, taken, pc_tag);
        }
