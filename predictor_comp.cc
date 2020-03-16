/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"
#include <bitset>
#include <cstdlib>
#include <time.h>

/* Total size is 12K for 2 statges and 18k for 3 stages*/
#define BASE_ADDR_BITS 8
static std::bitset<2> BasePredictor[1<<BASE_ADDR_BITS];

#define TOTAL_STAGES 2
#define TAG_ADDR_BITS 12  //12-bit:0.83; 10-bit:2.45; 8bit:7.765
struct TAGE  // Each entry in the tag Pred Table is 13 bits
{
    std::bitset<3> Predictor;
    std::bitset<7> tag;  // 3-bit:4.5; 5-bit:1.385; 7-bit:0.857 8-bit:0.832
    std::bitset<2> ctr;
};
static TAGE TagLoc[TOTAL_STAGES][1<<TAG_ADDR_BITS];

static std::bitset<4> StageChooser (8);
static std::bitset<16> PathHistory;
static std::bitset<131> GlobalHistory;

static std::bitset<10> LocalHistory[1024];
static std::bitset<2> LocalPredictor[1024];
static std::bitset<2> LocalTag[1024];

static std::bitset<2> GlobalPredictor[4096];
static std::bitset<2> GlobalTag[1024];        


/**********************************************************************
 *******************HELPER FUNCTIONS***********************************
 **********************************************************************/
static const std::bitset<2> one (1), two (2);

template <size_t N>
inline std::bitset<N> increment ( std::bitset<N> in ) {
    if(in.all())
      return in;
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
    if(!in.any())
      return in;
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
			BasePredictor[pc_index] = increment(BasePredictor[pc_index]);
	} else {
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
			LocalPredictor[x] = increment(LocalPredictor[x]);
	} else {
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

inline void update_path_history(unsigned int pc_index, bool taken) {
	PathHistory =  (PathHistory << 1);
  if(pc_index & 1)
	  PathHistory[0] = 1;
}

inline void update_global_history(bool taken) {
	GlobalHistory =  (GlobalHistory << 1);
	GlobalHistory[0] = taken;
}

inline void update_global_predictor(bool taken) {
	unsigned int x = PathHistory.to_ulong();
	if(taken == true) {
		GlobalPredictor[x] = increment(GlobalPredictor[x]);

	}
	else if(taken == false) {
		GlobalPredictor[x] = decrement(GlobalPredictor[x]);
	}
}

/**********************************************************************
 *******************MY PREDICTOR **************************************
 **********************************************************************/
inline bool get_actual_prediction(unsigned int pc_index) {
	// get_local_prediction(pc_index)  get_global_prediction();
 	unsigned int l_val = LocalHistory[pc_index].to_ulong();
	//unsigned int g_val = PathHistory.to_ulong();
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
		LocalTag[l_val] = pc_tag;
		update_local_history(pc_index,taken);
		return;
		}
		else{
			update_base_predictor(pc_index,taken);
			return;
		}

	}
	else if( LocalTag[l_val].to_ulong()) {
		if(get_local_prediction(pc_index) == taken) {
			update_local_predictor(pc_index,taken);
			update_local_history(pc_index,taken);
			return;
		} else {
			//Global = LOCAL
			GlobalPredictor[g_val] = LocalPredictor[l_val];
			GlobalTag[pc_index] = LocalTag[l_val];
			LocalTag[l_val][0] = 0; LocalTag[l_val][1] = 0;
			update_global_predictor( taken);
			update_path_history(pc_index, taken);
			return;
		}

	}
	else if(GlobalTag[pc_index].to_ulong()) {
		update_global_predictor(taken);
		update_path_history(pc_index, taken);
		return;
	}
	else{
		update_base_predictor(pc_index,taken);
	}
	return;
}

/**********************************************************************
 *******************TAGE PREDICTOR **************************************
 **********************************************************************/

