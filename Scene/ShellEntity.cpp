/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "ShellEntity.h"
#include "TankEntity.h"
#include "EntityManager.h"
#include "Messenger.h"

namespace gen
{

	// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
	// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
	//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
	extern CEntityManager EntityManager;

	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;

	// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
	// Will be needed to implement the required shell behaviour in the Update function below
	extern TEntityUID GetTankUID(int team);



	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Shell Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// Shell constructor intialises shell-specific data and passes its parameters to the base
	// class constructor
	CShellEntity::CShellEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		TEntityUID       ShooterUID,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CEntity(entityTemplate, UID, name, position, rotation, scale)
	{
		// Initialise any shell data you add
		shooterUID = ShooterUID;
	}


	// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as
	// one of the assignment requirements
	// Return false if the entity is to be destroyed
	bool CShellEntity::Update(TFloat32 updateTime)
	{
		m_Timer += updateTime *1;


		Matrix().MoveLocalZ(kMaxSpeed);

		if (m_Timer > destructionPoint)
		{
			return false;
		}

		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != nullptr)
		{
			if (entity->GetUID() != shooterUID)
			{

				if (Distance(Position(), entity->Position()) < 8.0f)
				{
					SMessage msg;
					msg.type = Msg_Hit;
					msg.from = shooterUID;
					Messenger.SendMessage(entity->GetUID(), msg);
					dynamic_cast<CTankEntity*>(entity)->ShotBy(shooterUID);
					return false;
				}
			}
			entity = EntityManager.EnumEntity();  //Moves onto next Entity
		}
		EntityManager.EndEnumEntities();

		return true; // Placeholder
	}


} // namespace gen
