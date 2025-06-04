#include "mapbot.h"

#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/client/prediction/entities/character.h>
#include <game/client/prediction/gameworld.h>
#include <engine/engine.h>
#include <game/client/render.h>
#include <game/mapitems.h>
#include <limits>
#include <mutex>
#include <thread>
#include <memory>

// Bro something is wrong with the input scoring.
// It seems to be too random and not really reliable.

// Why?
void CMapBot::OnInit()
{
	m_IsGenerating = false;
	m_PathFound = false;
	m_Paths.clear();
}

// I don't know why do I need this, but it seems to be necessary
void CMapBot::OnReset()
{
	m_IsGenerating = false;
	m_PathFound = false;
	m_Paths.clear();
}

void CMapBot::OnMapLoad()
{
	OnReset();
}

void CMapBot::OnRender()
{
    if(!g_Config.m_ZrMapBot || !m_pClient->m_Cheat.m_Active)
        return;

    // Get current player state
    vec2 pos = m_pClient->m_PredictedChar.m_Pos;
    vec2 vel = m_pClient->m_PredictedChar.m_Vel;

    // Generate possible movement paths
    m_Paths = PredictMovement(pos, vel); // TODO: this should be done in OnTick

    // Render the paths
    RenderPaths();
}

void CMapBot::GeneratePath()
{
	m_IsGenerating = true;
	m_PathFound = true;
}

std::vector<CMapBot::MovementPath> CMapBot::PredictMovement(vec2 pos, vec2 vel)
{
    std::vector<MovementPath> paths;
    const int PredictionSteps = g_Config.m_ZrMapBotTicks;

    float accuracy = clamp(static_cast<float>(g_Config.m_ZrMapBotAccuracy), 1.0f, 200.0f);
    float angleStep = 45.0f * (201.0f - accuracy) / 200.0f;
    angleStep = clamp(angleStep, 1.0f, 45.0f);

    // I love this part of code
    std::vector<int> directions = g_Config.m_ZrMapBotMove ? std::vector<int>{-1, 0, 1} : std::vector<int>{0};
    std::vector<int> jumps = g_Config.m_ZrMapBotJump ? std::vector<int>{0, 1} : std::vector<int>{0};
    std::vector<int> hookStates = g_Config.m_ZrMapBotHook ? std::vector<int>{0, 1} : std::vector<int>{0};
    std::vector<float> angles;
    if(g_Config.m_ZrMapBotHook)
        for(float angle = 0; angle < 360.0f; angle += angleStep)
            angles.push_back(angle);
    else
        angles = {0};

    if(!m_pClient->m_FlowFieldPathfinder.HasPath())
        return paths; // No path available

    vec2 finishPos = m_pClient->m_FlowFieldPathfinder.GetFinishPos();

    MovementPath bestPath;
    float maxScore = -std::numeric_limits<float>::infinity();
    size_t bestPathIndex = -1;

    for(int dir : directions)
    {
        for(int jump : jumps)
        {
            for(int hook : hookStates)
            {
                for(float angle : angles)
                {
                    std::vector<PredictionPoint> points;
                    PredictCharacterMovement(pos, vel, dir, jump, hook, angle, PredictionSteps, points);

                    // Calculate score for the path
                    float score = 0.0f;
                    bool isFrozen = false;

                    for(const auto &point : points)
                    {
                        if(point.m_IsFrozen)
                        {
                            isFrozen = true;
                            break;
                        }

                        vec2 flowDir = m_pClient->m_FlowFieldPathfinder.GetFlowDirection((int)point.m_Pos.x, (int)point.m_Pos.y);

                        // Proximity score
                        float dist = length(point.m_Pos - pos);
                        float distScore = 1.0f / (1.0f + dist / 32.0f);

                        // Alignment score
                        vec2 moveDir = normalize(vel);
                        float alignScore = dot(moveDir, flowDir);

                        // Combine scores
                        score += distScore * 0.4f + alignScore * 0.6f;
                    }

                    // Skip paths that result in freezing
                    if(isFrozen)
                        continue;

                    // Create a new path for this input combination
                    MovementPath path(vel, hook, jump, dir);
                    path.m_Positions.clear();
                    for(const auto &point : points)
                        path.m_Positions.push_back(point.m_Pos);

                    paths.push_back(path);

                    // Update the best path
                    if(score > maxScore)
                    {
                        maxScore = score;
                        bestPath = path;
                        bestPathIndex = paths.size() - 1; // Store the index of the best path
                    }
                }
            }
        }
    }

    // Mark the best path as selected
    if(bestPathIndex != -1)
    {
        paths[bestPathIndex].m_IsSelected = true;
    }

    return paths;
}

