#include <common.h>

void LOAD_LevelFile(int levelID);

void MainRaceTrack_StartLoad(short levelID)

{
  // clear backup,
  // keep music,
  // destroy "most" fx, let menu fx play to end
  howl_StopAudio(1,0,0);

  // deactivate pause
  ElimBG_Deactivate(sdata->gGT);

  LOAD_LevelFile(levelID);
  return;
}