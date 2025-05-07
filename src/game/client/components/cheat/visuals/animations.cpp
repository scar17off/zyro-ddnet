#include "animations.h"
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/generated/client_data.h>

void CAnimations::OnReset()
{
    m_Points.clear();
    m_Iteration = 0.0f;
}

void CAnimations::InitializePoints()
{
    m_Points.clear();
    m_Points.reserve(g_Config.m_ZrAnimPoints); // Reserve exact space needed
    
    for(int i = 0; i < g_Config.m_ZrAnimPoints; i++)
    {
        float angle = (i / (float)g_Config.m_ZrAnimPoints) * pi * 2 - pi / 2;
        m_Points.push_back({
            vec2(
                g_Config.m_ZrAnimRadius * cos(angle),
                g_Config.m_ZrAnimRadius * sin(angle)
            ),
            angle
        });
    }
}

void CAnimations::OnRender()
{
    if(!g_Config.m_ZrAnim || !m_pClient->m_Snap.m_pLocalCharacter || !m_pClient->m_Cheat.m_Active)
        return;

    // Initialize points if needed or if point count changed
    static int s_LastPointCount = 0;
    if(m_Points.empty() || s_LastPointCount != g_Config.m_ZrAnimPoints)
    {
        InitializePoints();
        s_LastPointCount = g_Config.m_ZrAnimPoints;
    }

    // Update points
    UpdatePoints();

    // Render grenades
    RenderGrenades();

    // Update iteration
    m_Iteration += g_Config.m_ZrAnimSpeed / 100.0f;
}

