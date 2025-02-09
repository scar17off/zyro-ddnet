#include "hook_hitscan.h"
#include "game/client/gameclient.h"

vec2 CHookHitscan::EdgeScan(int ClientId)
{
    vec2 myPos = m_pClient->m_PredictedChar.m_Pos;
    vec2 targetPos = m_pClient->m_aClients[ClientId].m_Predicted.m_Pos;

    // Try direct hook first
    if(HitScanHook(myPos, targetPos, targetPos - myPos))
        return targetPos - myPos;

    // Constants for scanning
    const float PLAYER_SIZE = 28.0f;
    const int MAX_HIT_POINTS = 8; // This should stay static
    
    // Calculate scan step based on accuracy setting (1-200 range)
    float accuracy = clamp(g_Config.m_ZrAimbotHookAccuracy, 1, 200) / 200.0f;
    const float SCAN_STEP = pi / 18.0f * (1.0f - accuracy) + pi / 360.0f * accuracy;
    const float FULL_CIRCLE = 2.0f * pi;
    
    // Scan around the entire player hitbox
    std::vector<vec2> hitPoints;
    hitPoints.reserve(MAX_HIT_POINTS);
    
    for(float angle = 0.0f; angle < FULL_CIRCLE; angle += SCAN_STEP)
    {
        // Calculate point on player's hitbox
        vec2 hitboxPoint = vec2(
            targetPos.x + cosf(angle) * PLAYER_SIZE,
            targetPos.y + sinf(angle) * PLAYER_SIZE
        );

        vec2 hookDir = hitboxPoint - myPos;
        
        // Skip if angle is too steep
        float hookAngle = atan2f(hookDir.y, hookDir.x);
        if(abs(hookAngle) > pi * 0.8f)
            continue;

        if(HitScanHook(myPos, targetPos, hookDir))
        {
            hitPoints.push_back(hookDir);
            if(hitPoints.size() >= MAX_HIT_POINTS)
                break;
        }
    }

    return !hitPoints.empty() ? hitPoints[hitPoints.size() / 2] : vec2(0, 0);
}

bool CHookHitscan::HitScanHook(vec2 initPos, vec2 targetPos, vec2 scanDir)
{
    float len = m_pClient->m_aTuning->m_HookLength - 42.0f;
    float len_Static = m_pClient->m_aTuning->m_HookLength;

    if(!m_pClient->Input()->KeyIsPressed(KEY_MOUSE_2))
        len = len_Static;

    vec2 exDirection = normalize(scanDir);
    vec2 finishPos = initPos + exDirection * len;

    vec2 oldPos = initPos + exDirection * 42.0f;
    vec2 newPos = oldPos;

    bool DoBreak = false;

    do
    {
        oldPos = newPos;
        newPos = oldPos + exDirection * m_pClient->m_aTuning->m_HookFireSpeed;

        if(distance(initPos, newPos) > len_Static)
        {
            newPos = initPos + normalize(newPos - initPos) * len_Static;
            DoBreak = true;
        }

        int TeleNr = 0;
        const int Hit = m_pClient->Collision()->IntersectLineTeleHook(oldPos, newPos, &finishPos, nullptr, &TeleNr);

        if(IntersectCharacter(oldPos, targetPos, finishPos))
            return true;

        if(Hit)
            break;

        newPos.x = round_to_int(newPos.x);
        newPos.y = round_to_int(newPos.y);

        if(oldPos == newPos)
            break;

        exDirection.x = round_to_int(exDirection.x * 256.0f) / 256.0f;
        exDirection.y = round_to_int(exDirection.y * 256.0f) / 256.0f;
    } while(!DoBreak);
    
    return false;
}

bool CHookHitscan::IntersectCharacter(vec2 hookPos, vec2 targetPos, vec2 &newPos)
{
    vec2 closestPoint;
    if(closest_point_on_line(hookPos, newPos, targetPos, closestPoint))
    {
        if(distance(targetPos, closestPoint) < 28.f + 2.f)
        {
            newPos = closestPoint;
            return true;
        }
    }
    return false;
}