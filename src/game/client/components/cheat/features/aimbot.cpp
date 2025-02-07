#include "aimbot.h"

#include "game/client/gameclient.h"
#include "game/tuning.h"

float CAimbot::GetWeaponReach(int Weapon)
{
    switch(Weapon)
    {
        case WEAPON_HAMMER:
            return 20.0f;
        case WEAPON_GUN:
            return m_pClient->m_aTuning->m_GunSpeed * m_pClient->m_aTuning->m_GunLifetime;
        case WEAPON_SHOTGUN:
            return m_pClient->m_aTuning->m_ShotgunSpeed * m_pClient->m_aTuning->m_ShotgunLifetime;
        case WEAPON_GRENADE:
            return m_pClient->m_aTuning->m_GrenadeSpeed * m_pClient->m_aTuning->m_GrenadeLifetime;
        case WEAPON_LASER:
            return m_pClient->m_aTuning->m_LaserReach;
        default:
            return 0.0f;
    }
}

int CAimbot::SearchTarget()
{
    int NearestPlayer = -1;
    float NearestDistance = 9999.0f;

    vec2 LocalPos = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientId].m_RenderPos;
    float WeaponReach = GetWeaponReach(m_pClient->m_Snap.m_pLocalCharacter->m_Weapon);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(m_pClient->m_aClients[i].m_Active &&
            i != m_pClient->m_Snap.m_LocalClientId)
        {
            vec2 PlayerPos = m_pClient->m_aClients[i].m_RenderPos;
            vec2 TargetPosition = PlayerPos - LocalPos;

            float Distance = distance(PlayerPos, LocalPos);
            if(Distance < NearestDistance && Distance < WeaponReach && InFoV(TargetPosition))
            {
                NearestDistance = Distance;
                NearestPlayer = i;
            }
        }
    }

    return NearestPlayer;
}

bool CAimbot::InFoV(vec2 Position)
{
    float Angle = atan2(Position.y, Position.x);

    float AngleDifference = Angle - atan2(m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetY, m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetX);
    return AngleDifference < g_Config.m_Cheat_Aimbot_FoV / 2.0f;
}