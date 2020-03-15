/* Author: Mark Faust;   
 * Description: This file defines the two required functions for the branch predictor.
*/

#include "predictor.h"
#include <bitset>
#include <cstdlib>
#include <time.h>

static std::bitset<2> BasePredictor[1<<14];

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

static inline uint32_t SatIncrement(uint32_t x, uint32_t max)
{
  if(x<max) return x+1;
  return x;
}

static inline uint32_t SatDecrement(uint32_t x)
{
  if(x>0) return x-1;
  return x;
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

/**********************************************************************
 *******************TAGE PREDICTOR **************************************
 **********************************************************************/

#define TOTAL_STAGES 3

/////////////// STORAGE BUDGET JUSTIFICATION ////////////////
// Total storage budget: 32KB + 17 bits
// Total PHT counters: 2^17 
// Total PHT size = 2^17 * 2 bits/counter = 2^18 bits = 32KB
// GHR size: 17 bits
// Total Size = PHT size + GHR size
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
// Each entry in the tag Pred Table
struct TAGE 
{
    std::bitset<3> Predictor;
    uint32_t tag;
    std::bitset<2> ctr;
};
// 2^12 * 14 bits * 3 + 2^15

#define TAGPREDLOG 12
static TAGE TagLoc[TOTAL_STAGES][1<<TAGPREDLOG];
uint32_t numTagPredEntries = (1 << TAGPREDLOG);


static uint32_t TagIndex[TOTAL_STAGES];
static uint32_t tag[TOTAL_STAGES];
  
static const std::bitset<2> one (1), two (2);
// Folded History implementation ... from ghr (geometric length) -> compressed(target)
struct CompressedHist
{
 /// Objective is to convert geomLengt of GHR into tagPredLog which is of length equal to the index of the corresponding bank
    // It can be also used for tag when the final length would be the Tag
    uint32_t geomLength;
    uint32_t targetLength;
    uint32_t compHist;
      
    void updateCompHist(std::bitset<131> ghr)
    {
        int mask = (1 << targetLength) - 1;
        int mask1 = ghr[geomLength] << (geomLength % targetLength);
        int mask2 = (1 << targetLength);
            compHist  = (compHist << 1) + ghr[0];
            compHist ^= ((compHist & mask2) >> targetLength);
            compHist ^= mask1;
            compHist &= mask;  
         
    }    
};

  std::bitset<131> GHR;           // global history register
  // 16 bit path history
  int PHR;
//  uint32_t  *bimodal;          // pattern history table
  uint32_t  historyLength; // history length

  //Tagged Predictors
  uint32_t geometric[TOTAL_STAGES];
  //Compressed Buffers
  CompressedHist indexComp[TOTAL_STAGES];
  CompressedHist tagComp[2][TOTAL_STAGES]; 
 
  // index had to be made global else recalculate for update

  uint32_t clock1;
  int clock_flip;
  int StageChooser;
    
inline void init_tage_predictor(void){

   // Next to initiating the taggedPredictors
    
    
    // Geometric lengths of history taken to consider correlation of different age.
    // Table 0 with the longest history as per PPM code
    geometric[0] = 130;
    geometric[1] = 44;
    geometric[2] = 15;
    //geometric[3] = 5;
    /*this gives 3.41 MPKI !!
     * geometric[0] = 200;
    geometric[1] = 80;
    geometric[2] = 20;
    geometric[3] = 5;*/ 
    
    // Initializing Compressed Buffers.
    // first for index of the the tagged tables
    for(int i = 0; i < TOTAL_STAGES; i++)
    {
        indexComp[i].compHist = 0;
        indexComp[i].geomLength = geometric[i]; 
        indexComp[i].targetLength = TAGPREDLOG;
    }
    
    // next for the tag themselves
    
        // The tables have different tag lengths
        // T2 and T3 have tag length -> 8
        // T0 and T1 have tag length -> 9
        // second index indicates the Bank no.
        // Reason for using two -> PPM paper... single 
  // CSR is sensitive to periodic patterns in global history which is a common case
    for(int j = 0; j < 2 ; j++)
    {
        for(int i=0; i < TOTAL_STAGES; i++)
        {
            tagComp[j][i].compHist = 0;
            tagComp[j][i].geomLength = geometric[i];
            if(j == 0)
            {
                if(i < 2)
                tagComp[j][i].targetLength = 9 ;
                else
                tagComp[j][i].targetLength = 9 ;    
            }
            else
            {
                if(i < 2)
                tagComp[j][i].targetLength = 8 ;
                else
                tagComp[j][i].targetLength = 8 ;
            }
        }   
    }    

       clock1 = 0;
       clock_flip = 1;
       PHR = 0;
       GHR.reset();
       StageChooser = 8;
}

// Predictions need to be global ?? recheck
bool FirstStagePred;
bool NextStagePred;
int FirstStage;
int NextStage;
  
inline bool get_tage_predictor(uint32_t PC) {      
      // Hash to get tag includes info about bank, pc and global history compressed   
    // formula given in PPM paper 
    // pc[9:0] xor CSR1 xor (CSR2 << 1)
    for(int i = 0; i < TOTAL_STAGES; i++) {
       tag[i] = PC ^ tagComp[0][i].compHist ^ (tagComp[1][i].compHist << 1);
       tag[i] &= ((1<<9)-1);
       /// These need to be masked
     // 9 bit tags for T0 and T1 // 8 bit tags for T2 and T3
    }
    
  // How to get index for each bank ??
  // bank 1
    // Hash of PC, PC >> index length important , GHR geometric, path info         
       TagIndex[0] = PC ^ (PC >> TAGPREDLOG) ^ indexComp[0].compHist ^ PHR ^ (PHR >> TAGPREDLOG);
       TagIndex[1] = PC ^ (PC >> (TAGPREDLOG - 1)) ^ indexComp[1].compHist ^ (PHR );
       TagIndex[2] = PC ^ (PC >> (TAGPREDLOG - 2)) ^ indexComp[2].compHist ^ (PHR & 31);
       //TagIndex[3] = PC ^ (PC >> (TAGPREDLOG - 3)) ^ indexComp[3].compHist ^ (PHR & 7);  // 1 & 63 gives 3.358 // shuttle 31 and 15: 3.250 //ece 31 and 1: 3.252
       /// These need to be masked   // shuttle : 1023 63 and 15: 3.254 // ece 1023, 31 and 1 : 3.254
       ////  63 and 7  and PC  -2 -4 -6 // 63 and 1 gives 3.256  // 63 and 7 s: // 31 and 7 : 3.243 best !
       for(int i = 0; i < TOTAL_STAGES; i++)
       {
            TagIndex[i] = TagIndex[i] & ((1<<TAGPREDLOG) - 1);
       }
       
       // Intialize Two stage before finding in the TAGE
       FirstStagePred = -1;
       NextStagePred = -1;
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
              break;
          }  
      }    
    
    if(NextStage < TOTAL_STAGES) // Found the value in the One of the stages
     {
         NextStagePred = TagLoc[NextStage][TagIndex[NextStage]].Predictor[2];
     } else {        
        NextStagePred = get_base_prediction(PC % (1<<14));
     }
 
    if(FirstStage < TOTAL_STAGES) // Found the value in the One of the stages
    {
        if((TagLoc[FirstStage][TagIndex[FirstStage]].Predictor.to_ulong()  != 3) ||(TagLoc[FirstStage][TagIndex[FirstStage]].Predictor.to_ulong() != 4 ) || (TagLoc[FirstStage][TagIndex[FirstStage]].ctr != 0) || (StageChooser < 8))
        {
            FirstStagePred = TagLoc[FirstStage][TagIndex[FirstStage]].Predictor[2];
            return FirstStagePred;
        }
    }
    return NextStagePred;
}