// Folded History implementation ... from ghr (geometric length) -> compressed(target)
struct FoldHistory
{
    uint32_t HistLength;
    uint32_t compHist;
    void UpdateFoldHist(std::bitset<131> ghr)
    {
        int mask = (1 << 8) - 1;
        int mask1 = ghr[HistLength] << (HistLength % 8);
        int mask2 = (1 << 8);
            compHist  = (compHist << 1) + ghr[0];
            compHist ^= ((compHist & mask2) >> 8);
            compHist ^= mask1;
            compHist &= mask;

    }
};

  //Tagged Predictors
  uint32_t geometric[TOTAL_STAGES] = {130,44};
  //Compressed Buffers
 static FoldHistory indexComp[TOTAL_STAGES];
 static FoldHistory tagComp[2][TOTAL_STAGES];

inline void init_tage_predictor(void){

    for(int j = 0; j < 2 ; j++)
    {
        for(int i=0; i < TOTAL_STAGES; i++)
        {
            tagComp[j][i].HistLength = geometric[i];
            indexComp[i].HistLength = geometric[i];
        }
    }
}

// Predictions need to be global ?? recheck
bool FirstStagePred;
bool NextStagePred;
int FirstStage;
int NextStage;

static uint32_t TagIndex[TOTAL_STAGES];
static uint32_t tag[TOTAL_STAGES];

inline bool get_tage_predictor(uint32_t PC) {

        TagIndex[0] = PC ^ (PC >> TAG_ADDR_BITS) ^ indexComp[0].compHist ^ PathHistory.to_ulong() ^ (PathHistory.to_ulong() >> TAG_ADDR_BITS);
        TagIndex[1] = PC ^ (PC >> (TAG_ADDR_BITS - 1)) ^ indexComp[1].compHist ^ (PathHistory.to_ulong() );
        for(int i = 0; i < TOTAL_STAGES; i++) {
         tag[i] = PC ^ tagComp[0][i].compHist ^ (tagComp[1][i].compHist << 1);
         tag[i] &= ((1<<9)-1);
         TagIndex[i] = TagIndex[i] & ((1<<TAG_ADDR_BITS) - 1);
        }

       // Intialize Two stage before finding in the TAGE
       FirstStagePred = -1;
       NextStagePred = get_base_prediction(PC % (1<<BASE_ADDR_BITS));
       FirstStage = TOTAL_STAGES;
       NextStage = TOTAL_STAGES;

       //Search for a Tag match in first stage
       for(int iterator = 0; iterator < TOTAL_STAGES; iterator++)
       {
            if(TagLoc[iterator][TagIndex[iterator]].tag == tag[iterator])
            {
                FirstStage = iterator;
                break;
            }
       }
       //Search for a Tag match in Next stage
      for(int iterator = FirstStage + 1; iterator < TOTAL_STAGES; iterator++)
      {
          if(TagLoc[iterator][TagIndex[iterator]].tag == tag[iterator])
          {
              NextStage = iterator;
              NextStagePred = TagLoc[NextStage][TagIndex[NextStage]].Predictor[2];
              break;
          }
      }

    if(FirstStage < TOTAL_STAGES)
    {
        if((TagLoc[FirstStage][TagIndex[FirstStage]].Predictor  != 3) ||(TagLoc[FirstStage][TagIndex[FirstStage]].Predictor != 4 ) || (TagLoc[FirstStage][TagIndex[FirstStage]].ctr != 0) || (!StageChooser[3]))
        {
            FirstStagePred = TagLoc[FirstStage][TagIndex[FirstStage]].Predictor[2];
            return FirstStagePred;
        }
    }
    return NextStagePred;
}

