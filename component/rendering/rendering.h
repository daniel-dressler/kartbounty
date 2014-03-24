#ifndef __RENDERING__
#define __RENDERING__

#include <map>

#include "../../Standard.h"
#include "../entities/entities.h"
#include "../events/events.h"

#include "SELib/SELib.h"
#include "glhelpers.h"
#include "ShaderStructs.h"

class Renderer
{
private:
	Int32				m_bInitComplete;
	SDL_Window*			m_Window;
	Events::Mailbox*	m_pMailbox;

	Int32				m_nSplitScreen;

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
	GLtex				m_difArenaTops;
	GLtex				m_nrmArenaTops;

	GLmesh				m_mshArenaFloor;
	GLtex				m_difArenaFloor;
	GLtex				m_nrmArenaFloor;

	GLmesh				m_mshKart;
	GLmesh				m_mshKartTire;

	GLmesh				m_mshGold;
	GLmesh				m_mshBullet;

	// Power ups
	GLmesh				m_mshPowerRing1;
	GLmesh				m_mshPowerRing2;
	GLmesh				m_mshPowerSphere;

	GLmesh				m_mshGUIStart;
	GLtex				m_texGUIStart;

	Vector3				m_vArenaOfs;

	void _DrawArena();
	void _DrawArenaQuad( Vector3 vColor );

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

	bool atStartMenu;

public:
	int setup();
	int render( float fElapseSec );

	Renderer();
	~Renderer();
};

#endif
