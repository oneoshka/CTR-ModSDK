#include <common.h>

void DECOMP_AH_Pause_Destroy(void)
{
    char i;
    struct Instance *inst;
    struct Thread *t;
    int iVar3;

	// global -> register
	struct PauseObject* ptrPauseObject =
		D232.ptrPauseObject;

	// set global
    D232.ptrPauseObject = 0;

	// check register
	if (ptrPauseObject == 0)
		return;

    // loop through 14 instances, destroy them
    for (i = 0; i < 0xe; i++)
    {
        INSTANCE_Death(
			ptrPauseObject->PauseMember[i].inst);
    }

	// kill thread
    ptrPauseObject->t->flags |= 0x800;
}