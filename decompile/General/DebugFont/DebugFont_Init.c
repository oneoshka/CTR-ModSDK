#include <common.h>

// Does not fit byte budget
void DECOMP_DebugFont_Init(struct GameTracker* gGT)
{
	struct Icon* debugFontIcon = gGT->ptrIcons[0x42];
	
	if(debugFontIcon == 0) return;
	
	sdata->debugFont.UV = *(short*)&debugFontIcon->texLayout.u0;
	sdata->debugFont.clut = debugFontIcon->texLayout.clut;
	sdata->debugFont.tpage = debugFontIcon->texLayout.tpage;
}
