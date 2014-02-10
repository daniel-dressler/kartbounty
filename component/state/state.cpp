#include "../rendering/SELib/SELib.h"
#include "state.h"

StateData g_state;

StateData& GetState()
{
	return g_state;
}

StateData *GetMutState()
{
	return &g_state;
}

StateData::StateData()
{
	MEMSET( this, 0, sizeof(*this) );
}

StateData::~StateData()
{
	if( bttmArena )
		delete bttmArena;
}
