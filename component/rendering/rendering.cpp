#include <vector>
#include <functional>

#include "rendering.h"
#include "../physics/physics.h"

struct GuiBox
{
	GLvertex box[6];

	void Make( Real x, Real y, Real w, Real h, const Vector4& color )
	{
		Real hw = w * 0.5f;
		Real hh = h * 0.5f;
		box[0] = GLvertex( Vector3( -hw + x, -hh + y, 0 ), Vector2( 0, 1 ), color );
		box[1] = GLvertex( Vector3( -hw + x,  hh + y, 0 ), Vector2( 0, 0 ), color );
		box[2] = GLvertex( Vector3(  hw + x, -hh + y, 0 ), Vector2( 1, 1 ), color );
		box[3] = GLvertex( Vector3( -hw + x,  hh + y, 0 ), Vector2( 0, 0 ), color );
		box[4] = GLvertex( Vector3(  hw + x,  hh + y, 0 ), Vector2( 1, 0 ), color );
		box[5] = GLvertex( Vector3(  hw + x, -hh + y, 0 ), Vector2( 1, 1 ), color );
	}

	void Num( Int32 n )
	{
		const Real fSpread = 1.0f / 10.0f;
		box[0].vTex.x = box[1].vTex.x = box[3].vTex.x = n * fSpread;
		box[2].vTex.x = box[4].vTex.x = box[5].vTex.x = ( n + 1 ) * fSpread;
	}

	inline operator const GLvertex* () const { return (GLvertex*)this; }
	inline GLvertex& operator[]( const int i ) { return box[i]; }
	GuiBox( Real x, Real y, Real w, Real h, const Vector4& color = Vector4(1,1,1,1) ) { Make( x, y, w, h, color ); }
	GuiBox() {}
};

std::map<int, struct Physics::Simulation::bullet *> list_of_bullets;
std::vector<entity_id> kartsByScore;

