#include "GaGameComponent.h"
#include "GaHotspotComponent.h"
#include "GaPositionUtility.h"

#include "System/Debug/DsCore.h"
#include "System/Debug/DsImGui.h"
#include "System/Os/OsCore.h"

#include "System/Scene/ScnCore.h"
#include "System/Scene/ScnEntity.h"
#include "System/Scene/Rendering/ScnCanvasComponent.h"
#include "System/Scene/Rendering/ScnFont.h"

#include "System/SysKernel.h"

//////////////////////////////////////////////////////////////////////////
// Reflection
REFLECTION_DEFINE_DERIVED( GaGameProcessor );

void GaGameProcessor::StaticRegisterClass()
{
	ReRegisterClass< GaGameProcessor, Super >();
}


//////////////////////////////////////////////////////////////////////////
// Ctor
GaGameProcessor::GaGameProcessor():
	ScnComponentProcessor( 
		{
			ScnComponentProcessFuncEntry(
				"Update game state",
				ScnComponentPriority::DEFAULT_UPDATE,
				std::bind( &GaGameProcessor::update, this, std::placeholders::_1 ) )
		} )
{
}

//////////////////////////////////////////////////////////////////////////
// Dtor
//virtual 
GaGameProcessor::~GaGameProcessor()
{

}

//////////////////////////////////////////////////////////////////////////
// initialise
void GaGameProcessor::initialise()
{

}

//////////////////////////////////////////////////////////////////////////
// shutdown
void GaGameProcessor::shutdown()
{
	
}

//////////////////////////////////////////////////////////////////////////
// drawMenus
void GaGameProcessor::update( const ScnComponentList& Components )
{
	BcAssert( Components.size() <= 1 );

	BcF32 Tick = SysKernel::pImpl()->getFrameTime();
	for( auto InComponent : Components )
	{
		BcAssert( InComponent->isTypeOf< GaGameComponent >() );
		auto* Component = static_cast< GaGameComponent* >( InComponent.get() );

		// Handle game state specific stuff.
		switch( Component->GameState_ )
		{
		case GaGameComponent::GameState::IDLE:
			onIdle( Component, Tick );
			break;
		case GaGameComponent::GameState::BUILD_PHASE:
			onBuildPhase( Component, Tick );
			break;
		case GaGameComponent::GameState::DEFEND_PHASE:
			onDefendPhase( Component, Tick );
			break;
		case GaGameComponent::GameState::GAME_OVER:
			break;
		}

#if !PSY_PRODUCTION
		if( ImGui::Begin( "Game Debug" ) )
		{
			ImGui::Text( "Game timer: %f", Component->GameTimer_ );
			ImGui::Text( "Game state: %u", Component->GameState_ );
			if( ImGui::Button( "Back to Main Menu" ) )
			{
				ScnEntitySpawnParams SpawnParams( 
						BcName::INVALID, "menus", "MainMenu",
						MaMat4d(), Component->getParentEntity()->getParentEntity() );
				SpawnParams.OnSpawn_ = [ Component ]( ScnEntity* Entity )
					{
						ScnCore::pImpl()->removeEntity( Component->getParentEntity() );
					};

				ScnCore::pImpl()->spawnEntity( SpawnParams );

			}
			ImGui::Separator();
			ImGui::End();
		}
#endif
	}
}

//////////////////////////////////////////////////////////////////////////
// advanceGameTimer
void GaGameProcessor::advanceGameTimer( GaGameComponent* Component, BcF32 Tick )
{
	Component->GameTimer_ += Tick;

	const BcF32 LevelGameTimer = std::fmodf( Component->GameTimer_, Component->GamePhaseTime_ );
	const BcF32 HalfGamePhaseTime = Component->GamePhaseTime_ * 0.5f;

	// Swap between states at the half way/wrap points.
	if( Component->GameState_ == GaGameComponent::GameState::BUILD_PHASE &&
		LevelGameTimer > HalfGamePhaseTime )
	{
		Component->setState( GaGameComponent::GameState::DEFEND_PHASE );
	}
	else if( Component->GameState_ == GaGameComponent::GameState::DEFEND_PHASE &&
		LevelGameTimer < HalfGamePhaseTime )
	{
		Component->setState( GaGameComponent::GameState::BUILD_PHASE );
	}
}

