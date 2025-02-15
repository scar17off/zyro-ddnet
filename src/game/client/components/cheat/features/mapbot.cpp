#include "mapbot.h"
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/mapitems.h>
#include <engine/shared/config.h>
#include <game/client/prediction/gameworld.h>
#include <game/client/prediction/entities/character.h>
#include <thread>
#include <mutex>

/*
Bugs:
- The position scoring method isn't working. Make more scoring methods and combine them to get the best result.
*/

void CMapBot::OnInit()
{
    m_IsGenerating = false;
    m_PathFound = false;
    m_pGameWorld = nullptr;
    m_HasBestInput = false;
    m_LastUpdateTime = 0;
    m_StartPos = vec2(0, 0);
    m_FinishPos = vec2(0, 0);
}

void CMapBot::OnReset()
{
    m_IsGenerating = false;
    m_PathFound = false;
    
    if(m_pGameWorld)
    {
        delete m_pGameWorld;
        m_pGameWorld = nullptr;
    }
}

void CMapBot::OnMapLoad()
{
    OnReset();
}

void CMapBot::OnRender()
{
    if(!g_Config.m_ZrMapBot)
        return;
       
    RenderPath();
    UpdatePath();
}

void CMapBot::GeneratePath()
{
    if(m_IsGenerating)
        return;
        
    // Check if we have a valid path from pathfinder
    if(!m_pClient->m_FlowFieldPathfinder.HasPath())
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mapbot", "No path found! Press 'path' button first");
        return;
    }

    m_IsGenerating = true;
    m_PathFound = true;
    
    // Get flow field directions
    m_CurrentPath = m_pClient->m_FlowFieldPathfinder.GetPath();
    m_StartPos = m_pClient->m_FlowFieldPathfinder.GetStartPos();
    m_FinishPos = m_pClient->m_FlowFieldPathfinder.GetFinishPos();
    
    // Initialize game world for simulation
    InitGameWorld();
    
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mapbot", "Starting path generation...");
}

void CMapBot::UpdatePath()
{
    if(!m_IsGenerating || !m_pGameWorld)
        return;
        
    // Rate limiting
    int64_t Now = time_get();
    int64_t UpdateInterval = time_freq() / 50; // 50 Hz update rate

    if(Now - m_LastUpdateTime < UpdateInterval)
        return;
        
    m_LastUpdateTime = Now;

    CCharacter *pChar = (CCharacter *)m_pGameWorld->GetEntity(
        m_pClient->m_Snap.m_LocalClientId,
        CGameWorld::ENTTYPE_CHARACTER);
    
    if(!pChar)
        return;

    // Generate inputs for current position
    std::vector<InputState> inputs = GenerateInputs(pChar->Core()->m_Pos, pChar->Core()->m_Vel);
    
    // Find best input
    float bestScore = -1;
    m_HasBestInput = false;
    
    for(const auto& input : inputs)
    {
        if(input.m_PathScore > bestScore)
        {
            bestScore = input.m_PathScore;
            m_BestInput = input;
            m_HasBestInput = true;
        }
    }
    
    // Apply best input and update simulation
    if(m_HasBestInput)
    {
        UpdateGameWorld();
    }

    // Check if we reached finish
    if(length(pChar->Core()->m_Pos - m_FinishPos) < 32.0f)
    {
        m_IsGenerating = false;
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mapbot", "Reached finish!");
    }
}

void CMapBot::InitGameWorld()
{
    if(m_pGameWorld)
        return;

    // Create new game world
    m_pGameWorld = new CGameWorld();
    m_pGameWorld->m_pCollision = m_pClient->Collision();
    m_pGameWorld->m_WorldConfig.m_PredictFreeze = true;

    // Set up tuning
    m_pGameWorld->m_Core = m_pClient->m_GameWorld.m_Core;
    m_pGameWorld->m_pTuningList = m_pClient->m_GameWorld.m_pTuningList;
    m_pGameWorld->m_Teams = m_pClient->m_GameWorld.m_Teams;

    // Initialize character
    if(m_pClient->m_Snap.m_pLocalCharacter)
    {
        CNetObj_Character CharObj = *m_pClient->m_Snap.m_pLocalCharacter;
        CCharacter *pChar = new CCharacter(m_pGameWorld, m_pClient->m_Snap.m_LocalClientId, &CharObj);
        m_pGameWorld->InsertEntity(pChar);

        // Initialize weapons
        pChar->SetWeaponGot(WEAPON_HAMMER, true);
        pChar->SetWeaponAmmo(WEAPON_HAMMER, -1);
        pChar->SetWeaponGot(WEAPON_GUN, true);
        pChar->SetWeaponAmmo(WEAPON_GUN, 10);
        pChar->SetActiveWeapon(WEAPON_GUN);
    }
}

