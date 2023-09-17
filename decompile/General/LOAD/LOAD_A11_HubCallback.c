#include <common.h>

void DECOMP_LOAD_HubCallback(struct LoadQueueSlot* lqs)
{	
	sdata->load_inProgress = 0;
	LOAD_Callback_LEV_Adv(lqs);
	
	struct GameTracker* gGT = sdata->gGT;	
	gGT->level2 = sdata->ptrLEV_DuringLoading;
	MEMPACK_SwapPacks(gGT->activeMempackIndex);
}