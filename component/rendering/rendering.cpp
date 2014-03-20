#include <vector>
#include <functional>

#include "rendering.h"

Renderer::Renderer()
{
	m_bInitComplete = 0;
	m_Window = 0;
	m_fTime = 0;

	m_pMailbox = new Events::Mailbox();
	m_pMailbox->request( Events::EventType::Input );
	m_pMailbox->request( Events::EventType::StateUpdate );
	m_pMailbox->request( Events::EventType::PlayerKart );
	m_pMailbox->request( Events::EventType::KartCreated );
	m_pMailbox->request( Events::EventType::KartDestroyed );
	m_pMailbox->request( Events::EventType::PowerupPlacement );
	m_pMailbox->request( Events::EventType::PowerupDestroyed );
	m_pMailbox->request( Events::EventType::PowerupPickup );

	m_vArenaOfs = Vector3( -10,0,10 );
}

Renderer::~Renderer()
{

	glhDestroyMesh( m_mshArenaCldr );

	glhDestroyMesh( m_mshArenaWalls );
	glhDestroyTexture( m_difArenaWalls );
	glhDestroyTexture( m_nrmArenaWalls );

	glhDestroyMesh( m_mshArenaFlags );
	glhDestroyTexture( m_difArenaFlags );
	glhDestroyTexture( m_nrmArenaFlags );

	glhDestroyMesh( m_mshArenaTops );
	glhDestroyTexture( m_difArenaTops );
	glhDestroyTexture( m_nrmArenaTops );

	glhDestroyMesh( m_mshArenaFloor );
	glhDestroyTexture( m_difArenaFloor );
	glhDestroyTexture( m_nrmArenaFloor );

	glhDestroyMesh( m_mshKart );

	glhDestroyMesh( m_mshPowerRing1 );
	glhDestroyMesh( m_mshPowerRing2 );
	glhDestroyMesh( m_mshPowerSphere );

	glhDestroyBuffer( m_bufPerMesh );
	glhDestroyBuffer( m_bufPerFrame );
	glhUnloadEffect( m_eftMesh );
	SDL_DestroyWindow( m_Window );
	SDL_Quit();

	if( m_pMailbox )
		delete m_pMailbox;
}

int LoadMesh( GLmesh& mesh, char* strFilename )
{
	GLchar* pData;
	Int32 nSize;
	if( !glhReadFile( strFilename, pData, nSize ) )
		return 0;

	SEG::Mesh meshdata;
	if( !meshdata.ReadData( (Byte*)pData, nSize ) )
		return 0;

	free( pData );

	if( !glhCreateMesh( mesh, meshdata ) )
		return 0;

	return 1;
}

