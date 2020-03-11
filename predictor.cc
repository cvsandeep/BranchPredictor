/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"
#include <bitset>

static std::bitset<10> LocalHistory[1024];
static std::bitset<3> LocalPredictor[1024];
static std::bitset<2> GlobalPredictor[4096];
static std::bitset<2> ChoicePredictor[4096];
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
 *******************LOCAL PREDICTOR ***********************************
 **********************************************************************/

inline bool get_local_prediction(unsigned int pc_index) {
	unsigned int x = LocalHistory[pc_index].to_ulong();
	return LocalPredictor[x][2];
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
 *******************CHOICE PREDICTOR***********************************
 **********************************************************************/

inline bool get_choice_prediction(void) {
	unsigned int x = PathHistory.to_ulong();
	return ChoicePredictor[x][1];
}

inline void update_choice_predictor(unsigned short index, bool outcome) {
        //Get last prediction of local and global predictors
        bool localChoice = get_local_prediction(index);
        bool globalChoice = get_global_prediction();
        unsigned short historyIndex = PathHistory.to_ulong();
        //Cross check choices to actual outcome
        if (localChoice == outcome && globalChoice != outcome) {
                //Local Choice was correct
                if (ChoicePredictor[historyIndex].any())
                	ChoicePredictor[historyIndex] = decrement(ChoicePredictor[historyIndex]);
        } else if (localChoice != outcome && globalChoice == outcome) {
                //Global Choice was correct
                if (!ChoicePredictor[historyIndex].all())
                	ChoicePredictor[historyIndex] = increment(ChoicePredictor[historyIndex]);
        }
}

inline void update_path_history(bool taken) {
	PathHistory =  (PathHistory << 1);
	PathHistory[0] = taken;
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
					if (!get_choice_prediction()) {
							//Use Local Table
							prediction =  get_local_prediction(pc_index);
					} else {
							//Use Global Table
							prediction =  get_global_prediction();
					}
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
			update_choice_predictor(pc_index, taken);
			update_local_predictor(pc_index, taken);
			update_local_history(pc_index, taken);

			update_global_predictor(taken);
			update_path_history(taken);
        }
