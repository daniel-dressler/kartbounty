#ifndef __RENDERING__
#define __RENDERING__

#include <map>

#include "../../Standard.h"
#include "../entities/entities.h"
#include "../events/events.h"

#include "SELib/SELib.h"
#include "glhelpers.h"
#include "ShaderStructs.h"

#define RS_START	0
#define RS_DRIVING	3
#define RS_END		5

class Renderer
{
private:
	Int32				m_bInitComplete;
	SDL_Window*			m_Window;
	Events::Mailbox*	m_pMailbox;

	Int32				m_nSplitScreen;
	Int32				m_nScreenState;

	Real				m_fTime;

	GLeffect			m_eftMesh;
	GLeffect			m_eftGUI;
	GLbuffer			m_bufPerMesh;
	GLbuffer			m_bufPerFrame;
	GLbuffer			m_bufGUI;


	// Arena
	GLmesh				m_mshArenaCldr;

	GLmesh				m_mshArenaWalls;
	GLtex				m_difArenaWalls;
	GLtex				m_nrmArenaWalls;

	GLmesh				m_mshArenaFlags;
	GLtex				m_difArenaFlags;
	GLtex				m_nrmArenaFlags;

	GLmesh				m_mshArenaTops;

	GLmesh				m_mshArenaFloor;
	GLtex				m_difArenaFloor;
	GLtex				m_nrmArenaFloor;

	GLmesh				m_mshKart;
	GLmesh				m_mshKartTire;
	GLmesh				m_mshKartShadow;
	GLtex				m_difKartShadow;

	GLmesh				m_mshGold;
	GLmesh				m_mshBullet;

	// Power ups
	GLmesh				m_mshPowerRing1;
	GLmesh				m_mshPowerRing2;
	GLmesh				m_mshPowerSphere;

	// GUI
	GLmesh				m_mshGUIStart;
	GLtex				m_texGUIStart;
	GLtex				m_texGUIScore;

	GLtex				m_texGUINumbers;
	GLtex				m_texGUIPlayer;

	GLtex				m_difBlank;
	GLtex				m_nrmBlank;

	Vector3				m_vArenaOfs;

	void _DrawArena();
	void _DrawArenaQuad( Vector3 vColor );
	void _DrawScoreBoard( Int32 x, Int32 y, Int32 player );
	void _DrawScore( Int32 kart, Int32 x, Int32 y );

	// Local knowledge of karts
	struct kart {
		entity_id idKart;
		Vector4 vColor;
	};
	std::map<entity_id, struct kart> m_mKarts;


	// Local knowldge of powerups
	struct powerup {
		powerup_id_t idPowerup;
		Entities::powerup_t type;
		Vector3 vPos;
	};
	std::map<powerup_id_t, struct powerup> m_powerups;

public:
	int setup();
	int render( float fElapseSec );

	Renderer();
	~Renderer();
};

#endif
