/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"
static unsigned int LocalHistory[1024],LocalPredictor[1024], GlobalPredictor[4096], ChoicePredictor[4096];
static unsigned int PathHistory;

inline void update_local_history(unsigned int pc, bool taken) {
	LocalHistory[(pc>>2)&0x3FF] =  (LocalHistory[(pc>>2)&0x3FF] << 1) | taken;
}

inline void update_path_history(bool taken) {
	PathHistory =  (PathHistory << 1) | taken;
}

inline unsigned get_local_history(unsigned int pc) {
	return LocalHistory[(pc>>2)&0x3FF];
}

inline unsigned get_path_history(void) {
	return PathHistory & 0xFFF;
}

inline bool get_local_prediction(unsigned int pc) {
	unsigned int x = get_local_history(pc);
	if((LocalPredictor[x] & 0x4) >> 2) {
		return true;
	} else {
		return false;
	}
}

inline void update_local_predictor(unsigned int pc, bool taken) {
	unsigned int x = get_local_history(pc);
	if (taken) {
		if (LocalPredictor[x] < 7)
			LocalPredictor[x]++;
	} else {
		if(LocalPredictor[x])
			LocalPredictor[x]--;
	}
}

inline bool get_global_prediction(void) {
	unsigned int x = get_path_history();
	if (GlobalPredictor[x] & 0x2 >> 1) {
		return true;
	}
	else {
		return false;
	}
}

inline void update_global_predictor(bool taken) {
	unsigned int x = get_path_history();
	if(taken == true && GlobalPredictor[x] < 3) {
		GlobalPredictor[x]++;
	}
	else if(taken == false && GlobalPredictor[x] > 0) {
		GlobalPredictor[x]--;
	}
}

inline bool get_choice_prediction(void) {
	unsigned int x = get_path_history();
	if (ChoicePredictor[x] & 0x2 >> 1) {
		return true;
	}
	else {
		return false;
	}
}

inline void update_choice_predictor(bool taken, unsigned int pc) {
	unsigned int x = get_path_history();
	if(get_local_prediction(pc) == taken && get_global_prediction() != taken ) {
		if(ChoicePredictor[x] < 3)
			ChoicePredictor[x]++;
	}
	else if(get_local_prediction(pc) != taken && get_global_prediction() == taken ) {
		if(ChoicePredictor[x] > 0)
			ChoicePredictor[x]--;
	}
}

    bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os)
        {

		/* replace this code with your own */
            bool prediction = false;

			printf("%0x %0x %1d %1d %1d %1d ",br->instruction_addr,
			                                br->branch_target,br->is_indirect,br->is_conditional,
											br->is_call,br->is_return);
            if (br->is_conditional)
                prediction = true;
            if(get_choice_prediction())
            	prediction = get_local_prediction(br->instruction_addr);
            else
            	prediction = get_global_prediction();
            return prediction;   // true for taken, false for not taken
        }

    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
		/* replace this code with your own */
			printf("%1d\n",taken);
			update_local_predictor(br->instruction_addr,taken);
			update_local_history(br->instruction_addr,taken);

			update_global_predictor(taken);
			update_choice_predictor(br->instruction_addr,taken);
			update_path_history(taken);
        }
