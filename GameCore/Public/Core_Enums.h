#pragma once

namespace GameCore
{
	enum class PlayerLocoID { Grounded, Airborne, Climb, End };

	//Idle + 유지성 액션(가드 등)
	enum class PlayerStanceID { None, Guard, Parry, End };
	
	enum class PlayerActionID
	{
		None, 
		Run, Fall, Jump, Roll, Dodge, Interact,
		Attack/*: 공용 ID*/, Light_Attack, Heavy_Attack, Fable_Arts, Fable_EX, Fatal,
		Guard, Hit, Dead, 
		Grinder, Heal, Lamp, SwapWeapon,
		LegionArm, LegionArm_Ex, Item_Use,
		End
	};

	enum class AnimAction
	{
		Move, Roll, Dodge, Jump, KnockBack, Guard,	// Action 
		Light, Heavy, DashLight, DashHeavy, ChargeHeavy, Fable, FableEX, Fatal,		// Attack
		Hit, Dead, GuardHit, ParrySuccess,			// Reaction
		Heal, Lamp_On, Lamp_Off, SwapWeapon, EquipWeapon, 		// Interact
		Grinder, TwoHand_Grinder, OneHand_Grinder,
		Grapple, Grapple_Upgrade, Grapple_Aim, ChargeGun,			// Legion Arm 
		Raxasia_Grab, Blast_Launch,
		Stand_Up, Labber,

		/*Ladder */
		Ladder_StartUp, Ladder_StartDown,
		Ladder_UpLeft, Ladder_UpRight, Ladder_DownLeft, Ladder_DownRight,
		Ladder_IdleLeft, Ladder_IdleRight,
		Ladder_EndUpLeft, Ladder_EndUpRight, Ladder_EndDownLeft, Ladder_EndDownRight, Ladder_ExitIdle,
		Ladder_Activate,

		/* Interative */
		Stargazer_Active, Stargazer_InActive,

		Item_Grab, Item_PickUp, Elevator_Activate,
		Open_Chest, 
		Door_OpenCheck, Door_Unlock,
		Open_SlidingDoor,  
		Open_DoubleDoor_Big, Open_DoubleDoor_Boss, Open_DoubleDoor_Hotel, Open_DoubleDoor_Slide, Open_Sophia_Door,
		Crash_BossDoor,
		/* Game */
		Restart, Teleport_Start,

		/* Stat Room */
		StatRoom_Weapon, StatRoom_LegionArm,

		/*Scene*/
		Sophia_Cine_HandStart, Sophia_Cine_HandEnd, Sophia_Cine_Idle, Sophia_Cine_Walk,

		End
	};
	enum class AttackType { Light, Heavy, DashLight, DashHeavy, ChargeHeavy, Fable, FableEX, Fatal, End };
	
	enum class ACTION_INPUTTYPE { GUARD, LIGHT_ATTACK, HEAVY_ATTACK, FABLE_BASE,FATAL, EX_MOVE, GRINDER, HEAL, SWAP, LEGION, LEGION_EX, INTERACT, ITEM_USE, LAMP, END };

	enum class WeaponType { UnArmed, OneHand, TwoHand, Pole, End };

	enum class ActionCategory : uint8_t { Attack, Action, Reaction, End };

	enum class LegionArmType { ProtoArm, Grapple, ChargeGun, End };

	enum class SPAWN_MONSTER_TYPE { BUTTLER_BASE, BUTTLER_TRAIN, BUTTLER_NOHEAD, BUTTLER_RANGE, MONSTER_ELITE, TRAINING, WATCH_DOG, BOSS_FESTIVAL, BOSS_RAXASIA_P1, BOSS_RAXASIA_P2, BALISTAR, CANNON, BUTTERFLY, BIG_DOG, ZOMBIE, PRIEST, BOSS_DONKEY,END };
	
	enum class SPAWN_WEAPON_TYPE { AUTO, CANDLE, SIGN, KNIFE, RANGE, ELITE, BOSS, END };
	
	enum class PLAYER_FX { AttackTrail, Parry, GuardHit, GrindLoop, TurnOnLamp, TurnOffLamp, End };

	enum class REACT_TYPE { Hit, Guard, GuardHeavy, GuardBreak, KnockBack, Normal_UnGuard, Heavy_UnGuard, End };

	//챕터 전환 상태 
	enum class CHAPTER_STATE { ACTIVE, FADE_OUT, UNLOAD, LOAD, LOAD_WAIT, CREATE, RESETTING, FADE_IN, END};

	enum class TRANSITION_KIND { NONE, CHANGE_CHAPTER, RESPAWN, END };
	
	enum class GRAB_STATE { GRAB, THROW, END };

	enum class GrappleState { Idle, Fire, Pull, Fail, Chase, Smash, Zip, Aim, End };

	enum class ChargeGunState { Idle, Enter, Aim, Fire, Exit, End };

	enum class LADDER_CLIMB_STATE { ENTER, IDLE, STEP_UP, STEP_DOWN, EXIT_TOP, EXIT_BOTTOM, END };

	enum class ITEM_SLOT { Grinder,  Heal , End };

	enum class LegionArmEvent
	{
		//Grapple
		Grapple_Accepted, Grapple_AimAccepted, Grapple_WireLaunch, Grapple_Pull, Grapple_Fail, Grapple_Chase, Grapple_Smash_Strike,  Grapple_Smash_Jump,
		//ChageGun
		ChargeGun_Accepted, ChargeGun_AimReady, ChargeGun_Fire, ChargeGun_Exit,
		End
	};
	enum PIECE_TYPE
	{
		PIECE_MERGE,
		PIECE_LEFT,
		PIECE_RIGHT,
		PIECE_END
	};

	enum class OBJECT_TYPE
	{
		BROKEN_STARGAZER,
		STARGAZER,
		LADDER_UP,
		LADDER_DOWN,
		DOOR,
		LADDER_ACTIVATOR,
		ELEVATOR_CONTROLLER,
		ITEM,
		TREASURECHEST,
		NPC,
		END
	};

	// 외부 상호작용 Trigger 대상이 Player에게 요청하는 실제 상호작용 종류다.
	enum class INTERACTION_TYPE
	{
		BROKEN_STARGAZER,
		STARGAZER,
		LADDER_UP,
		LADDER_DOWN,
		DOOR_BIG,	// 슬라이드 락사시아 
		DOOR_BOSS,  // 페스티벌
		DOOR_HOTEL,	// 호텔 입구 
		DOOR_SLIDE, // 기차역 슬라이드
		LADDER_ACTIVATOR,
		ELEVATOR_CONTROLLER,
		ITEM,
		TREASURE_CHEST,
		NPC,
		END
	};

	// 하나의 ITEM 월드 오브젝트가 전달할 실제 보상 종류다.
	enum class ITEM_TYPE
	{
		ERGO,
		WEAPON,
		LEGION_ARM,
		BODY_PART,
		END
	};

	enum class NPC_TYPE
	{
		SOPHIA,
		ANTONIA,
		POLENDINA,
		VENDOR,
		RIDDLEKING,
		EUGENIE,
		MADDONKEY,
		END
	};

	enum class PLAYER_PART
	{
		HAIR, 
		FACE, 
		BODY, 
		HELMET, 
		ACC,
		ARM, 
		END
	};

	enum class PlayerAnimEvent
	{
		Heal,
		Turn_On_Lamp,
		Turn_Off_Lamp,
		End
	};

	enum class BUFF_TYPE
	{
		AttackUp,
		DamageTakenUp,
		End
	};


}