//////////////////////////////////////////////////////////////////////////
// onIdle
void GaGameProcessor::onIdle( GaGameComponent* Component, BcF32 Tick )
{
	Component->setState( GaGameComponent::GameState::BUILD_PHASE );
}

//////////////////////////////////////////////////////////////////////////
// onBuildPhase
void GaGameProcessor::onBuildPhase( GaGameComponent* Component, BcF32 Tick )
{
	advanceGameTimer( Component, Tick );

}

//////////////////////////////////////////////////////////////////////////
// onDefendPhase
void GaGameProcessor::onDefendPhase( GaGameComponent* Component, BcF32 Tick )
{
	advanceGameTimer( Component, Tick );

}

//////////////////////////////////////////////////////////////////////////
// onGameOver
void GaGameProcessor::onGameOver( GaGameComponent* Component, BcF32 Tick )
{

}

//////////////////////////////////////////////////////////////////////////
// Reflection
REFLECTION_DEFINE_DERIVED( GaGameComponent );

void GaGameComponent::StaticRegisterClass()
{
	ReField* Fields[] = 
	{
		new ReField( "Level_", &GaGameComponent::Level_, bcRFF_IMPORTER ),
	};

	ReRegisterClass< GaGameComponent, Super >( Fields )
		.addAttribute( new GaGameProcessor() );
}

//////////////////////////////////////////////////////////////////////////
// Ctor
GaGameComponent::GaGameComponent()
{

}

//////////////////////////////////////////////////////////////////////////
// Dtor
GaGameComponent::~GaGameComponent()
{

}

//////////////////////////////////////////////////////////////////////////
// onAttach
void GaGameComponent::onAttach( ScnEntityWeakRef Parent )
{
	// Get canvas + font.
	Canvas_ = Parent->getComponentAnyParentByType< ScnCanvasComponent >();
	BcAssert( Canvas_ );
	Font_ = Parent->getComponentAnyParentByType< ScnFontComponent >();
	BcAssert( Font_ );

	// Spawn tentacle things.
	MaMat4d TransformA;
	MaMat4d TransformB;
	auto TentacleA = GaPositionUtility::GetScreenPosition( MaVec2d( 128.0f, 128.0f ), MaVec2d( 32.0f, 32.0f ), GaPositionUtility::TOP | GaPositionUtility::LEFT );
	auto TentacleB = GaPositionUtility::GetScreenPosition( MaVec2d( 128.0f, 128.0f ), MaVec2d( 32.0f, 32.0f ), GaPositionUtility::TOP | GaPositionUtility::RIGHT );
	TransformA.translation( MaVec3d( TentacleA, 0.0f ) );
	TransformB.translation( MaVec3d( TentacleB, 0.0f ) );
	ScnCore::pImpl()->spawnEntity( 
		ScnEntitySpawnParams( 
			BcName::INVALID, "game", "TentacleEntity",
			TransformA, Parent ) );
	ScnCore::pImpl()->spawnEntity( 
		ScnEntitySpawnParams( 
			BcName::INVALID, "game", "TentacleEntity",
			TransformB, Parent ) );

	
	Super::onAttach( Parent );
}

//////////////////////////////////////////////////////////////////////////
// onDetach
void GaGameComponent::onDetach( ScnEntityWeakRef Parent )
{
	Parent->unsubscribeAll( this );
	Super::onDetach( Parent );
}

//////////////////////////////////////////////////////////////////////////
// setState
void GaGameComponent::setState( GameState GameState )
{
	PSY_LOG( "Changing state from %u -> %u", GameState_, GameState );
	GameState_ = GameState;
}
