#include "backend/looped_command.hpp"
#include "gta/enums.hpp"
#include "natives.hpp"
#include "util/entity.hpp"
#include <corecrt_math_defines.h>
namespace big
{
	class aimbot : looped_command
	{
		using looped_command::looped_command;

		struct aimbot_dat
		{
			Vector3 aim_lock;
			Vector3 smooth_factor;
			bool using_aimbot_first_time = true;
			Entity entity;
		} g_aimbot;

		virtual void on_tick() override
		{
			float local_fov_change = g.weapons.aimbot.fov;
			for (auto ped : entity::get_entities(false, true))
			{
				if (!ENTITY::IS_ENTITY_DEAD(ped, 0)) // Tracetype is always 17. LOS check
				{
					int relation = PED::GET_RELATIONSHIP_BETWEEN_PEDS(ped, self::ped); // relation for enemy check
					int type     = PED::GET_PED_TYPE(ped); // for police check, cop types are 6, swat is 27
					Vector3 world_position = ENTITY::GET_ENTITY_COORDS(ped, false);

					if (SYSTEM::VDIST2(self::pos.x,
					        self::pos.y,
					        self::pos.z,
					        world_position.x,
					        world_position.y,
					        world_position.z)
					    > (g.weapons.aimbot.distance * g.weapons.aimbot.distance))
						continue; // If the entity is further than our preset distance then just skip it

					if (PED::IS_PED_A_PLAYER(ped) && g.weapons.aimbot.on_player) // check if its a player
					{
						goto aimbot_handler;
					}
					else if (((relation == 4) || (relation == 5)) && g.weapons.aimbot.on_enemy) // relation 4 and 5 are for enemies
					{
						goto aimbot_handler;
					}
					else if (((type == 6 && !PED::IS_PED_MODEL(ped, rage::joaat("s_m_y_uscg_01"))) || type == 27 || // s_m_y_uscg_01 = us coast guard 1 (technically military)
					             PED::IS_PED_MODEL(ped, rage::joaat("s_m_y_ranger_01")) || PED::IS_PED_MODEL(ped, rage::joaat("s_f_y_ranger_01"))) // ranger models
					    && g.weapons.aimbot.on_police)
					{
						goto aimbot_handler;
					}
					else if (g.weapons.aimbot.on_npc && !PED::IS_PED_A_PLAYER(ped))

					// Update aim lock coords
					aimbot_handler:
					{
						if (!(ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(self::ped, ped, 17) || !g.weapons.aimbot.los_check))
							continue;

						// Jump to here to handle instead of continue statements
						g_aimbot.entity = ped;
						g_aimbot.aim_lock =
						    ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(ped, PED::GET_PED_BONE_INDEX(ped, g.weapons.aimbot.selected_bone));
					}
				}
			}
			if (!g_aimbot.entity || ENTITY::IS_ENTITY_DEAD(g_aimbot.entity, 0))
			{
				return;
			}
			if (PED::GET_PED_CONFIG_FLAG(self::ped, 78, 0))
			{
				Vector3 camera_target;

				if (g.weapons.aimbot.smoothing)
				{
					//Avoid buggy cam
					if (g_aimbot.using_aimbot_first_time)
					{
						Vector3 cam_coords               = CAM::GET_GAMEPLAY_CAM_COORD();
						Vector3 cam_rot                  = CAM::GET_GAMEPLAY_CAM_ROT(0);
						Vector3 cam_direction            = math::rotation_to_direction(cam_rot);
						float distance                   = 150.f;
						Vector3 multiply                 = cam_direction * distance;
						Vector3 front_cam                = cam_coords + multiply;
						camera_target                    = front_cam - CAM::GET_GAMEPLAY_CAM_COORD();
						g_aimbot.smooth_factor           = camera_target;
						g_aimbot.using_aimbot_first_time = false;
					}
					Vector3 target = g_aimbot.aim_lock - CAM::GET_GAMEPLAY_CAM_COORD();
					g_aimbot.smooth_factor.x += (target.x - g_aimbot.smooth_factor.x) * g.weapons.aimbot.smoothing_speed / 10.f;
					g_aimbot.smooth_factor.y += (target.y - g_aimbot.smooth_factor.y) * g.weapons.aimbot.smoothing_speed / 10.f;
					g_aimbot.smooth_factor.z += (target.z - g_aimbot.smooth_factor.z) * g.weapons.aimbot.smoothing_speed / 10.f;

					camera_target = g_aimbot.smooth_factor;
				}
				else
				{
					camera_target = g_aimbot.aim_lock - CAM::GET_GAMEPLAY_CAM_COORD();
				}
				if (g_aimbot.aim_lock.x == 0.f && g_aimbot.aim_lock.y == 0.f && g_aimbot.aim_lock.z == 0.f)
					return;

				float camera_heading = atan2f(camera_target.x, camera_target.y) * 180.0f / M_PI;
				float magnitude      = sqrtf(camera_target.x * camera_target.x + camera_target.y * camera_target.y
                    + camera_target.z * camera_target.z);

				float camera_pitch = asinf(camera_target.z / magnitude) * 180.0f / M_PI;
				float self_heading = ENTITY::GET_ENTITY_HEADING(self::ped);
				float self_pitch   = ENTITY::GET_ENTITY_PITCH(self::ped);
				if (camera_heading >= 0.0f && camera_heading <= 180.0f)
				{
					camera_heading = 360.0f - camera_heading;
				}
				else if (camera_heading <= -0.0f && camera_heading >= -180.0f)
				{
					camera_heading = -camera_heading;
				}
				if (CAM::GET_FOLLOW_PED_CAM_VIEW_MODE() == 4)
				{
					CAM::SET_FIRST_PERSON_SHOOTER_CAMERA_HEADING(camera_heading - self_heading);
					CAM::SET_FIRST_PERSON_SHOOTER_CAMERA_PITCH(camera_pitch - self_pitch);
				}
				else
				{
					CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(camera_heading - self_heading);
					CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(camera_pitch - self_pitch, 1.0f);
				}
			}
			else
			{
				g_aimbot.entity                  = 0;
				g_aimbot.using_aimbot_first_time = true;
			}
		}
		virtual void on_disable() override
		{
			g_aimbot.using_aimbot_first_time = true;
		}
	};

