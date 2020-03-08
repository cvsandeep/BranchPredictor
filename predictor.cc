/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

int global_predict_bit;

#include "predictor.h"
unsigned int *localpredictor;
bool get_local_prdediction(void);
void update_local_predictor(bool taken);

inline bool get_local_prdediction(void) {

	if((*localpredictor & 0x4) >> 2) {
		return true;
	} else {
		return false;
	}
}

inline void update_local_predictor(bool taken) {
	if (taken) {
		if (*localpredictor < 7)
			*localpredictor = *localpredictor + 1;
	} else {
		if(*localpredictor)
			*localpredictor = *localpredictor - 1;
	}
}

    inline bool get_global_prediction(void){

    	bool prediction = false;

    	//get_global_predict_bit();

    	if (global_predict_bit & 0x3 >> 1){
    		prediction = false;
    	}
    	else
    		prediction = true;

    	return prediction;
	}
	inline void update_global_predictor(bool taken){

		if(taken == true && global_predict_bit < 3){
			global_predict_bit++;
		}
		else if(taken == false && global_predict_bit > 0){
			global_predict_bit--;
		}

	}


    bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os)
        {

		/* replace this code with your own */
            bool prediction = false;

			printf("%0x %0x %1d %1d %1d %1d ",br->instruction_addr,
			                                br->branch_target,br->is_indirect,br->is_conditional,
											br->is_call,br->is_return);
            //if (br->is_conditional)
            //    prediction = true;

            //prediction = get_local_prdediction();
            return prediction;   // true for taken, false for not taken
        }

    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
		/* replace this code with your own */
			printf("%1d\n",taken);
			update_local_predictor(taken);

        }
