#include "balancebot.h"

#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <limits>
#include <optional>

// used modern c++17 features here
// it saved me like 20 lines of code xd (but wasted 2 hours of time to learn it)

bool CBalanceBot::IsValidVerticalPosition(const vec2& targetPos, const vec2& localPos) const
{
    switch(g_Config.m_ZrBalanceBotVFilter)
    {
        case 0: return targetPos.y > localPos.y; // Below only
        case 1: return targetPos.y < localPos.y; // Above only
        case 2: [[fallthrough]];
        default: return true; // Both
    }
}

bool CBalanceBot::CanReachTarget(const vec2& localPos, const vec2& targetPos) const
{
    // Check if target position is valid
    if(!IsValidVerticalPosition(targetPos, localPos))
        return false;

    vec2 hookPos = localPos;
    vec2 scanDir = targetPos - localPos;
    float len = m_pClient->m_aTuning->m_HookLength - 28.0f * 1.5f;
    float len_Static = m_pClient->m_aTuning->m_HookLength;

    vec2 exDirection = normalize(scanDir);
    vec2 finishPos = hookPos + exDirection * len;
    vec2 oldPos = hookPos + exDirection * 28.0f * 1.5f;
    vec2 newPos = oldPos;

    bool DoBreak = false;
    do
    {
        oldPos = newPos;
        newPos = oldPos + exDirection * m_pClient->m_aTuning->m_HookFireSpeed;

        if(distance(hookPos, newPos) > len_Static)
        {
            newPos = hookPos + normalize(newPos - hookPos) * len_Static;
            DoBreak = true;
        }

        int TeleNr = 0;
        const int Hit = m_pClient->Collision()->IntersectLineTeleHook(oldPos, newPos, &finishPos, nullptr, &TeleNr);

        // Check if we can hit the target
        vec2 closestPoint;
        if(closest_point_on_line(oldPos, newPos, targetPos, closestPoint))
        {
            if(distance(targetPos, closestPoint) < 28.0f + 2.0f)
                return true;
        }

        if(Hit)
            return false;

        newPos.x = round_to_int(newPos.x);
        newPos.y = round_to_int(newPos.y);

        if(oldPos == newPos)
            break;

        exDirection.x = round_to_int(exDirection.x * 256.0f) / 256.0f;
        exDirection.y = round_to_int(exDirection.y * 256.0f) / 256.0f;
    } while(!DoBreak);

    return true;
}

std::optional<CBalanceBot::TargetInfo> CBalanceBot::FindTarget(const vec2& localPos, float maxDistance) const
{
    float minDist = std::numeric_limits<float>::max();
    std::optional<CBalanceBot::TargetInfo> bestTarget;

    for(int i = 0; i < MAX_CLIENTS; ++i)
    {
        if(i == m_pClient->m_Snap.m_LocalClientId)
            continue;
            
        const auto& client = m_pClient->m_aClients[i];
        if(!client.m_Active || client.m_Team == TEAM_SPECTATORS)
            continue;

        const vec2& playerPos = client.m_Predicted.m_Pos;
        
        if(!CanReachTarget(localPos, playerPos))
            continue;

        const float dist = distance(playerPos, localPos);
        if(dist < minDist && dist < maxDistance)
        {
            minDist = dist;
            bestTarget.emplace(i, playerPos, dist);
        }
    }
    
    return bestTarget;
}

// yes yes this is that easy to register a console command guys just don't forget to add it to the binds.cpp # CBinds::SetDefaults method
void CBalanceBot::OnConsoleInit()
{
    Console()->Register("zrbalance", "", CFGFLAG_CLIENT, ConToggle, this, "Toggle balance bot");
}

void CBalanceBot::ConToggle(IConsole::IResult *pResult, void *pUserData)
{
    CBalanceBot *pBalanceBot = (CBalanceBot *)pUserData;
    pBalanceBot->m_Active = !pBalanceBot->m_Active;
}

void CBalanceBot::Balance()
{
    if(!g_Config.m_ZrBalanceBot || !m_Active || !m_pClient->m_Snap.m_pLocalCharacter)
        return;

    struct PlayerState {
        vec2 pos;
        vec2 vel;
        vec2 predictedPos;
    };

    const auto getPlayerState = [this](bool precise) -> PlayerState {
        if(precise) {
            // Precise mode - use predicted positions
            const auto& client = m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientId];
            const vec2 pos = client.m_Predicted.m_Pos;
            const vec2 vel = client.m_Predicted.m_Vel;
            return {pos, vel, pos + vel};
        } else {
            // Normal mode - use raw positions
            const vec2 pos = m_pClient->m_LocalCharacterPos;
            const vec2 vel = {
                m_pClient->m_Snap.m_pLocalCharacter->m_VelX/256.0f,
                m_pClient->m_Snap.m_pLocalCharacter->m_VelY/256.0f
            };
            return {pos, vel, pos + vel};
        }
    };

    const auto state = getPlayerState(g_Config.m_ZrBalanceBotPrecise);
    const float maxDistance = m_pClient->m_aTuning->m_HookLength;
    
    const auto maybeTarget = FindTarget(state.predictedPos, maxDistance);
    if(!maybeTarget.has_value())
        return;

    const auto& target = maybeTarget.value();
    const float horizontalDist = state.predictedPos.x - target.pos.x;
    const float stopDistance = g_Config.m_ZrBalanceBotPrecise ? 0.25f : 0.5f;

    if(std::abs(horizontalDist) > stopDistance) 
    {
        const int direction = (horizontalDist > 0) ? -1 : 1;
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Direction = direction;
    }
    else 
    {
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_Direction = 0;
    }
}