Renderer::Renderer()
{
	m_bInitComplete = 0;
	m_Window = 0;
	m_nSplitScreen = 1;
	m_nScreenState = RS_START;
	m_fTime = 0;

	m_pMailbox = new Events::Mailbox();
	m_pMailbox->request( Events::EventType::Input );
	m_pMailbox->request( Events::EventType::StateUpdate );
	m_pMailbox->request( Events::EventType::PlayerKart );
	m_pMailbox->request( Events::EventType::AiKart );
	m_pMailbox->request( Events::EventType::KartCreated );
	m_pMailbox->request( Events::EventType::KartDestroyed );
	m_pMailbox->request( Events::EventType::PowerupPlacement );
	m_pMailbox->request( Events::EventType::PowerupDestroyed );
	m_pMailbox->request( Events::EventType::PowerupPickup );
	m_pMailbox->request( Events::EventType::BulletList );
	m_pMailbox->request( Events::EventType::ScoreBoardUpdate );
	m_pMailbox->request( Events::EventType::RoundStart );
	m_pMailbox->request( Events::EventType::RoundEnd );

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
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
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

	// Note: default glew only inits 2.0 and lower apis
	glewExperimental = GL_TRUE;
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

	m_eftGUI = glhLoadEffect( "component/rendering/VShaderGUI.glsl", NULL, "component/rendering/PShaderGUI.glsl", 0, 0 );
	if( !m_eftGUI.program )
		exit(5);

	glhCreateBuffer( m_eftMesh, "cstPerMesh", sizeof(cstPerMesh), &m_bufPerMesh );
	glhCreateBuffer( m_eftMesh, "cstPerFrame", sizeof(cstPerFrame), &m_bufPerFrame );
	glhCreateBuffer( m_eftGUI, "cstGUI", sizeof(cstGUI), &m_bufGUI );
	
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
	if( !glhLoadTexture( m_nrmArenaFloor, "assets/arena_floor_norm.png" ) )
		exit(17);

	if( !LoadMesh( m_mshArenaTops, "assets/arena_tops.msh" ) )
		exit(18);

	if( !LoadMesh( m_mshGold, "assets/Gold.msh" ) )
		exit(21);

	if( !LoadMesh( m_mshBullet, "assets/Bullet.msh" ) )
		exit(21);
	
	if( !LoadMesh( m_mshKart, "assets/Kart.msh" ) )
		exit(21);
	if( !LoadMesh( m_mshKartTire, "assets/Kart_tire.msh" ) )
		exit(21);
	if( !LoadMesh( m_mshKartShadow, "assets/Kart_shadow.msh" ) )
		exit(21);
	if( !glhLoadTexture( m_difKartShadow, "assets/kart_shadow.png" ) )
		exit(19);

	if( !LoadMesh( m_mshPowerSphere, "assets/PowerSphere.msh" ) )
		exit(22);
	if( !LoadMesh( m_mshPowerRing1, "assets/PowerRing1.msh" ) )
		exit(23);
	if( !LoadMesh( m_mshPowerRing2, "assets/PowerRing2.msh" ) )
		exit(24);

	if( !glhLoadTexture( m_difBlank, "assets/blank.png" ) )
		exit(19);
	if( !glhLoadTexture( m_nrmBlank, "assets/blank_norm.png" ) )
		exit(20);

	if( !glhCreateGUI( m_mshGUIStart, GuiBox( 0, 0, 600, 400 ), 6 ) )
		exit(25);
	if( !glhLoadTexture( m_texGUIStart, "assets/KartStartScreen.png" ) )
		exit(14);
	if( !glhLoadTexture( m_texGUIScore, "assets/KartScoreScreen.png" ) )
		exit(14);
	if( !glhLoadTexture( m_texGUINumbers, "assets/gui_numbers.png" ) )
		exit(14);
	if( !glhLoadTexture( m_texGUIPlayer, "assets/gui_player.png" ) )
		exit(14);

	
	
	glhMapTexture( m_eftMesh, "g_texDiffuse", 0 );
	glhMapTexture( m_eftMesh, "g_texNormal", 1 );
	glhMapTexture( m_eftGUI, "g_texDiffuse", 0 );

	m_bInitComplete = 1;
	
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

bool player_kart_found = false;
int Renderer::render( float fElapseSec )
{
	m_fTime += fElapseSec;

	if( !m_bInitComplete )
		return 0;

	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;

	Int32 nWinWidth, nWinHeight;
	SDL_GetWindowSize( m_Window, &nWinWidth, &nWinHeight );

	std::vector<Entities::CarEntity::Camera> cameras;
	for( Events::Event *event : m_pMailbox->checkMail() )
	{
		switch( event->type ) 
		{
		case Events::EventType::RoundStart:
			m_nScreenState = RS_DRIVING;
			break;
		case Events::EventType::RoundEnd:
			m_nScreenState = RS_END;
			break;
		// Each bullet has a position and a direction passed for rendering and an additional value "time to live" that physics uses, no idea if rendering needs it.
		case Events::EventType::BulletList:
		{
			auto bullet_list_event = ((Events::BulletListEvent *)event);
			list_of_bullets = *((std::map<int, struct Physics::Simulation::bullet *> *)(bullet_list_event->list_of_bullets)); // This was passed as a void *, don't forget to cast!

			//DEBUGOUT("LIST OF BULLETS REVIECED!\n")
		}
		break;
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
				
//				cameras.clear();
				cameras.push_back(kart->camera);

				player_kart_found = true;
			}
			break;
		case Events::EventType::AiKart:
			{
//				if (!player_kart_found)
				{
					entity_id kart_id = ((Events::AiKartEvent *)event)->kart_id;
					auto kart = GETENTITY(kart_id, CarEntity);

					cameras.push_back(kart->camera);
				}
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
		case Events::EventType::ScoreBoardUpdate:
			{
				auto scoreboard = ((Events::ScoreBoardUpdateEvent *)event);
				kartsByScore = scoreboard->kartsByScore;
			}
			break;
		default:
			break;
		}
	}
	m_pMailbox->emptyMail();

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


	// START RENDERING

	struct RCAMERA
	{
		Int32 x,y,w,h,fov;
		Vector3 eyepos, eyefocus;
	};
	std::vector<RCAMERA> aryCameras;

	switch( m_nScreenState )
	{
	case RS_START:
	case RS_END:
		{
			RCAMERA cam;
			cam.x = cam.y = 0;
			cam.w = nWinWidth;
			cam.h = nWinHeight;
			cam.fov = DEGTORAD( 80 );
			cam.eyepos = Vector3(0,15,-5);
			cam.eyefocus = Vector3(0,-0.8,0);
			aryCameras.push_back( cam );
		}
		break;
	case RS_DRIVING:
		{
			switch( cameras.size() )
			{
			case 0:
				{
					RCAMERA cam;
					cam.x = cam.y = 0;
					cam.w = nWinWidth;
					cam.h = nWinHeight;
					cam.fov = DEGTORAD( 80 );
					cam.eyepos = Vector3(0,15,-5);
					cam.eyefocus = Vector3(0,-0.8,0);
					aryCameras.push_back( cam );
				}
				break;
			case 1:
				{
					RCAMERA cam;
					cam.x = cam.y = 0;
					cam.w = nWinWidth;
					cam.h = nWinHeight;
					cam.fov = DEGTORAD( cameras[0].fFOV );
					cam.eyepos = cameras[0].vPos;
					cam.eyefocus = cameras[0].vFocus;
					aryCameras.push_back( cam );
				}
				break;
			case 2:
				{
					RCAMERA cam;
					cam.x = cam.y = 0;
					cam.w = nWinWidth;
					cam.h = nWinHeight>>1;
					cam.fov = DEGTORAD( cameras[0].fFOV );
					cam.eyepos = cameras[0].vPos;
					cam.eyefocus = cameras[0].vFocus;
					aryCameras.push_back( cam );

					cam.y = nWinHeight>>1;
					cam.fov = DEGTORAD( cameras[1].fFOV );
					cam.eyepos = cameras[1].vPos;
					cam.eyefocus = cameras[1].vFocus;
					aryCameras.push_back( cam );
				}
				break;
			case 3:
				{
					RCAMERA cam;
					cam.x = cam.y = 0;
					cam.w = nWinWidth;
					cam.h = nWinHeight>>1;
					cam.fov = DEGTORAD( cameras[0].fFOV );
					cam.eyepos = cameras[0].vPos;
					cam.eyefocus = cameras[0].vFocus;
					aryCameras.push_back( cam );

					cam.w = nWinWidth>>1;
					cam.y = nWinHeight>>1;
					cam.fov = DEGTORAD( cameras[1].fFOV );
					cam.eyepos = cameras[1].vPos;
					cam.eyefocus = cameras[1].vFocus;
					aryCameras.push_back( cam );

					cam.x = nWinWidth>>1;
					cam.fov = DEGTORAD( cameras[2].fFOV );
					cam.eyepos = cameras[2].vPos;
					cam.eyefocus = cameras[2].vFocus;
					aryCameras.push_back( cam );
				}
				break;
			default: // 4
				{
					RCAMERA cam;
					cam.x = cam.y = 0;
					cam.w = nWinWidth>>1;
					cam.h = nWinHeight>>1;
					cam.fov = DEGTORAD( cameras[0].fFOV );
					cam.eyepos = cameras[0].vPos;
					cam.eyefocus = cameras[0].vFocus;
					aryCameras.push_back( cam );
					
					cam.x = nWinWidth>>1;
					cam.fov = DEGTORAD( cameras[1].fFOV );
					cam.eyepos = cameras[1].vPos;
					cam.eyefocus = cameras[1].vFocus;
					aryCameras.push_back( cam );

					cam.x = 0;
					cam.y = nWinHeight>>1;
					cam.fov = DEGTORAD( cameras[2].fFOV );
					cam.eyepos = cameras[2].vPos;
					cam.eyefocus = cameras[2].vFocus;
					aryCameras.push_back( cam );

					cam.x = nWinWidth>>1;
					cam.fov = DEGTORAD( cameras[3].fFOV );
					cam.eyepos = cameras[3].vPos;
					cam.eyefocus = cameras[3].vFocus;
					aryCameras.push_back( cam );
				}
				break;
			}
		}
		break;
	};

	for( Int32 i = 0; i < aryCameras.size(); i++ )
	{	
		glViewport( aryCameras[i].x, aryCameras[i].y, aryCameras[i].w, aryCameras[i].h );

		Vector3 vFocus = Vector3(0,-0.8,0);
		perFrame.vEyePos = aryCameras[i].eyepos;
		perFrame.vEyeDir = Vector3( aryCameras[i].eyefocus - aryCameras[i].eyepos ).Normalize();
		perFrame.matProj.Perspective( aryCameras[i].fov, (Real)aryCameras[i].w/aryCameras[i].h, 0.1f, 100.0f );
		perFrame.matView.LookAt( perFrame.vEyePos.xyz(), aryCameras[i].eyefocus, Vector3( 0, 1, 0 ) );
		perFrame.matViewProj = perFrame.matView * perFrame.matProj;
		glhUpdateBuffer( m_eftMesh, m_bufPerFrame );

		_DrawArena();

		for (std::pair<entity_id, struct kart>kart_id_pair: m_mKarts) {
			auto kart_entity = GETENTITY(kart_id_pair.first, CarEntity);

			// DRAW SHADOW
			Vector3 vAxis = Vector3::Cross( kart_entity->groundNormal, Vector3( 0, 1, 0 ) );
			Real fAngle = ACOS( Vector3::Dot( kart_entity->groundNormal, Vector3( 0, 1, 0 ) ) );

			perMesh.vColor = Vector4( 0,0,0,0.3f );
			perMesh.matWorld = Matrix::GetRotateAxis( vAxis, fAngle ) *
				Matrix::GetTranslate( kart_entity->groundHit + Vector3( 0, 0.01f, 0 ) );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );

			glhEnableTexture( m_difKartShadow );
			glhEnableTexture( m_nrmBlank, 1 );

			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			glhDrawMesh( m_eftMesh, m_mshKartShadow );
			glDisable( GL_BLEND );

			// DRAW KART
			glhEnableTexture( m_difBlank );
			glhEnableTexture( m_nrmBlank, 1 );

			perMesh.vColor = kart_id_pair.second.vColor;
			perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
			perMesh.matWorld = Matrix::GetRotateQuaternion( kart_entity->Orient ) *
				Matrix::GetTranslate( kart_entity->Pos );
			perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
			glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
			glhDrawMesh( m_eftMesh, m_mshKart );

			for( int i = 0; i < 4; i++ )
			{
				perMesh.matWorld = Matrix::GetRotateQuaternion( kart_entity->tireOrient[i] ) *
					Matrix::GetTranslate( kart_entity->tirePos[i] );
				perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
				glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
				glhDrawMesh( m_eftMesh, m_mshKartTire );
			}
		}

		glhEnableTexture( m_difBlank );
		glhEnableTexture( m_nrmBlank, 1 );
		for (auto id_powerup_pair : m_powerups) {
			auto powerup = id_powerup_pair.second;
			Vector3 pos = powerup.vPos;

			if( powerup.type == Entities::GoldCasePowerup )
			{
				perMesh.vRenderParams = Vector4( 1, 0, 1, 0 );
				perMesh.vColor = Vector4(1,1,1,1);
				perMesh.matWorld = Matrix::GetTranslate( pos );
				perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
				glhUpdateBuffer( m_eftMesh, m_bufPerMesh );

				glEnable( GL_BLEND );
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				glhDrawMesh( m_eftMesh, m_mshGold );
				glDisable( GL_BLEND );
			}
			else
			{
				Vector4 color1;
				Vector4 color2;
				switch (powerup.type) 
				{
					case Entities::HealthPowerup:
						color1 = Vector4(1, 0, 0, 1);
						color2 = Vector4(1, 1, 1, 1);
					break;

					case Entities::SpeedPowerup:
						color1 = Vector4(1, 1, 1, 1);
						color2 = Vector4(0, 0, 0, 0);
					break;

					case Entities::RocketPowerup:
						color1 = Vector4(0, 0, 0, 0);
						color2 = Vector4(0, 0, 0, 0);
					break;

					case Entities::PulsePowerup:
						color1 = Vector4(0.01, 1, 0, 1);
						color2 = Vector4(0.0001, 0.01, 1, 1);
					break;

					default:
						color1 = Vector4(1, 1, 0, 1);
						color2 = Vector4(0, 0, 1, 1);
					break;
				}

				perMesh.vRenderParams = Vector4( 1, 0, 0, 0 );
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
	}

	for (auto bullet_pair : list_of_bullets) 
	{
			
		Vector4 color1;
		Vector4 color2;
		auto bullet = bullet_pair.second;
		Vector3 pos;
		pos.x = bullet->position.getX();
		pos.y = bullet->position.getY();
		pos.z = bullet->position.getZ();

		Vector3 vDir = Vector3( bullet->direction.x(), bullet->direction.y(), bullet->direction.z() );
		Vector3 vAxis = Vector3::Cross( vDir, Vector3( 0, 0, 1 ) );
		Real fAngle = ACOS( Vector3::Dot( vDir, Vector3( 0, 0, 1 ) ) );
			
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		perMesh.vColor = Vector4(1, 1, 1, 1);
		perMesh.vRenderParams = Vector4( 1, 1, 1, 0 );
		perMesh.matWorld = Matrix::GetRotateAxis( vAxis, fAngle ) * Matrix::GetTranslate( pos );
		perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
		glhUpdateBuffer( m_eftMesh, m_bufPerMesh );
		glhDrawMesh( m_eftMesh, m_mshBullet );
		glDisable( GL_BLEND );
	}

	// GUI TEST
	glClear( GL_DEPTH_BUFFER_BIT );

	Matrix matOrtho;
	matOrtho.Orthographic( 0, nWinWidth, 0, nWinHeight, -1, 1 );

	cstGUI& guidata = *(cstGUI*)m_bufGUI.data;
	guidata.matWorldViewProj = matOrtho;
	glhUpdateBuffer( m_eftGUI, m_bufGUI );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	switch( m_nScreenState )
	{
	case RS_START:
		glhEnableTexture( m_texGUIStart );
		glhDrawMesh( m_eftGUI, m_mshGUIStart );
		break;
	case RS_END:
		glhEnableTexture( m_texGUIScore );
		glhDrawMesh( m_eftGUI, m_mshGUIStart );
		_DrawScore( 0, 0 );
		break;
	case RS_DRIVING:
		_DrawScore( -(nWinWidth>>1) + 125, (nWinHeight>>1) - 40 );
		break;
	}

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );

	SDL_GL_SwapWindow( m_Window );

	return 1;
}

void Renderer::_DrawArena()
{
	cstPerMesh& perMesh = *(cstPerMesh*)m_bufPerMesh.data;
	cstPerFrame& perFrame = *(cstPerFrame*)m_bufPerFrame.data;
	
	// QUAD A
	glCullFace( GL_BACK );
	perMesh.vRenderParams = Vector4( -1, 1, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( -1, 1, 1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 1, 0, 0 ) );

	// QUAD B
	perMesh.vRenderParams = Vector4( 1, -1, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs ) * Matrix::GetScale( 1, 1, -1 );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 0, 1, 0 ) );
	
	// QUAD C
	glCullFace( GL_FRONT );
	perMesh.vRenderParams = Vector4( 1, 1, 0, 0 );
	perMesh.matWorld = Matrix::GetTranslate( m_vArenaOfs );
	perMesh.matWorldViewProj = perMesh.matWorld * perFrame.matViewProj;
	_DrawArenaQuad( Vector3( 1, 1, 0 ) );
	
	// QUAD D
	perMesh.vRenderParams = Vector4( -1, -1, 0, 0 );
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

	glhEnableTexture( m_difBlank );
	glhEnableTexture( m_nrmBlank, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaTops );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glhEnableTexture( m_difArenaFlags );
	glhEnableTexture( m_nrmArenaFlags, 1 );
	glhDrawMesh( m_eftMesh, m_mshArenaFlags );

	glDisable( GL_BLEND );
}