void CMapBot::RenderPaths()
{
    if(!g_Config.m_ZrMapBotRender)
        return;

    for(const auto &path : m_Paths)
    {
        // Different colors for different types of movement
        ColorRGBA color;
        if(path.m_IsSelected)
            color = ColorRGBA(0.0f, 1.0f, 1.0f, 0.5f); // Cyan for the selected path
        else if(path.m_IsHook && path.m_IsJump)
            color = ColorRGBA(1.0f, 0.0f, 1.0f, 0.3f); // Purple for hook+jump
        else if(path.m_IsHook)
            color = ColorRGBA(0.0f, 1.0f, 0.0f, 0.3f); // Green for hook
        else if(path.m_IsJump)
            color = ColorRGBA(0.0f, 0.0f, 1.0f, 0.3f); // Blue for jump
        else
            color = ColorRGBA(1.0f, 1.0f, 0.0f, 0.3f); // Yellow for normal movement

        // Draw smooth line through all positions
        for(size_t i = 0; i < path.m_Positions.size() - 1; i++)
        {
            m_pClient->m_Visuals.DrawLine(
                path.m_Positions[i].x, path.m_Positions[i].y,
                path.m_Positions[i + 1].x, path.m_Positions[i + 1].y,
                1.0f, color);
        }

        // Draw end point
        if(!path.m_Positions.empty())
        {
            float BoxSize = 4.0f;
            Graphics()->TextureClear();
            Graphics()->QuadsBegin();
            Graphics()->SetColor(color.r, color.g, color.b, 0.8f);
            vec2 endPos = path.m_Positions.back();
            IGraphics::CQuadItem QuadItem(endPos.x - BoxSize / 2, endPos.y - BoxSize / 2, BoxSize, BoxSize);
            Graphics()->QuadsDrawTL(&QuadItem, 1);
            Graphics()->QuadsEnd();
        }
    }
}

bool CMapBot::IsValidPosition(vec2 pos) const
{
	// Check if position is in solid tile
	return !m_pClient->Collision()->CheckPoint(pos.x, pos.y);
}

void CMapBot::PredictCharacterMovement(
	vec2 pos, vec2 vel,
	int dir, int jump, int hook, float angle,
	int PredictionSteps,
	std::vector<PredictionPoint> &points)
{
    // not really sure why creating a new world is required each time, but it works
	CGameWorld *tempWorld = new CGameWorld();
	tempWorld->m_pCollision = m_pClient->Collision();
	tempWorld->m_Teams = CTeamsCore();
	tempWorld->m_Core = m_pClient->m_GameWorld.m_Core;
	tempWorld->m_pTuningList = m_pClient->m_GameWorld.m_pTuningList;

	CNetObj_Character charObj = {};
	charObj.m_X = (int)pos.x;
	charObj.m_Y = (int)pos.y;
	charObj.m_VelX = (int)(vel.x * 256.0f);
	charObj.m_VelY = (int)(vel.y * 256.0f);

    // CNetObj_DDNetCharacter is required here (for some reason)
    CNetObj_DDNetCharacter ddnetCharObj = {};
    CCharacter *pChar = new CCharacter(tempWorld, 0, &charObj, &ddnetCharObj);
    tempWorld->InsertEntity(pChar, false);

    // Prepare the input
    CNetObj_PlayerInput input = {};
    input.m_Direction = dir;
    input.m_Jump = jump;
    input.m_Hook = hook;

    if(hook)
    {
        float rad = angle * pi / 180.0f;
        input.m_TargetX = (int)(cos(rad) * 256.0f);
        input.m_TargetY = (int)(sin(rad) * 256.0f);
    }
    else
    {
        input.m_TargetX = 0;
        input.m_TargetY = 0;
    }

    for(int i = 0; i < PredictionSteps; i++)
    {
        pChar->OnDirectInput(&input);
        pChar->OnPredictedInput(&input);

        for(CEntity *pEnt = tempWorld->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pEnt; pEnt = pEnt->TypeNext())
        {
            pEnt->PreTick();
            pEnt->Tick();
            pEnt->TickDeferred();
        }

        points.emplace_back(pChar->Core()->m_Pos, pChar->m_FreezeTime > 0);
    }

    delete tempWorld;
}