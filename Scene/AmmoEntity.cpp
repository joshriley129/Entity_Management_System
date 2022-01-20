/*******************************************
	AmmoEntity.cpp

	Ammo entity class
********************************************/

#include "AmmoEntity.h"
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
		Ammo Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// Shell constructor intialises shell-specific data and passes its parameters to the base
	// class constructor
	CAmmoEntity::CAmmoEntity
	(
		CEntityTemplate* entityTemplate,
		TEntityUID       UID,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CEntity(entityTemplate, UID, name, position, rotation, scale)
	{
		// Initialise any shell data you add
		m_UID = UID;
	}


	// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as
	// one of the assignment requirements
	// Return false if the entity is to be destroyed
	bool CAmmoEntity::Update(TFloat32 updateTime)
	{
		//FALL TO GROUND
		if (Position().y > 0.5f)
		{
			Matrix().MoveLocalY(-5.0f);
		}
		//SEND MESSAGE
		else
		{
			EntityManager.BeginEnumEntities("", "", "Tank");
			CEntity* entity = EntityManager.EnumEntity();
			while (entity != nullptr)
			{
				SMessage msg;
				msg.type = Msg_Ammo;
				msg.from = m_UID;
				Messenger.SendMessage(entity->GetUID(), msg);
				entity = EntityManager.EnumEntity();  //Moves onto next Entity
			}
			EntityManager.EndEnumEntities();
		}

		//REFILL AMMO
		EntityManager.BeginEnumEntities("", "", "Tank");
		CEntity* entity = EntityManager.EnumEntity();
		while (entity != nullptr)
		{
			if (Distance(Position(), entity->Position()) < 2.0f)
			{
				dynamic_cast<CTankEntity*>(entity)->Restock();
				return false;
			}

			entity = EntityManager.EnumEntity();  //Moves onto next Entity
		}
		EntityManager.EndEnumEntities();
	


		return true; // Placeholder
	}


} // namespace gen