void CAnimations::UpdatePoints()
{
    vec2 localPos = m_pClient->m_LocalCharacterPos;

    // Ensure we only update active points
    for(int i = 0; i < g_Config.m_ZrAnimPoints && i < (int)m_Points.size(); i++)
    {
        vec2 offset;
        
        // Calculate position based on selected animation
        switch(g_Config.m_ZrAnimType)
        {
            case ANIM_CIRCLE:
                offset = CalcCircle(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_EXPANDING_CIRCLE:
                offset = CalcExpandingCircle(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_WAVE:
                offset = CalcWave(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_EIGHT:
                offset = CalcEight(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_TRIANGLE:
                offset = CalcTriangle(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_DISK_X:
                offset = CalcDiskX(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_LEMNISCATE:
                offset = CalcLemniscate(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_SPIRAL:
                offset = CalcSpiral(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_COPY_DISK:
                offset = CalcCopyDisk(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_SQUARE:
                offset = CalcSquare(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_TOP_BOTTOM:
                offset = CalcTopBottom(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_X:
                offset = CalcX(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_RIGHT_LEFT:
                offset = CalcRightLeft(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_RANDOM_TB:
                offset = CalcRandomTB(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_ATOM:
                offset = CalcAtom(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_VERTICAL_SECTOR:
                offset = CalcVerticalSector(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_HORIZONTAL_SECTOR:
                offset = CalcHorizontalSector(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_RHOMBUS:
                offset = CalcRhombus(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            case ANIM_HEART:
                offset = CalcHeart(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
            default:
                offset = CalcCircle(i, g_Config.m_ZrAnimPoints, g_Config.m_ZrAnimRadius, m_Iteration);
                break;
        }

        m_Points[i].m_Pos = localPos + offset;
        m_Points[i].m_Angle = angle(offset);
    }
}

void CAnimations::RenderGrenades()
{
    // Save screen mapping
    float Points[4];
    Graphics()->GetScreen(Points, Points+1, Points+2, Points+3);

    // Map screen coordinates
    RenderTools()->MapScreenToGroup(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y, Layers()->GameGroup(), m_pClient->m_Camera.m_Zoom);

    // Begin rendering
    Graphics()->WrapClamp();
    Graphics()->TextureSet(m_pClient->m_GameSkin.m_aSpriteWeaponProjectiles[WEAPON_GRENADE]);
    Graphics()->QuadsBegin();
    Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Update time (copied from items.cpp)
    static float s_Time = 0.0f;
    static float s_LastLocalTime = LocalTime();

    if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
    {
        const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
        if(!pInfo->m_Paused)
            s_Time += (LocalTime() - s_LastLocalTime) * pInfo->m_Speed;
    }
    else
    {
        if(m_pClient->m_Snap.m_pGameInfoObj && !(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags & GAMESTATEFLAG_PAUSED))
            s_Time += LocalTime() - s_LastLocalTime;
    }

    // Store previous positions for smoke trail
    static std::vector<vec2> s_PrevPositions;
    if(s_PrevPositions.size() != m_Points.size())
        s_PrevPositions.resize(m_Points.size());

    // Render grenades and smoke trails
    for(int i = 0; i < g_Config.m_ZrAnimPoints && i < (int)m_Points.size(); i++)
    {
        const auto& point = m_Points[i];
        
        // Calculate velocity for smoke trail
        vec2 Vel = point.m_Pos - s_PrevPositions[i];
        if(length(Vel) > 0)
            Vel = normalize(Vel) * 50.0f;
        
        // Render smoke trail
        m_pClient->m_Effects.SmokeTrail(
            point.m_Pos,    // Current position
            Vel * -1,       // Negative velocity for trail direction
            1.0f,           // Alpha
            0.0f            // Time passed
        );

        // Store current position for next frame
        s_PrevPositions[i] = point.m_Pos;

        // Render grenade
        Graphics()->QuadsSetRotation(s_Time * pi * 2 * 2 + point.m_Angle);
        Graphics()->QuadsSetSubset(0, 0, 1, 1);
        IGraphics::CQuadItem QuadItem(point.m_Pos.x, point.m_Pos.y, 32.0f, 32.0f);
        Graphics()->QuadsDraw(&QuadItem, 1);
    }

    s_LastLocalTime = LocalTime();
    Graphics()->QuadsEnd();
    Graphics()->WrapNormal();

    // Restore screen mapping
    Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
}

// Animation calculation functions
vec2 CAnimations::CalcCircle(int index, int totalPoints, float radius, float speed)
{
    float baseAngle = (index / (float)totalPoints) * pi * 2;
    float angle = baseAngle + speed - pi / 2;
    return vec2(
        radius * cos(angle),
        radius * sin(angle)
    );
}

vec2 CAnimations::CalcExpandingCircle(int index, int totalPoints, float radius, float speed)
{
    float angle = (index / (float)totalPoints) * pi * 2;
    float scale = 1 + sin(speed);
    return vec2(
        radius * cos(angle) * scale,
        radius * sin(angle) * scale
    );
}

vec2 CAnimations::CalcWave(int index, int totalPoints, float radius, float speed) 
{
    float t = (2 * pi / totalPoints) * index + speed;
    return vec2(
        radius * cos(t),
        radius * sin(2 * t)
    );
}

vec2 CAnimations::CalcEight(int index, int totalPoints, float radius, float speed)
{
    float t = speed;
    float angle = (index / (float)totalPoints) * pi * 2 + t;
    float scale = radius / 1.4142f; // sqrt(2) to fit in radius
    return vec2(
        scale * sin(2 * angle) / 2,
        -scale * cos(angle) * cos(t)
    );
}

vec2 CAnimations::CalcTriangle(int index, int totalPoints, float radius, float speed)
{
    float t = pi * 2 / totalPoints * index + speed;
    return vec2(
        (2 * sin(t) + sin(2 * t)) * radius / 2,
        (2 * cos(t) - cos(2 * t)) * radius / 2
    );
}

vec2 CAnimations::CalcDiskX(int index, int totalPoints, float radius, float speed)
{
    float t = pi * 2 / totalPoints * index + speed;
    float t1 = pi * 2 / totalPoints * index - speed;

    if(index % 2 == 0)
    {
        return vec2(
            cos(t) * (radius * 1.5f),
            sin(t1) * (radius * 1.5f)
        );
    }
    else
    {
        return vec2(
            cos(t1) * radius,
            sin(t) * radius
        );
    }
}

vec2 CAnimations::CalcLemniscate(int index, int totalPoints, float radius, float speed)
{
    float t = speed + (index / (float)totalPoints) * pi * 2;
    float denom = 1 + sin(t) * sin(t);
    return vec2(
        (radius * cos(t)) / denom,
        (radius * cos(t) * sin(t)) / denom
    );
}

vec2 CAnimations::CalcSpiral(int index, int totalPoints, float radius, float speed)
{
    float growth = 0.1f;
    float baseAngle = (index / (float)totalPoints) * pi * 2;
    float baseRadius = radius * baseAngle * growth;
    float angle = baseAngle + speed;
    return vec2(
        baseRadius * cos(angle),
        baseRadius * sin(angle)
    );
}

vec2 CAnimations::CalcCopyDisk(int index, int totalPoints, float radius, float speed)
{
    float t = pi * 2 / totalPoints * index + speed;
    float t1 = pi / totalPoints * index + speed;
    return vec2(
        (2 * sin(t) + sin(2 * t1)) * radius / 2,
        (2 * cos(t) - cos(2 * t1)) * radius / 2
    );
}

vec2 CAnimations::CalcSquare(int index, int totalPoints, float radius, float speed)
{
    float t = speed * 0.1f;
    float basePosition = index / (float)totalPoints;
    float position = fmod(basePosition + t, 1.0f);
    float sidePosition = position * 4; // Four sides
    int sideIndex = (int)sidePosition;
    float progress = fmod(sidePosition, 1.0f); // Progress within current side

    switch(sideIndex)
    {
        case 0: // Top: left to right
            return vec2(radius * (-1 + 2 * progress), -radius);
        case 1: // Right: top to bottom
            return vec2(radius, radius * (-1 + 2 * progress));
        case 2: // Bottom: right to left
            return vec2(radius * (1 - 2 * progress), radius);
        default: // Left: bottom to top
            return vec2(-radius, radius * (1 - 2 * progress));
    }
}

vec2 CAnimations::CalcTopBottom(int index, int totalPoints, float radius, float speed)
{
    float t = speed;
    float xPos = (index / (float)totalPoints) * 2 - 1; // -1 to 1
    return vec2(
        radius * xPos,
        radius * sin(t + xPos * pi)
    );
}

vec2 CAnimations::CalcX(int index, int totalPoints, float radius, float speed)
{
    float r = 2 * pi * 2 / totalPoints * index + speed;
    if(index % 2 == 0)
    {
        float s = sin(r);
        return vec2(
            (s * 15 + 3 * s) * radius / 15,
            (cos(r) * 3 + 15 * s) * radius / 15
        );
    }
    else
    {
        float c = cos(r);
        return vec2(
            (c * 8 + 9 * c) * radius / 15,
            (sin(r) * 3 + -15 * c) * radius / 15
        );
    }
}

vec2 CAnimations::CalcRightLeft(int index, int totalPoints, float radius, float speed)
{
    float t = speed;
    float yPos = (index / (float)totalPoints) * 2 - 1; // -1 to 1
    return vec2(
        radius * sin(t + yPos * pi),
        radius * yPos
    );
}

vec2 CAnimations::CalcRandomTB(int index, int totalPoints, float radius, float speed)
{
    return vec2(
        cos(2 - totalPoints * index - speed) * radius,
        sin(2 / (float)totalPoints * index + speed) * radius
    );
}

vec2 CAnimations::CalcAtom(int index, int totalPoints, float radius, float speed)
{
    if(index >= totalPoints / 2)
    {
        return vec2(
            cos(2 * pi * 2 / totalPoints * index + speed) * radius / 2,
            sin(2 * pi * 2 / totalPoints * index + speed + 2) * radius / 2
        );
    }
    else
    {
        return vec2(
            cos(2 * pi * 2 / totalPoints * index + speed + 2) * radius / 2,
            sin(2 * pi * 2 / totalPoints * index + speed) * radius / 2
        );
    }
}

vec2 CAnimations::CalcVerticalSector(int index, int totalPoints, float radius, float speed)
{
    float t = speed;
    float angle = (index / (float)totalPoints) * pi - pi / 2;
    return vec2(
        radius * cos(angle) * cos(t),
        radius * sin(angle)
    );
}

vec2 CAnimations::CalcHorizontalSector(int index, int totalPoints, float radius, float speed)
{
    float t = speed;
    float angle = (index / (float)totalPoints) * pi - pi / 2;
    return vec2(
        radius * cos(angle),
        radius * sin(angle) * cos(t)
    );
}

vec2 CAnimations::CalcRhombus(int index, int totalPoints, float radius, float speed)
{
    float t = speed;
    float phase = (index / (float)totalPoints) * pi * 2;
    
    // Calculate position based on which side of rhombus we're on
    int side = (int)(phase / (pi/2)); // 0-3 for four sides
    float sidePhase = fmod(phase, pi/2) / (pi/2); // 0-1 within each side
    
    vec2 pos;
    switch(side) {
        case 0: // Top right
            pos = vec2(
                radius * (1 - sidePhase),
                radius * sidePhase
            );
            break;
        case 1: // Bottom right 
            pos = vec2(
                -radius * sidePhase,
                radius * (1 - sidePhase)
            );
            break;
        case 2: // Bottom left
            pos = vec2(
                -radius * (1 - sidePhase),
                -radius * sidePhase
            );
            break;
        default: // Top left
            pos = vec2(
                radius * sidePhase,
                -radius * (1 - sidePhase)
            );
            break;
    }
    
    // Rotate based on time
    float rot = t * 0.5f;
    return vec2(
        pos.x * cos(rot) - pos.y * sin(rot),
        pos.x * sin(rot) + pos.y * cos(rot)
    );
}

vec2 CAnimations::CalcHeart(int index, int totalPoints, float radius, float speed)
{
    float t = speed;
    float angle = (index / (float)totalPoints) * pi * 2 + t;
    return vec2(
        radius * pow(sin(angle), 3),
        radius * (
            -0.8125f * cos(angle) +
            0.3125f * cos(2 * angle) +
            0.125f * cos(3 * angle) +
            0.0625f * cos(4 * angle)
        )
    );
}