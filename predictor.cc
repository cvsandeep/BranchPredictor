/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

int global_predict_bit;

#include "predictor.h"


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
    	bool prediction = false;

    	//get_global_predict_bit();

    	//prediction = get_global_prediction();

    	return prediction;

        }



    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
		/* replace this code with your own */

    	update_global_predictor(taken);

        }
