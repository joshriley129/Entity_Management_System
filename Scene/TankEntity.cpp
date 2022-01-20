/*******************************************
	TankEntity.cpp

	Tank entity template and entity classes
********************************************/

// Additional technical notes for the assignment:
// - Each tank has a team number (0 or 1), HP and other instance data - see the end of TankEntity.h
//   You will need to add other instance data suitable for the assignment requirements
// - A function GetTankUID is defined in TankAssignment.cpp and made available here, which returns
//   the UID of the tank on a given team. This can be used to get the enemy tank UID
// - Tanks have three parts: the root, the body and the turret. Each part has its own matrix, which
//   can be accessed with the Matrix function - root: Matrix(), body: Matrix(1), turret: Matrix(2)
//   However, the body and turret matrix are relative to the root's matrix - so to get the actual 
//   world matrix of the body, for example, we must multiply: Matrix(1) * Matrix()
// - Vector facing work similar to the car tag lab will be needed for the turret->enemy facing 
//   requirements for the Patrol and Aim states
// - The CMatrix4x4 function DecomposeAffineEuler allows you to extract the x,y & z rotations
//   of a matrix. This can be used on the *relative* turret matrix to help in rotating it to face
//   forwards in Evade state
// - The CShellEntity class is simply an outline. To support shell firing, you will need to add
//   member data to it and rewrite its constructor & update function. You will also need to update 
//   the CreateShell function in EntityManager.cpp to pass any additional constructor data required
// - Destroy an entity by returning false from its Update function - the entity manager wil perform
//   the destruction. Don't try to call DestroyEntity from within the Update function.
// - As entities can be destroyed, you must check that entity UIDs refer to existant entities, before
//   using their entity pointers. The return value from EntityManager.GetEntity will be NULL if the
//   entity no longer exists. Use this to avoid trying to target a tank that no longer exists etc.

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
	// Will be needed to implement the required tank behaviour in the Update function below
	extern TEntityUID GetTankUID(int team);



	/*-----------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------
		Tank Entity Class
	-------------------------------------------------------------------------------------------
	-----------------------------------------------------------------------------------------*/

	// Tank constructor intialises tank-specific data and passes its parameters to the base
	// class constructor
	CTankEntity::CTankEntity
	(
		CTankTemplate* tankTemplate,
		TEntityUID      UID,
		TUInt32         team,
		const string& name /*=""*/,
		const CVector3& position /*= CVector3::kOrigin*/,
		const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
		const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	) : CEntity(tankTemplate, UID, name, position, rotation, scale)
	{
		m_TankTemplate = tankTemplate;

		// Tanks are on teams so they know who the enemy is
		m_Team = team;

		// Initialise other tank data and state
		m_Speed = 0.0f;
		m_HP = m_TankTemplate->GetMaxHP();
		m_Ammo = m_TankTemplate->GetStartingAmmo();
		m_ShellDamage = m_TankTemplate->GetShellDamage();
		m_ShotsFired = 0;
		m_State = Inactive;
		m_Timer = 0.0f;
		m_MaxTurnSpeed = 3.0f;
	}


	// Update the tank - controls its behaviour. The shell code just performs some test behaviour, it
	// is to be rewritten as one of the assignment requirements
	// Return false if the entity is to be destroyed
	bool CTankEntity::Update(TFloat32 updateTime)
	{
		//Initilaising Patrol Points
		CVector3 m_PatrolPointA[5] =
		{
			CVector3(-30.0f, 0.5f, -10.0f),
			CVector3(-10.0f, 0.5f, -20.0f),
			CVector3(-30.0f, 0.5f, 40.0f),
			CVector3(-10.0f, 0.5f, 20.0f),
			CVector3(-30.0f, 0.5f, -10.0f)
		};

		CVector3 m_PatrolPointB[5] =
		{
			CVector3(10.0f, 0.5f, 10.0f),
			CVector3(30.0f, 0.5f, 20.0f),
			CVector3(40.0f, 0.5f, 40.0f),
			CVector3(20.0f, 0.5f, 60.0f),
			CVector3(20.0f, 0.5f, -30.0f)
		};

		// Fetch any messages
		SMessage msg;
		while (Messenger.FetchMessage(GetUID(), &msg))
		{
			// Set state variables based on received messages
			switch (msg.type)
			{
			case Msg_Start:
			{
				m_State = Patrol;

				if (GetTeam() == 0)
				{
					m_TargetPointA = m_PatrolPointA[0];
				}
				else
				{
					m_TargetPointA = m_PatrolPointB[0];
				}

				break;
			}
			case Msg_Stop:
			{
				m_State = Inactive;
				break;
			}
			case Msg_Hit:
			{
				//Take Damage
				m_HP -= dynamic_cast<CTankEntity*>(EntityManager.GetEntity(msg.from))->GetShellDamage();
				//Ask for help
				if (m_HP > 0)
				{
					if (dynamic_cast<CTankEntity*>(EntityManager.GetEntity(msg.from)) != nullptr)
					{
						if (dynamic_cast<CTankEntity*>(EntityManager.GetEntity(msg.from))->GetTeam() != m_Team)
						{

							EntityManager.BeginEnumEntities("", "", "Tank");
							CEntity* entity = EntityManager.EnumEntity();
							while (entity != nullptr)
							{
								if (entity->GetUID() != GetUID())
								{
									//CHECKS NOT ON SAME TEAM
									if (m_Team == dynamic_cast<CTankEntity*>(entity)->GetTeam())
									{
										SMessage msg7;
										msg7.type = Msg_Help;
										msg7.from = GetUID();
										Messenger.SendMessage(entity->GetUID(), msg7);
									}
								}
								entity = EntityManager.EnumEntity();  //Moves onto next Entity
							}
							EntityManager.EndEnumEntities();
							break;
						}
					}
				}
			}
			case Msg_Help:
			{
				if (m_State == Patrol || m_State == Evade)
				{
					m_NeedsHelp = msg.from;


					EntityManager.BeginEnumEntities("", "", "Tank");
					CEntity* entity = EntityManager.EnumEntity();
					while (entity != nullptr)
					{
						if (entity->GetUID() != GetUID())
						{
							//
							if (m_NeedsHelp == dynamic_cast<CTankEntity*>(entity)->GetUID())
							{
								if (dynamic_cast<CTankEntity*>(entity)->GetTarget() != GetUID())
								{
									m_TargetTank = dynamic_cast<CTankEntity*>(entity)->GetTarget();
									m_State = Aim;
									break;
								}
							}
						}
						entity = EntityManager.EnumEntity();  //Moves onto next Entity
					}
					EntityManager.EndEnumEntities();
				}
				break;
			}
			case Msg_Ammo:
			{
				m_TargetAmmo = msg.from;
				m_State = Hunting;

				break;
			}
			}
		}


		// Tank behaviour
		//Health

		if (m_HP <= 0)
		{
			m_State = Inactive;

			//resets timer
			if (m_Dead == false)
			{
				m_Timer = 0;
				m_Dead == true;
			}
			//Flips Tank
			m_Timer += updateTime * 1;
			if (m_Timer < 0.5)
			{
				//Flip Up
				Matrix().MoveY(0.3f);
				Matrix().RotateLocalZ(0.05f);
				Matrix().MoveLocalX(0.1f);
			}
			else
			{
				//Flip Down
				if (Position().y > 0.5)
				{
					Matrix().MoveY(-0.3f);
					Matrix().RotateLocalZ(0.1f);
					Matrix().MoveLocalX(-0.1f);
				}
				else
				{
					//When Hit Ground, stay still
					Matrix().MoveY(0);
					Matrix().RotateLocalZ(0);
					Matrix().MoveLocalX(0);
				}
			}
		}

		// Only move if in Patrol state
		if (m_State == Patrol)
		{
			// Face wander point and move forwards towards it. 
			PatrolMove(updateTime);

			// When reached the wander point turn and head to next point.
			if (GetTeam() == 0)
			{
				if (Position().DistanceTo(m_TargetPointA) < 8.0f)
				{
					if (m_TargetPointA == m_PatrolPointA[0])
					{
						m_TargetPointA = m_PatrolPointA[1];
					}
					else if (m_TargetPointA == m_PatrolPointA[1])
					{
						m_TargetPointA = m_PatrolPointA[2];
					}
					else if (m_TargetPointA == m_PatrolPointA[2])
					{
						m_TargetPointA = m_PatrolPointA[3];
					}
					else if (m_TargetPointA == m_PatrolPointA[3])
					{
						m_TargetPointA = m_PatrolPointA[4];
					}
					else if (m_TargetPointA == m_PatrolPointA[4])
					{
						m_TargetPointA = m_PatrolPointA[0];
					}
				}
			}
			else
			{
				if (Position().DistanceTo(m_TargetPointA) < 8.0f)
				{
					if (m_TargetPointA == m_PatrolPointB[0])
					{
						m_TargetPointA = m_PatrolPointB[1];
					}
					else if (m_TargetPointA == m_PatrolPointB[1])
					{
						m_TargetPointA = m_PatrolPointB[2];
					}
					else if (m_TargetPointA == m_PatrolPointB[2])
					{
						m_TargetPointA = m_PatrolPointB[3];
					}
					else if (m_TargetPointA == m_PatrolPointB[3])
					{
						m_TargetPointA = m_PatrolPointB[4];
					}
					else if (m_TargetPointA == m_PatrolPointB[4])
					{
						m_TargetPointA = m_PatrolPointB[0];
					}
				}
			}


			//When within 15 either side enter aim state
			Matrix(2).RotateLocalY(m_TankTemplate->GetTurretTurnSpeed() * 0.5f * updateTime);

			CMatrix4x4 world = Matrix() * Matrix(2);
			CVector3 TurretFacingVector = Normalise(CVector3(world.e20, world.e21, world.e22));
			CVector3 TurretRightwardVector = Normalise(CVector3(world.e00, world.e01, world.e02));

			EntityManager.BeginEnumEntities("", "", "Tank");
			CEntity* entity = EntityManager.EnumEntity();
			while (entity != nullptr)
			{
				if (entity->GetUID() != GetUID())
				{
					//CHECKS NOT ON SAME TEAM
					if (m_Team != dynamic_cast<CTankEntity*>(entity)->GetTeam())
					{
						//CHECKS NOT ALREADY DEAD
						if (dynamic_cast<CTankEntity*>(entity)->GetHP() > 0)
						{
							m_TargetTank = entity->GetUID();
							if (EntityManager.GetEntity(m_TargetTank) != nullptr)
							{
								CVector3 target = Normalise(EntityManager.GetEntity(m_TargetTank)->Position() - Position());
								float angle = ToDegrees(acos(Dot(TurretFacingVector, target)));

								if (angle < 15.0f)
								{
									bool inHouse = false;
									//LINE EQUATIONS   y = mx + c

									// Left Wall
									float d = Position().x - -7.5f;
									float s = d / target.x;
									CVector3 r = Position() + target * s;
									if (r.z < 45.0f && r.z > 35.0f)
									{
										inHouse = true;
									}

									// Right Wall
									d = Position().x - 5.0f;
									s = d / target.x;
									r = Position() + target * s;
									if (r.z < 45.0f && r.z > 35.0f)
									{
										inHouse = true;
									}

									// Top Wall
									d = Position().z - 45.5f;
									s = d / target.z;
									r = Position() + target * s;
									if (r.x < -7.5f && r.x > 5.0f)
									{
										inHouse = true;
									}

									// Bottom Wall
									d = Position().z - 36.0f;
									s = d / target.z;
									r = Position() + target * s;
									if (r.x < -7.5f && r.x > 5.0f)
									{
										inHouse = true;
									}

									if (inHouse == false)
									{
										if (m_Ammo > 0)
										{
											m_State = Aim;
										}
									}
								}
							}
						}
					}
				}
				entity = EntityManager.EnumEntity();  //Moves onto next Entity
			}
			EntityManager.EndEnumEntities();
		}
		else if (m_State == Aim)
		{
			m_Timer += updateTime * 1;
			//Lock on to Target
			if (EntityManager.GetEntity(m_TargetTank) != nullptr)
			{

				CMatrix4x4 world = Matrix() * Matrix(2);
				CVector3 TurretFacingVector = Normalise(CVector3(world.e20, world.e21, world.e22));
				CVector3 TurretRightwardVector = Normalise(CVector3(world.e00, world.e01, world.e02));


				/*Matrix(2).RotateLocalY(m_TankTemplate->GetTurretTurnSpeed() * updateTime);*/
				CVector3 target = Normalise(EntityManager.GetEntity(m_TargetTank)->Position() - Position());
				float angle = ToDegrees(acos(Dot(TurretFacingVector, target)));
				float facer = Dot(TurretRightwardVector, target);


				if (m_Timer > 1.0f)
				{
					//Create Shell
					if (m_Ammo > 0)
					{
						TEntityUID shell = EntityManager.CreateShell("Shell Type 1", GetUID(), "", CVector3(Matrix().Position().x + (TurretFacingVector.x), 2.5f, Matrix().Position().z + (TurretFacingVector.z * 2)));
						EntityManager.GetEntity(shell)->Matrix().FaceDirection(TurretFacingVector);
						m_Ammo--;
						m_ShotsFired++;
					}
					//Enter Evade State
					m_TargetPointA = CVector3(Position().x + Random(-40.0f, 40.0f), 0.5f, Position().z + Random(-40.0f, 40.0f)); //choose a point within 40 units
					m_Timer = 0; //resets timer
					m_State = Evade; //enter evade state
				}

				if (angle > 2.0f)
				{
					float turretTurn = m_TankTemplate->GetTurretTurnSpeed();
					if (facer > 0)
					{
						Matrix(2).RotateLocalY(turretTurn * updateTime);
					}
					else
					{
						Matrix(2).RotateLocalY(-turretTurn * updateTime);
					}

				}
			}
			else
			{
				m_Timer = 0;
				m_State = Evade;
			}

		}
		else if (m_State == Evade)
		{
			//Moves
			EvadeMove(updateTime);

			//Turret point at front

			CVector3 TurretFacingVector = Normalise(CVector3(Matrix(2).e20, Matrix(2).e21, Matrix(2).e22));
			CVector3 TurretRightwardVector = Normalise(CVector3(Matrix(2).e00, Matrix(2).e01, Matrix(2).e02));
			CVector3 target = CVector3(0, 0, 1);
			float facer = Dot(TurretRightwardVector, target);
			float angle = ToDegrees(acos(Dot(TurretFacingVector, target)));
			float turretTurn = m_TankTemplate->GetTurretTurnSpeed();

			if (angle < 2.0f)
			{
				Matrix(2).FaceDirection(target);
			}
			else
			{
				if (facer > 0)
				{
					Matrix(2).RotateLocalY(turretTurn * updateTime);
				}
				else
				{
					Matrix(2).RotateLocalY(-turretTurn * updateTime);
				}

			}

			//Enters patrol when reaches new point
			if (Distance(Position(), m_TargetPointA) < 7.0f)
			{
				if (GetTeam() == 0)
				{
					m_TargetPointA = m_PatrolPointA[0];
				}
				else
				{
					m_TargetPointA = m_PatrolPointB[0];
				}
				m_State = Patrol;
			}
		}
		else if (m_State == Inactive) //reduces speed to 0 and game stops
		{
			m_Speed = 0;
		}
		else if (m_State == Hunting)
		{
			CVector3 TurretFacingVector = Normalise(CVector3(Matrix(2).e20, Matrix(2).e21, Matrix(2).e22));
			CVector3 TurretRightwardVector = Normalise(CVector3(Matrix(2).e00, Matrix(2).e01, Matrix(2).e02));
			CVector3 target = CVector3(0, 0, 1);
			float facer = Dot(TurretRightwardVector, target);
			float angle = ToDegrees(acos(Dot(TurretFacingVector, target)));
			float turretTurn = m_TankTemplate->GetTurretTurnSpeed();

			if (angle < 2.0f)
			{
				Matrix(2).FaceDirection(target);
			}
			else
			{
				if (facer > 0)
				{
					Matrix(2).RotateLocalY(turretTurn * updateTime);
				}
				else
				{
					Matrix(2).RotateLocalY(-turretTurn * updateTime);
				}

			}
			if (EntityManager.GetEntity(m_TargetAmmo) != nullptr)
			{
				CVector3 FacingVector = Normalise(CVector3(Matrix().e20, Matrix().e21, Matrix().e22));
				CVector3 RightwardVector = Normalise(CVector3(Matrix().e00, Matrix().e01, Matrix().e02));
				CVector3 target = Normalise(EntityManager.GetEntity(m_TargetAmmo)->Position() - Position());

				float angle = ToDegrees(acos(Dot(FacingVector, target)));

				if (angle < 2.0f)
				{
					Matrix().FaceTarget(EntityManager.GetEntity(m_TargetAmmo)->Position());
				}
				else
				{
					if (Dot(RightwardVector, target) > 0)
					{
						Matrix().RotateLocalY(m_MaxTurnSpeed * updateTime);
					}
					else
					{
						Matrix().RotateLocalY(-m_MaxTurnSpeed * updateTime);
					}
				}

				Matrix().MoveLocalZ(m_TankTemplate->GetMaxSpeed() * updateTime);
			}
			else
			{
				m_State = Patrol;
			}
		}

		return true; // Don't destroy the entity
	}


	void CTankEntity::PatrolMove(TFloat32 updateTime)
	{
		CVector3 FacingVector = Normalise(CVector3(Matrix().e20, Matrix().e21, Matrix().e22));
		CVector3 RightwardVector = Normalise(CVector3(Matrix().e00, Matrix().e01, Matrix().e02));
		CVector3 target = Normalise(m_TargetPointA - Position());

		float angle = ToDegrees(acos(Dot(FacingVector, target)));

		if (angle < 2.0f)
		{
			Matrix().FaceTarget(m_TargetPointA);
		}
		else
		{
			if (Dot(RightwardVector, target) > 0)
			{
				Matrix().RotateLocalY(m_MaxTurnSpeed * updateTime);
			}
			else
			{
				Matrix().RotateLocalY(-m_MaxTurnSpeed * updateTime);
			}
		}

		Matrix().MoveLocalZ(m_TankTemplate->GetMaxSpeed() * 0.75f * updateTime);

	}
	void CTankEntity::EvadeMove(TFloat32 updateTime)
	{
		CVector3 FacingVector = Normalise(CVector3(Matrix().e20, Matrix().e21, Matrix().e22));
		CVector3 RightwardVector = Normalise(CVector3(Matrix().e00, Matrix().e01, Matrix().e02));
		CVector3 target = Normalise(m_TargetPointA - Position());

		float angle = ToDegrees(acos(Dot(FacingVector, target)));

		if (angle < 5.0f)
		{
			Matrix().FaceTarget(m_TargetPointA);
		}
		else
		{
			if (Dot(RightwardVector, target) > 0)
			{
				Matrix().RotateLocalY(m_MaxTurnSpeed * updateTime);
			}
			else
			{
				Matrix().RotateLocalY(-m_MaxTurnSpeed * updateTime);
			}
		}

		Matrix().MoveLocalZ(m_TankTemplate->GetMaxSpeed() * updateTime);

	}

} // namespace gen