void Renderer::_DrawScore( Int32 x, Int32 y ) // 125, 40
{
	Int32 nWinWidth, nWinHeight;
	SDL_GetWindowSize( m_Window, &nWinWidth, &nWinHeight );

	GLmesh temp_mesh;

	for( Int32 i = 0; i < kartsByScore.size(); i++ )
	{
		Int32 ofs = i * 30;

		auto kart_entity = GETENTITY(kartsByScore[i], CarEntity);

		Vector4 color = m_mKarts[kartsByScore[i]].vColor;
		color.w = 0.5f;

		glhCreateGUI( temp_mesh, GuiBox( GuiBox( x, y - ofs, 200, 30, color ) ), 6 );
		glhEnableTexture( m_difBlank );
		glhDrawMesh( m_eftGUI, temp_mesh );
		glhDestroyMesh( temp_mesh );

		glhCreateGUI( temp_mesh, GuiBox( GuiBox( 10 + x, y - ofs, 110, 30, Vector4( 1,1,1,1 ) ) ), 6 );
		glhEnableTexture( m_texGUIPlayer );
		glhDrawMesh( m_eftGUI, temp_mesh );
		glhDestroyMesh( temp_mesh );

		GuiBox pnum_box = GuiBox( 80 + x, y - ofs, 20, 30, Vector4( 1,1,1,1 ) );
		pnum_box.Num( i );
		glhCreateGUI( temp_mesh, pnum_box, 6 );
		glhEnableTexture( m_texGUINumbers );
		glhDrawMesh( m_eftGUI, temp_mesh );
		glhDestroyMesh( temp_mesh );

		GuiBox snum_box = GuiBox( -85 + x, y - ofs, 20, 30, Vector4( 1,1,1,1 ) );
		snum_box.Num( kart_entity->gold );
		glhCreateGUI( temp_mesh, snum_box, 6 );
		glhEnableTexture( m_texGUINumbers );
		glhDrawMesh( m_eftGUI, temp_mesh );
		glhDestroyMesh( temp_mesh );
	}
}