int Renderer::setup()
{

	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );

	SDL_Window *win = SDL_CreateWindow( GAMENAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
			);
	if( !win )
		exit(1);
	m_Window = win;

	SDL_GLContext glcontext = SDL_GL_CreateContext( win );
	if( !glcontext )
		exit(2);

	SDL_GL_MakeCurrent( win, glcontext );

	if( glewInit() != GLEW_OK )
		exit(4);

	// OpenGL Test Info
	//unused const GLubyte* strVendor = glGetString( GL_VENDOR );
	GLint major, minor;
	glGetIntegerv( GL_MAJOR_VERSION, &major );
	glGetIntegerv( GL_MINOR_VERSION, &minor );

	// OpenGL State Init
	glClearDepth( 1.0f );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_FRONT );
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
	glEnable( GL_MULTISAMPLE );

	// Load Standard Effects
	const GLchar* aryHeaders[] = { "component/rendering/ShaderStructs.glsl" };
	m_eftMesh = glhLoadEffect( "component/rendering/VShader.glsl", NULL, "component/rendering/PShader.glsl", aryHeaders, 1 );
	if( !m_eftMesh.program )
		exit(5);

	glhCreateBuffer( m_eftMesh, "cstPerMesh", sizeof(cstPerMesh), &m_bufPerMesh );
	glhCreateBuffer( m_eftMesh, "cstPerFrame", sizeof(cstPerFrame), &m_bufPerFrame );
	
	{
		GLchar* pData;
		Int32 nSize;
		if( !glhReadFile( "assets/arena_cldr.msh", pData, nSize ) )
			exit(6);

		SEG::Mesh meshdata;
		btTriangleMesh *arena_mesh = new btTriangleMesh();
		if( !meshdata.ReadData( (Byte*)pData, nSize, 0, arena_mesh, m_vArenaOfs ) )
			exit(7);

		// Send Arena to phsyics
		std::vector<Events::Event *> events;
		auto arena_event = NEWEVENT(ArenaMeshCreated);
		arena_event->arena = arena_mesh;
		events.push_back(arena_event);
		m_pMailbox->sendMail(events);

		free( pData );

		if( !glhCreateMesh( m_mshArenaCldr, meshdata ) )
			exit(8);
	}

	if( !LoadMesh( m_mshArenaWalls, "assets/arena_walls.msh" ) )
		exit(9);
	if( !glhLoadTexture( m_difArenaWalls, "assets/arena_walls_diff.png" ) )
		exit(10);
	if( !glhLoadTexture( m_nrmArenaWalls, "assets/arena_walls_norm.png" ) )
		exit(11);

	if( !LoadMesh( m_mshArenaFlags, "assets/arena_flags.msh" ) )
		exit(12);
	if( !glhLoadTexture( m_difArenaFlags, "assets/arena_flags_diff.png" ) )
		exit(13);
	if( !glhLoadTexture( m_nrmArenaFlags, "assets/arena_flags_norm.png" ) )
		exit(14);

	if( !LoadMesh( m_mshArenaFloor, "assets/arena_floor.msh" ) )
		exit(15);
	if( !glhLoadTexture( m_difArenaFloor, "assets/arena_floor_diff.png" ) )
		exit(16);
	if( !glhLoadTexture( m_nrmArenaFloor, "assets/blank_norm.png" ) )
		exit(17);

	if( !LoadMesh( m_mshArenaTops, "assets/arena_tops.msh" ) )
		exit(18);
	if( !glhLoadTexture( m_difArenaTops, "assets/blank.png" ) )
		exit(19);
	if( !glhLoadTexture( m_nrmArenaTops, "assets/blank_norm.png" ) )
		exit(20);


	if( !LoadMesh( m_mshKart, "assets/Kart.msh" ) )
		exit(21);
	
	if( !LoadMesh( m_mshPowerSphere, "assets/PowerSphere.msh" ) )
		exit(22);
	
	if( !LoadMesh( m_mshPowerRing1, "assets/PowerRing1.msh" ) )
		exit(23);
	
	if( !LoadMesh( m_mshPowerRing2, "assets/PowerRing2.msh" ) )
		exit(24);
	
	glhMapTexture( m_eftMesh, "g_texDiffuse", 0 );
	glhMapTexture( m_eftMesh, "g_texNormal", 1 );

	m_bInitComplete = 1;

	// Safe to delete?
	/*
	GetState().Karts[1].vPos = Vector3( 10,1.5,10 );							// These two lines are temporary
	GetState().Karts[1].qOrient.Identity().RotateAxisAngle(Vector3(0,1,0), DEGTORAD(-90));

	
	GetState().Karts[2].vPos = Vector3( -10,1.5,10 );							// These two lines are temporary
	GetState().Karts[2].qOrient.Identity().RotateAxisAngle(Vector3(0,1,0), DEGTORAD(90));
	
	GetState().Karts[3].vPos = Vector3( 10, 1.5, -10 );							// These two lines are temporary
	GetState().Karts[3].qOrient.Identity().RotateAxisAngle(Vector3(0,1,0), DEGTORAD(180));
	*/
	
	return 1;
}

