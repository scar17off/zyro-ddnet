#include "aimbot.h"

#include "game/client/gameclient.h"

// TODO: Remove this function and make it possible to configure each weapon
float CAimbot::GetWeaponReach(int Weapon)
{
    switch(Weapon)
    {
        case WEAPON_HAMMER:
            return 20.0f;
        case WEAPON_GUN:
		    return m_pClient->GetTuning(0)->m_GunSpeed * m_pClient->GetTuning(0)->m_GunLifetime;
        case WEAPON_SHOTGUN:
            return m_pClient->GetTuning(0)->m_ShotgunSpeed * m_pClient->GetTuning(0)->m_ShotgunLifetime;
        case WEAPON_GRENADE:
            return m_pClient->GetTuning(0)->m_GrenadeSpeed * m_pClient->GetTuning(0)->m_GrenadeLifetime;
        case WEAPON_LASER:
            return m_pClient->GetTuning(0)->m_LaserReach;
        default:
            return 0.0f;
    }
}

int CAimbot::SearchTarget()
{
    if(!m_pClient->m_Snap.m_pLocalCharacter)
        return -1;

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
            if(Distance < NearestDistance && Distance < WeaponReach && InFoV(TargetPosition, m_pClient->m_Snap.m_pLocalCharacter->m_Weapon))
            {
                NearestDistance = Distance;
                NearestPlayer = i;
            }
        }
    }

    // Update current target
    m_CurrentTarget = NearestPlayer;

    return NearestPlayer;
}

bool CAimbot::InFoV(vec2 Position, int Weapon)
{
    const WeaponConfig* pConfig = GetWeaponConfig(Weapon);

    // Convert FOV from degrees to radians (now using global FOV)
    float FovRadians = (g_Config.m_ZrAimbotFoV * pi) / 180.0f;
    
    // Get current aim direction
    vec2 CurrentAim = normalize(vec2(
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetX,
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetY
    ));

    // Check if any part of player hitbox is in FOV
    constexpr float ProximityRadius = CCharacterCore::PhysicalSize();
    vec2 Points[] = {
        Position + vec2(-ProximityRadius, -ProximityRadius), // Top left
        Position + vec2(ProximityRadius, -ProximityRadius),  // Top right
        Position + vec2(-ProximityRadius, ProximityRadius),  // Bottom left
        Position + vec2(ProximityRadius, ProximityRadius)    // Bottom right
    };

    for(const vec2 &Point : Points)
    {
        vec2 Dir = normalize(Point);
        float Dot = dot(Dir, CurrentAim);
        float AngleDiff = acos(clamp(Dot, -1.0f, 1.0f));
        if(AngleDiff <= FovRadians / 2.0f)
            return true;
    }

    return false;
}

void CAimbot::OnRender()
{
    if(!g_Config.m_ZrAimbotDrawFov)
        return;

    if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
        return;
    
    if(!m_pClient->m_Snap.m_pLocalCharacter)
        return;
    
    vec2 InitPos = m_pClient->m_LocalCharacterPos;
    int CurrentWeapon = m_pClient->m_Snap.m_pLocalCharacter->m_Weapon;
    float WeaponReach = GetWeaponReach(CurrentWeapon);
    
    const WeaponConfig* pConfig = GetWeaponConfig(CurrentWeapon);
    if(!pConfig || !*pConfig->m_pEnabled)
        return;
    
    vec2 Direction = normalize(m_pClient->m_Controls.m_aMousePos[g_Config.m_ClDummy]);
    float BaseAngle = angle(Direction);
    float FovRadians = (g_Config.m_ZrAimbotFoV * pi) / 180.0f;

    // Draw FOV lines
    vec2 Dir1 = direction(BaseAngle - FovRadians/2);
    vec2 Dir2 = direction(BaseAngle + FovRadians/2);
    
    vec2 FovPos1 = InitPos + Dir1 * WeaponReach;
    vec2 FovPos2 = InitPos + Dir2 * WeaponReach;
    
    m_pClient->m_Visuals.DrawLine(
        InitPos.x, InitPos.y,        // from
        FovPos1.x, FovPos1.y,        // to
        g_Config.m_ZrAimbotFovWidth, // width
        ColorRGBA(1,1,1,0.3f)        // color
    );
    
    m_pClient->m_Visuals.DrawLine(
        InitPos.x, InitPos.y,        // from
        FovPos2.x, FovPos2.y,        // to
        g_Config.m_ZrAimbotFovWidth, // width
        ColorRGBA(1,1,1,0.3f)        // color
    );
}

const WeaponConfig* CAimbot::GetWeaponConfig(int Weapon) const
{
    static WeaponConfig s_aWeaponConfigs[] = {
        {&g_Config.m_ZrAimbotHammerEnabled, "Hammer"},
        {&g_Config.m_ZrAimbotGunEnabled, "Gun"},
        {&g_Config.m_ZrAimbotShotgunEnabled, "Shotgun"},
        {&g_Config.m_ZrAimbotGrenadeEnabled, "Grenade"},
        {&g_Config.m_ZrAimbotLaserEnabled, "Laser"},
        {&g_Config.m_ZrAimbotNinjaEnabled, "Ninja"},
        {&g_Config.m_ZrAimbotHookEnabled, "Hook"}
    };

    switch(Weapon)
    {
        case WEAPON_HAMMER: return &s_aWeaponConfigs[0];
        case WEAPON_GUN: return &s_aWeaponConfigs[1];
        case WEAPON_SHOTGUN: return &s_aWeaponConfigs[2];
        case WEAPON_GRENADE: return &s_aWeaponConfigs[3];
        case WEAPON_LASER: return &s_aWeaponConfigs[4];
        case WEAPON_NINJA: return &s_aWeaponConfigs[5];
        default: return &s_aWeaponConfigs[6]; // Hook
    }
}