	aimbot
		g_aimbot("aimbot", "VIEW_OVERLAY_AIMBOT", "BACKEND_LOOPED_WEAPONS_AIMBOT_DESC", g.weapons.aimbot.enable);
	bool_command
		g_smoothing("smoothing", "BACKEND_LOOPED_WEAPONS_SMOOTHING", "BACKEND_LOOPED_WEAPONS_SMOOTHING_DESC", g.weapons.aimbot.smoothing);
	bool_command
	    g_aimbot_on_player("aimatplayer", "PLAYER", "BACKEND_LOOPED_WEAPONS_AIM_AT_PLAYER_DESC", g.weapons.aimbot.on_player);
	bool_command
		g_aimbot_on_npc("aimatnpc", "NPC", "BACKEND_LOOPED_WEAPONS_AIM_AT_NPC_DESC", g.weapons.aimbot.on_npc);
	bool_command
	    g_aimbot_on_police("aimatpolice", "POLICE", "BACKEND_LOOPED_WEAPONS_AIM_AT_POLICE_DESC", g.weapons.aimbot.on_police);
	bool_command
		g_aimbot_on_enemy("aimatenemy", "BACKEND_LOOPED_WEAPONS_AIM_AT_ENEMY", "BACKEND_LOOPED_WEAPONS_AIM_AT_ENEMY_DESC", g.weapons.aimbot.on_enemy);
	bool_command
		g_aimbot_los_check("loscheck", "LOS", "BACKEND_LOOPED_WEAPONS_LOS_DESC", g.weapons.aimbot.los_check);
}