void CMapBot::RenderPath()
{
    if(!m_PathFound || m_PathPoints.empty())
        return;
    
    // Draw path lines
    for(size_t i = 0; i < m_PathPoints.size() - 1; i++)
    {
        m_pClient->m_Visuals.DrawLine(
            m_PathPoints[i].x, m_PathPoints[i].y,
            m_PathPoints[i + 1].x, m_PathPoints[i + 1].y,
            2.0f, ColorRGBA(1.0f, 1.0f, 0.0f, 0.5f)
        );
    }
}

void CMapBot::UpdateGameWorld()
{
    if(!m_pGameWorld || !m_HasBestInput)
        return;

    CCharacter *pChar = (CCharacter *)m_pGameWorld->GetEntity(
        m_pClient->m_Snap.m_LocalClientId,
        CGameWorld::ENTTYPE_CHARACTER);
    
    if(!pChar)
        return;

    // Apply input
    CNetObj_PlayerInput InputCopy = m_BestInput.m_Input;
    pChar->OnDirectInput(&InputCopy);
    pChar->OnPredictedInput(&InputCopy);

    // Update game world
    m_pGameWorld->Tick();

    // Store path point
    m_PathPoints.push_back(pChar->Core()->m_Pos);
}

std::vector<CMapBot::InputState> CMapBot::GenerateInputs(const vec2& pos, const vec2& vel)
{
    const int NUM_THREADS = 4;
    std::vector<InputState> inputs;
    std::vector<std::shared_ptr<CInputJob>> Jobs;

    // Split direction range into segments for threads
    int DirsPerThread = 3 / NUM_THREADS;
    
    // Create and queue jobs
    for(int i = 0; i < NUM_THREADS; i++)
    {
        int StartDir = -1 + i * DirsPerThread;
        int EndDir = StartDir + DirsPerThread;
        
        auto pJob = std::make_shared<CInputJob>(
            StartDir, EndDir,
            pos, vel,
            &inputs, this
        );
        
        Jobs.push_back(pJob);
        Kernel()->RequestInterface<IEngine>()->AddJob(std::shared_ptr<IJob>(pJob));
    }

    // Wait for all jobs to complete
    for(const auto& pJob : Jobs)
    {
        while(!pJob->Done())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return inputs;
}

void CMapBot::GenerateInputsThread(int StartDir, int EndDir, const vec2& pos, const vec2& vel, std::vector<InputState>& inputs)
{
    std::vector<InputState> LocalInputs;

    // Generate different input combinations (all possible inputs)
    for(int dir = StartDir; dir <= EndDir; dir++)
    {
        for(int hook = 0; hook <= 1; hook++)
        {
            for(int jump = 0; jump <= 1; jump++)
            {
                const float ACCURACY_FACTOR = g_Config.m_ZrMapBotAccuracy / 200.0f;
                const float AngleStep = 360.0f * (1.0f - ACCURACY_FACTOR) + 0.5f;
                
                for(float angle = 0; angle < 360.0f; angle += AngleStep)
                {
                    // Calculate hook target
                    float angleRad = angle * pi / 180.0f;
                    vec2 hookDir = vec2(cos(angleRad), sin(angleRad));
                    vec2 hookTarget = pos + hookDir * 400.0f;

                    // Set up input
                    CNetObj_PlayerInput input;
                    mem_zero(&input, sizeof(input));
                    
                    input.m_Direction = dir;
                    input.m_Jump = jump;
                    input.m_Hook = hook;
                    input.m_TargetX = (int)(hookTarget.x - pos.x);
                    input.m_TargetY = (int)(hookTarget.y - pos.y);

                    // Score position
                    float score = ScorePosition(pos, vel);
                    
                    if(score > 0)
                    {
                        InputState state;
                        state.m_Input = input;
                        state.m_MousePos = hookTarget;
                        state.m_Ticks = 1;
                        state.m_PathScore = score;
                        state.m_FinalPos = pos;
                        LocalInputs.push_back(state);
                    }
                }
            }
        }
    }

    if(!LocalInputs.empty())
    {
        std::lock_guard<std::mutex> Lock(m_InputMutex);
        inputs.insert(inputs.end(), LocalInputs.begin(), LocalInputs.end());
    }
}

float CMapBot::ScorePosition(const vec2& pos, const vec2& vel)
{
    int x = (int)pos.x / 32;
    int y = (int)pos.y / 32;
    
    if(!IsValidTile(x, y))
        return 0;
        
    // Get direction from flow field
    vec2 flowDir = m_pClient->m_FlowFieldPathfinder.GetFlowDirection(x, y);
    
    // Calculate alignment with flow field
    float alignScore = dot(normalize(vel), flowDir);
    
    // Calculate distance to finish
    float distScore = 1.0f / (1.0f + length(pos - m_FinishPos) / 32.0f);
    
    return alignScore * 0.7f + distScore * 0.3f;
}

bool CMapBot::IsValidTile(int x, int y) const
{
    return x >= 0 && x < m_pClient->Collision()->GetWidth() && 
           y >= 0 && y < m_pClient->Collision()->GetHeight() &&
           !m_pClient->Collision()->CheckPoint(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
}