inline void update_tage_predictor(uint32_t PC, bool taken){
  bool strong_old_present = false;
  bool new_entry = 0;
  bool ActualPred = get_tage_predictor(PC);
  
     update_global_history(taken);
     update_path_history(PC, taken); 
     for (int i = 0; i < TOTAL_STAGES; i++)
    {
            indexComp[i].UpdateFoldHist(GlobalHistory);
            tagComp[0][i].UpdateFoldHist(GlobalHistory);
            tagComp[1][i].UpdateFoldHist(GlobalHistory);

    }   
    
    if (FirstStage >= TOTAL_STAGES) {
      update_base_predictor((PC) % (1<<BASE_ADDR_BITS), taken);
    } else {
	       // Update the Predictior
        if(taken)
        {
            TagLoc[FirstStage][TagIndex[FirstStage]].Predictor = increment(TagLoc[FirstStage][TagIndex[FirstStage]].Predictor);
        }
        else
        {
            TagLoc[FirstStage][TagIndex[FirstStage]].Predictor = decrement(TagLoc[FirstStage][TagIndex[FirstStage]].Predictor);
        }
        
        // Update the counters if both stages predictions are wrong.
        if ((ActualPred != NextStagePred))
        {
    			if (ActualPred == taken)
    			{
    			  TagLoc[FirstStage][TagIndex[FirstStage]].ctr = increment(TagLoc[FirstStage][TagIndex[FirstStage]].ctr);
    			}
    			else
    			{
    			  TagLoc[FirstStage][TagIndex[FirstStage]].ctr = decrement(TagLoc[FirstStage][TagIndex[FirstStage]].ctr);
    			}
        }
    }

    // Check if the current Entry which gave the prediction is a newly allocated entry or not.
	     if((TagLoc[FirstStage][TagIndex[FirstStage]].ctr == 0) &&((TagLoc[FirstStage][TagIndex[FirstStage]].Predictor  == 3) || (TagLoc[FirstStage][TagIndex[FirstStage]].Predictor  == 4)))
    		{
    		  new_entry = true;
    			if (FirstStagePred != NextStagePred)
    			  {
        				if (NextStagePred == taken)
        			  {
        						StageChooser=increment(StageChooser);
        			  } else {
    							  StageChooser=decrement(StageChooser);
        			  }
    			 }
    		}

    // IF mispredicted 
    if((!new_entry) || (new_entry && (FirstStagePred != taken)))
    {
  	    if (((ActualPred != taken) & (FirstStage > 0)))
  		  {
  			  for (int i = 0; i < FirstStage; i++)
  			  {
      			if (TagLoc[i][TagIndex[i]].ctr == 0){
      					strong_old_present = true;
            }
  			  }
  	// If no entry useful than decrease useful bits of all entries but do not allocate
  			if (strong_old_present == false)
  			{
      			for (int i = FirstStage - 1; i >= 0; i--)
      			{
      				TagLoc[i][TagIndex[i]].ctr = decrement(TagLoc[i][TagIndex[i]].ctr);
     				}
  			} else {
  					int count = 0;
  					int bank_store[TOTAL_STAGES - 1] = {-1};
  					int matchBank = 0;
  					for (int i = 0; i < FirstStage; i++)
  					{
  						if (TagLoc[i][TagIndex[i]].ctr == 0)
  						{
  							count++;
  							bank_store[i] = i;
  						}
  					}
            matchBank = bank_store[count-1];
  					
    				for (int i = matchBank; i > -1; i--)
    				{
    					if ((TagLoc[i][TagIndex[i]].ctr == 0))
    					  {
    								if(taken)
    								{
    									TagLoc[i][TagIndex[i]].Predictor = 4;
    								}
    								else
    								{
    									TagLoc[i][TagIndex[i]].Predictor = 3;
    								}
    									TagLoc[i][TagIndex[i]].tag = tag[i];
    									TagLoc[i][TagIndex[i]].ctr = 0;
    					        break; // Only 1 entry allocated
    					  }
    				} //End For
  			} // Else
  		} // End Misprediction
    } // End new_entry

    //Done
}


    bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os)
        {
        static bool init=true;
        if(init){
          init_tage_predictor();
          init=false;
        }
		/* replace this code with your own */
            bool prediction = false;
            //unsigned int pc_index = (br->instruction_addr>>2) & 0x3FF;

            if (!br->is_conditional) {
                prediction = true;
            } else {
            	if (br->is_call) {   // Call push return address in stack
            		prediction = true;	// For now defaulting true
            	} else if(br->is_return) {   //If pop restore return address
            		prediction = true;	// For now defaulting true
            	} else {				// Its branch predicting as taken or not taken
            		 //Determine to use global or local predictors
					        //prediction =  get_actual_prediction(pc_index);
                  prediction = get_tage_predictor(br->instruction_addr);
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
			//unsigned int pc_index = (br->instruction_addr>>2) & 0x3FF;
      //unsigned int pc_tag = (br->instruction_addr>>22) & 0x3;
		//printf("IA: %0x\n", br->instruction_addr);
			//update_actual_predictor(pc_index, taken, pc_tag);
      update_tage_predictor(br->instruction_addr, taken);
        }
