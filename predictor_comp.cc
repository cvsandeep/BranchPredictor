/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"
#include <bitset>
#include <cstdlib>
#include <time.h>
//MAX-32800 bits
/*
BAse- 2^8 x 2  =2^9 = 512 bits + 150 = 662
Tage - 2^12 enteries and 13 bit * 2 = 106496 bits;
---- 13KB ----*/
/*
BAse- 2^8 x 2  =2^9 = 512 bits + 150 = 662
Tage - 2^12 enteries and 7 bit * 2 = 57344 bits;
---- 5KB ----*/

/*
Base- 2^8 x 2  =2^9 = 512 bits + 150 = 662
Tage - 2^11 enteries and 8 bit * 2 = 32768 bits;
---- 4KB ----*/

/*
Base- 2^11 x 2  =2^12 = 4096 bits + 150 = 4246
Tage - 2^11 enteries and 7 bit * 2 = 32768 bits;
---- =32768 bits = 4KB ----*/

#define BASE_ADDR_BITS 11
static std::bitset<2> BasePredictor[1<<BASE_ADDR_BITS];

#define TOTAL_STAGES 2
#define TAG_ADDR_BITS 11  //12-bit:0.83; 10-bit:2.45; 8bit:7.765
struct TAGE  // Each entry in the tag Pred Table is 13 bits
{
    std::bitset<2> Predictor;
    std::bitset<5> tag;  // 3-bit:4.5; 5-bit:1.385; 7-bit:0.857 8-bit:0.832
    std::bitset<2> ctr;  // Temporary local counter
};
static TAGE TagLoc[TOTAL_STAGES][1<<TAG_ADDR_BITS];

static std::bitset<4> StageChooser (8);
static std::bitset<16> PathHistory;
static std::bitset<131> GlobalHistory;

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
 *******************GLOBAL HISTORY  ***********************************
 **********************************************************************/

inline void update_path_history(unsigned int pc_index, bool taken) {
	PathHistory =  (PathHistory << 1);
  PathHistory[0] = taken;
}

inline void update_global_history(bool taken) {
	GlobalHistory =  (GlobalHistory << 1);
	GlobalHistory[0] = taken;
}



/**********************************************************************
 *******************TAGE PREDICTOR **************************************
 **********************************************************************/

// Folded History implementation ... from ghr (HistoryLen length) -> compressed(target)
struct FoldHistory
{
    uint32_t HistLength;
    uint32_t FoldComp;
    void UpdateFoldHist(std::bitset<131> ghr)
    {
        // Xor bits of chunk of Global history into a byte 
        int mask = 0xFF;
        int mask1 = ghr[HistLength] << (HistLength % 8);
        int mask2 = 0x100;
            FoldComp  = (FoldComp << 1) + ghr[0];
            FoldComp ^= ((FoldComp & mask2) >> 8);
            FoldComp ^= mask1;
            FoldComp &= mask;

    }
};


 uint32_t HistoryLen[TOTAL_STAGES] = {130,44};
 static FoldHistory IndexBuff[TOTAL_STAGES];  // For indexing the tag
 static FoldHistory TagBuff[2][TOTAL_STAGES]; // Tag value

inline void init_tage_predictor(void){

    for(int j = 0; j < 2 ; j++)
    {
        for(int i=0; i < TOTAL_STAGES; i++)
        {
            //Updating history length for index and tag.
            TagBuff[j][i].HistLength = HistoryLen[i];
            IndexBuff[i].HistLength = HistoryLen[i];
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

        // CSR bits ^ pc[9 : 0] ^ pc[19 : 10] nextstage  pc[9 : 0] ^ pc[19 : 10] ^ h[9 : 0].
        TagIndex[0] = PC ^ (PC >> TAG_ADDR_BITS) ^ IndexBuff[0].FoldComp ^ PathHistory.to_ulong() ^ (PathHistory.to_ulong() >> TAG_ADDR_BITS);
        TagIndex[1] = PC ^ (PC >> (TAG_ADDR_BITS - 1)) ^ IndexBuff[1].FoldComp ^ (PathHistory.to_ulong() );
        for(int i = 0; i < TOTAL_STAGES; i++) {
        //tag = pc[7 : 0] ^ CSR1 ^ (CSR2 << 1).
         tag[i] = PC ^ TagBuff[0][i].FoldComp ^ (TagBuff[1][i].FoldComp << 1);
         tag[i] &= 0x1FF;
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
              NextStagePred = TagLoc[NextStage][TagIndex[NextStage]].Predictor[1];
              break;
          }
      }

    if(FirstStage < TOTAL_STAGES)
    {
        // Check if it is in strong STate
        // Check if this stage is choosen
        // Not a new entry
        if((TagLoc[FirstStage][TagIndex[FirstStage]].Predictor  != 1) ||(TagLoc[FirstStage][TagIndex[FirstStage]].Predictor != 2 ) || (TagLoc[FirstStage][TagIndex[FirstStage]].ctr != 0) || (!StageChooser[3]))
        {
            FirstStagePred = TagLoc[FirstStage][TagIndex[FirstStage]].Predictor[1];
            return FirstStagePred;
        }
    }
    return NextStagePred;
}

inline void update_tage_predictor(uint32_t PC, bool taken){
  bool FoundSpace = false;
  bool new_entry = 0;
  bool ActualPred = get_tage_predictor(PC);
  
     update_global_history(taken);   // Global History used for tag  for index and tag
     update_path_history(PC, taken); // Path History is only 12 bit is used for indexing
     
     for (int i = 0; i < TOTAL_STAGES; i++)
    {
            IndexBuff[i].UpdateFoldHist(GlobalHistory);
            TagBuff[0][i].UpdateFoldHist(GlobalHistory);
            TagBuff[1][i].UpdateFoldHist(GlobalHistory);

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
	     if((TagLoc[FirstStage][TagIndex[FirstStage]].ctr == 0) &&((TagLoc[FirstStage][TagIndex[FirstStage]].Predictor  == 1) || (TagLoc[FirstStage][TagIndex[FirstStage]].Predictor  == 2)))
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

    // If mispredicted check in previous stages
    if((!new_entry) || (new_entry && (FirstStagePred != taken)))
    {
  	    if (((ActualPred != taken) & (FirstStage > 0)))
  		  {
  			  for (int i = 0; i < FirstStage; i++)
  			  {
      			if (TagLoc[i][TagIndex[i]].ctr == 0){
      					FoundSpace = true;
            }
  			  }
  	// If no entry useful than decrease useful bits of all entries but do not allocate
  			if (FoundSpace == false)
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
    									TagLoc[i][TagIndex[i]].Predictor = 2;
    								}
    								else
    								{
    									TagLoc[i][TagIndex[i]].Predictor = 1;
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