#define expand_int(x) ((uint64_t)x << 31 ^ (uint64_t)x)
Vector4 getNextColor()
{
	static uint64_t color = 1;
	uint64_t red = 0;
	uint64_t blue = 0;
	uint64_t green= 0;

	// A table might be nicer
	switch (color++) {
	case 4:
		blue += 9000;
	case 3: 
		green += 10000;
	case 2:
		blue += 100;
	case 1:
		red += 1;
		break;
	default:
		red += rand();
		blue += rand();
		green += rand();
		break;
	}

	float largest = MAX(red, MAX(blue, green));

	return Vector4(red / largest, blue / largest, green / largest, 1);
}

int Renderer::update( float fElapseSec )
{
	m_fTime += fElapseSec;

	if( !m_bInitComplete )
		return 0;

	std::vector<Entities::CarEntity::Camera> cameras;
	for( Events::Event *event : m_pMailbox->checkMail() )
	{
		switch( event->type )
		{
		case Events::EventType::KartCreated:
			{
				auto kart_event = ((Events::KartCreatedEvent *)event);
				struct kart kart_local;
				entity_id id = kart_local.idKart = kart_event->kart_id;
				kart_local.vColor = getNextColor();
				m_mKarts[id] = kart_local;
			}
			break;
		case Events::EventType::KartDestroyed:
			m_mKarts.erase(((Events::KartDestroyedEvent *)event)->kart_id);
			break;
		case Events::EventType::PlayerKart:
			{
				entity_id kart_id = ((Events::PlayerKartEvent *)event)->kart_id;
				auto kart = GETENTITY(kart_id, CarEntity);
				cameras.push_back(kart->camera);
			}
			break;
		case Events::EventType::PowerupPlacement:
			{
				auto powerup_event = ((Events::PowerupPlacementEvent *)event);
				struct powerup powerup_local;
				powerup_local.vPos = powerup_event->pos;
				powerup_local.type = powerup_event->powerup_type;
				powerup_local.idPowerup = powerup_event->powerup_id;
				m_powerups[powerup_local.idPowerup] = powerup_local;
			}
			break;
		case Events::EventType::PowerupPickup:
		case Events::EventType::PowerupDestroyed:
			{
				auto powerup = ((Events::PowerupDestroyedEvent *)event);
				m_powerups.erase(powerup->powerup_id);
			}
			break;
		default:
			break;
		}
	}
	m_pMailbox->emptyMail();

	Int32 nWinWidth, nWinHeight;
	SDL_GetWindowSize( m_Window, &nWinWidth, &nWinHeight );
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;
	for (Entities::CarEntity::Camera camera : cameras) {
		Vector3 vFocus = camera.vFocus;
		perFrame.vEyePos = camera.vPos;
		perFrame.vEyeDir = Vector3( vFocus - perFrame.vEyePos.xyz() ).Normalize();
		perFrame.matProj.Perspective( DEGTORAD( camera.fFOV ), (Real)nWinWidth/nWinHeight, 0.1f, 100.0f );
		perFrame.matView.LookAt( perFrame.vEyePos.xyz(), vFocus, Vector3( 0, 1, 0 ) );
		perFrame.matViewProj = perFrame.matView * perFrame.matProj;
	}

	glClearColor( 59/255.0, 68/255.0, 75/255.0, 1 );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


	Real fLightDist = 17.0f;
	Real fLightHeight = 5.0f;
	Real fLightPower = 25.0f;
	Real fSpinRadius = 4.0f;
	MEMSET( perFrame.vLight, 0, sizeof(Vector4) * MAX_LIGHTS );
	perFrame.vLight[0] = Vector4( SIN( m_fTime ) * fSpinRadius, 2, COS( m_fTime ) * fSpinRadius, fLightPower );
	perFrame.vLight[1] = Vector4( fLightDist, fLightHeight, 0, fLightPower );
	perFrame.vLight[2] = Vector4( 0, fLightHeight, fLightDist, fLightPower );
	perFrame.vLight[3] = Vector4( -fLightDist, fLightHeight, 0, fLightPower );
	perFrame.vLight[4] = Vector4( 0, fLightHeight, -fLightDist, fLightPower );
	perFrame.vLight[5] = Vector4( fLightDist, fLightHeight, fLightDist, fLightPower );
	perFrame.vLight[6] = Vector4( -fLightDist, fLightHeight, fLightDist, fLightPower );
	perFrame.vLight[7] = Vector4( fLightDist, fLightHeight, -fLightDist, fLightPower );
	perFrame.vLight[8] = Vector4( -fLightDist, fLightHeight, -fLightDist, fLightPower );
	perFrame.vLight[9] = Vector4( -SIN( m_fTime ) * fSpinRadius, 2, -COS( m_fTime ) * fSpinRadius, fLightPower );

	return 1;
}