inline void update_tage_predictor(uint32_t PC, bool resolveDir){
  bool strong_old_present = false;
    bool new_entry = 0;    
    bool predDir = get_tage_predictor(PC);
    if (FirstStage < TOTAL_STAGES)
    {
        // As per update policy
        // 1st update the useful counter
        if ((predDir != NextStagePred))
        {
	    
	    if (predDir == resolveDir)
	    {

		TagLoc[FirstStage][TagIndex[FirstStage]].ctr = increment(TagLoc[FirstStage][TagIndex[FirstStage]].ctr);

	    }
	    else
	    {
		TagLoc[FirstStage][TagIndex[FirstStage]].ctr = decrement(TagLoc[FirstStage][TagIndex[FirstStage]].ctr);
	    }

	}    
	 // 2nd update the counters which provided the prediction  
        if(resolveDir)
        {
            TagLoc[FirstStage][TagIndex[FirstStage]].Predictor = increment(TagLoc[FirstStage][TagIndex[FirstStage]].Predictor);
        }
        else
        {
            TagLoc[FirstStage][TagIndex[FirstStage]].Predictor = decrement(TagLoc[FirstStage][TagIndex[FirstStage]].Predictor);
        }
    }
    else
    {
        update_base_predictor((PC) % (1<<14), resolveDir);
    }
    // Check if the current Entry which gave the prediction is a newly allocated entry or not.
	if (FirstStage < TOTAL_STAGES)
	{
	    
	    if((TagLoc[FirstStage][TagIndex[FirstStage]].ctr == 0) &&((TagLoc[FirstStage][TagIndex[FirstStage]].Predictor.to_ulong()  == 3) || (TagLoc[FirstStage][TagIndex[FirstStage]].Predictor.to_ulong()  == 4)))
            {
              new_entry = true;
          		if (FirstStagePred != NextStagePred)
          		  {
          		    if (NextStagePred == resolveDir)
                  {
                        // Alternate prediction more useful is a counter to be of 4 bits
                    		if (StageChooser < 15)
                    		{  
                            StageChooser++;
                        }    
                  } else if (StageChooser > 0) {
                                  StageChooser--;
                  }
                 }
	    }
	}


// Proceeding to allocation of the entry.
    if((!new_entry) || (new_entry && (FirstStagePred != resolveDir)))
    {    
	if (((predDir != resolveDir) & (FirstStage > 0)))
	  {
                    
	    for (int i = 0; i < FirstStage; i++)
	      {
		if (TagLoc[i][TagIndex[i]].ctr == 0);
                strong_old_present = true;

	      }
// If no entry useful than decrease useful bits of all entries but do not allocate
	    if (strong_old_present == false)
	    {
		for (int i = FirstStage - 1; i >= 0; i--)
		{
		    TagLoc[i][TagIndex[i]].ctr = decrement(TagLoc[i][TagIndex[i]].ctr);
                }
            }
	    else
	      {

                srand(time(NULL));
                int randNo = rand() % 100;
                int count = 0;
                int bank_store[TOTAL_STAGES - 1] = {-1, -1};
                int matchBank = 0;
                for (int i = 0; i < FirstStage; i++)
                {
                    if (TagLoc[i][TagIndex[i]].ctr == 0)
                    {
                        count++;
                        bank_store[i] = i;
                    }
                }  
                if(count == 1)
                {
                    matchBank = bank_store[0];
                }
                else if(count > 1)
                {
                     if(randNo > 33 && randNo <= 99)
                    {
                        matchBank = bank_store[(count-1)];
                    }
                    else
                    {
                        matchBank = bank_store[(count-2)];
                    }   
                }


		for (int i = matchBank; i > -1; i--)
		{
		    if ((TagLoc[i][TagIndex[i]].ctr == 0))
		      {
                        if(resolveDir)
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
                }

	    }

	}
    }    


// Periodic Useful bit Reset Logic ( Important so as to optimize compared to PPM paper)
	clock1++;
        //for every 256 K instruction 1st MSB than LSB
	if(clock1 == (256*1024))
        {
            // reset clock
            clock1 = 0;
            if(clock_flip == 1)
            {
                // this is the 1st time
                clock_flip = 0;
            }
            else
            {
                clock_flip = 1;
            }

	    if(clock_flip == 1)
            {// MSB turn
                for (int jj = 0; jj < TOTAL_STAGES; jj++)
                {    
                    for (uint32_t ii = 0; ii < numTagPredEntries; ii++)
                    {
                        TagLoc[jj][ii].ctr = TagLoc[jj][ii].ctr & one;
                    }
                }
            }    
            else
            {// LSB turn
                for (int jj = 0; jj < TOTAL_STAGES; jj++)
                   {    
                       for (uint32_t ii = 0; ii < numTagPredEntries; ii++)
                       {
                           TagLoc[jj][ii].ctr = TagLoc[jj][ii].ctr & two;
                       }
                   }

	    }

	}

	
  // update the GHR
  GHR = (GHR << 1);

  if(resolveDir){
    GHR.set(0,1); 
  }

    for (int i = 0; i < TOTAL_STAGES; i++)
    {
                
            indexComp[i].updateCompHist(GHR);
            tagComp[0][i].updateCompHist(GHR);
            tagComp[1][i].updateCompHist(GHR);
            
    }
  // PHR update is simple, jus take the last bit ??
    // Always Limited to 16 bits as per paper.
    PHR = (PHR << 1); 
    if(PC & 1)
    {
        PHR = PHR + 1;
    }
    PHR = (PHR & ((1 << 16) - 1));
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
