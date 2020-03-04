/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"



    bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os)
        {
		/* replace this code with your own */
            bool prediction = false;

			printf("%0x %0x %1d %1d %1d %1d ",br->instruction_addr,
			                                br->branch_target,br->is_indirect,br->is_conditional,
											br->is_call,br->is_return);
            if (br->is_conditional)
                prediction = true;
            return prediction;   // true for taken, false for not taken
        }











    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
		/* replace this code with your own */
			printf("%1d\n",taken);

        }