int Renderer::render()
{
	if( !m_bInitComplete )
		return 0;

	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	Int32 nWinWidth, nWinHeight;
	SDL_GetWindowSize( m_Window, &nWinWidth, &nWinHeight );

	for( Int32 i = 0; i < 2; i++ )
	{	
		glViewport( 0, ( nWinHeight >> 1 ) * i, nWinWidth, nWinHeight >> 1 );

		glhUpdateBuffer( m_eftMesh, m_bufPerFrame );

		_DrawArena();

		glhEnableTexture( m_difArenaTops );
		glhEnableTexture( m_nrmArenaTops, 1 );

		for (std::pair<entity_id, struct kart>kart_id_pair: m_mKarts) {
			auto kart_entity = GETENTITY(kart_id_pair.first, CarEntity);

			perMesh.vColor = kart_id_pair.second.vColor;
			perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
			perMesh.matWorld = Matrix::GetRotateQuaternion( kart_entity->Orient ) *
				Matrix::GetTranslate( kart_entity->Pos );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshKart );
		}

		for (auto id_powerup_pair : m_powerups) {
			auto powerup = id_powerup_pair.second;
			Vector4 color1;
			Vector4 color2;
			Vector3 pos = powerup.vPos;

			// Could switch mesh if we had
			// multiple models
			switch (powerup.type) {
				case Entities::BulletPowerup:
					color1 = Vector4(1, 0, 0, 1);
					color2 = Vector4(1, 1, 1, 1);
				break;
				default:
					color1 = Vector4(1, 1, 0, 1);
					color2 = Vector4(0, 0, 1, 1);
				break;
			}

			perMesh.vColor = color1;
			perMesh.vRenderParams = Vector4( 1, 1, 0, 0 );
			perMesh.matWorld = Matrix::GetRotateY( m_fTime * 7 ) * Matrix::GetTranslate( pos );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshPowerRing1 );

			perMesh.matWorld = Matrix::GetRotateY( -m_fTime * 10 ) * Matrix::GetTranslate( pos );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshPowerRing2 );

			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			perMesh.vColor = color2;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshPowerSphere );
			glDisable( GL_BLEND );
		}
	}

	SDL_GL_SwapWindow( m_Window );

	return 1;
}

void Renderer::_DrawArena()
{
	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	// QUAD A
	glCullFace( GL_BACK );
	perMesh.vRenderParams = Vector4( -1, 0, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( -1, 1, 1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 1, 0, 0 ) );

	// QUAD B
	perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( 1, 1, -1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 0, 1, 0 ) );
	
	// QUAD C
	glCullFace( GL_FRONT );
	perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 1, 1, 0 ) );

	// QUAD D
	perMesh.vRenderParams = Vector4( -1, 0, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( -1, 1, -1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 0, 0, 1 ) );
}

void Renderer::_DrawArenaQuad( Vector3 vColor )
{
	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	perMesh.vColor = Vector4( 1,1,1,1 );
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );

	glhEnableTexture( m_difArenaWalls );
	glhEnableTexture( m_nrmArenaWalls, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaWalls );

	glhEnableTexture( m_difArenaFloor );
	glhEnableTexture( m_nrmArenaFloor, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaFloor );

	perMesh.vColor = Vector4( vColor,1 );
	glhUpdateBuffer( m_eftMesh, m_bufPerMesh );

	glhEnableTexture( m_difArenaTops );
	glhEnableTexture( m_nrmArenaTops, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaTops );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glhEnableTexture( m_difArenaFlags );
	glhEnableTexture( m_nrmArenaFlags, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaFlags );

	glDisable( GL_BLEND );
}
