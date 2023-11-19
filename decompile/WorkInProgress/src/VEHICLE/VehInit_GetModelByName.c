#include <common.h>

struct Model* VehInit_GetModelByName(char *searchName)
{
    struct Model *m;
    struct Model **models;
    int i;

    // array to character models loaded,
    // maximum of 4, used in VS mode
    models = (struct Model **)&data.driverModel_lowLOD[0];

    for (i = 0; i < 3; i++)
    {
        m = models[i];

        // 12/16 bytes is enough
        if (m != NULL &&
            (*(u_int *)&m->name[0] == *(u_int *)&searchName[0]) &&
            (*(u_int *)&m->name[4] == *(u_int *)&searchName[4]) &&
            (*(u_int *)&m->name[8] == *(u_int *)&searchName[8]))
        {
            // character found, return pointer
            return m;
        }
    }

    // if PLYROBJECTLIST is not nullptr
    if ((sdata->PLYROBJECTLIST != NULL) &&
        // make sure elements of PLYROBJECTLIST are valid
        (sdata->PLYROBJECTLIST[0] != NULL))
    {
        // PLYROBJECTLIST, used for arcade (6-8 players)
        models = sdata->PLYROBJECTLIST;

        // loop until all strings are checked (until current is not nullptr)
        for (i = 0; m = models[i], m != NULL; i++)
        {
			// 12/16 bytes is enough
            if ((*(u_int *)&m->name[0] == *(u_int *)&searchName[0]) &&
                (*(u_int *)&m->name[4] == *(u_int *)&searchName[4]) &&
                (*(u_int *)&m->name[8] == *(u_int *)&searchName[8]))
            {
                // character found, return pointer
                return m;
            }
        }
    }
    return NULL;
}