#include "laser_prediction.h"

#include <game/client/prediction/entities/laser.h>
#include <game/client/gameclient.h>
#include <engine/engine.h>
#include <thread>
#include <chrono>

std::vector<vec2> CLaserPrediction::PredictLaserBounce(float Angle)
{
    int ClientId = m_pClient->m_Snap.m_LocalClientId;
    float LaserReach = m_pClient->m_aTuning[g_Config.m_ClDummy].m_LaserReach;
    vec2 Direction = direction(Angle);
    vec2 Position = m_pClient->m_PredictedChar.m_Pos;
    std::vector<vec2> lines;

    // Store initial position
    lines.push_back(Position);

    // Create a temporary world for prediction
    CGameWorld TempWorld;
    TempWorld.m_pCollision = m_PredictionWorld.m_pCollision;
    TempWorld.m_WorldConfig = m_PredictionWorld.m_WorldConfig;
    TempWorld.m_GameTick = m_pClient->m_PredictedWorld.GameTick();
    TempWorld.m_pTuningList = m_pClient->m_aTuning;
    TempWorld.m_Teams = m_pClient->m_Teams;
    TempWorld.m_LocalClientId = ClientId;

    // Initialize teams
    TempWorld.NetObjBegin(m_pClient->m_Teams, ClientId);
    TempWorld.NetObjEnd();

    // Create laser entity for prediction
    CLaser *proj = new CLaser(&TempWorld, Position, Direction, LaserReach, ClientId, WEAPON_LASER);
    int initialTick = TempWorld.GameTick();
    vec2 prev_pos = Position;

    const int TICKS_PER_BOUNCE = m_pClient->m_aTuning[g_Config.m_ClDummy].m_LaserBounceDelay;
    const int MAX_BOUNCES = 64;
    
    for(int i = 0; i <= MAX_BOUNCES; i++)
    {
        TempWorld.m_GameTick = initialTick + i * TICKS_PER_BOUNCE;
        proj->Tick();
        vec2 cur_pos = proj->GetPos();

        if(cur_pos != prev_pos)
        {
            prev_pos = cur_pos;
            lines.push_back(cur_pos);
        }
    }

    delete proj;
    return lines;
}

bool CLaserPrediction::CheckLaserHit(const std::vector<vec2> &Path, int TargetID)
{
    if(Path.empty() || Path.size() < 2)
        return false;

    const CGameClient::CClientData &Target = m_pClient->m_aClients[TargetID];
    vec2 targetPos = Target.m_RenderPos;
    vec2 targetVel = Target.m_Predicted.m_Vel;
    
    const int TICKS_PER_BOUNCE = m_pClient->m_aTuning[g_Config.m_ClDummy].m_LaserBounceDelay;
    
    // Create temporary world and character core for prediction
    CWorldCore tempWorld;
    CCharacterCore tempCore;
    tempCore.Init(&tempWorld, m_pClient->Collision());
    
    // Get target character data
    const CNetObj_Character *pTargetChar = &m_pClient->m_Snap.m_aCharacters[TargetID].m_Cur;
    if(!pTargetChar)
        return false;
    
    tempCore.Read(pTargetChar);
    
    // Check each segment of the laser path
    for(size_t j = 1; j < Path.size(); j++)
    {
        vec2 Start = Path[j-1];
        vec2 End = Path[j];
        
        vec2 PredictedPos = targetPos;
        if(g_Config.m_ZrAimbotLaserPredict)
        {
            int ticksToPredict = (j-1) * TICKS_PER_BOUNCE / 50;
            
            // Reset core position for fresh prediction
            tempCore.Read(pTargetChar);
            
            // Predict position using character core
            for(int tick = 0; tick < ticksToPredict; tick++)
            {
                if(tempCore.m_IsInFreeze)
                    break;
                    
                tempCore.Tick(false);
                tempCore.Move();
                tempCore.Quantize();
            }
            
            PredictedPos = tempCore.m_Pos;
            
            // Not sure if this is needed, but it's here for now
            float PartialTick = float(ticksToPredict) / 50.0f;
            PredictedPos += tempCore.m_Vel * PartialTick;
        }
        
        // Check if laser segment intersects with predicted player position
        vec2 closestPoint;
        if(closest_point_on_line(Start, End, PredictedPos, closestPoint))
        {
            if(distance(PredictedPos, closestPoint) < CCharacterCore::PhysicalSize())
            {
                return true;
            }
        }
    }

    return false;
}

void CLaserPrediction::OnInit()
{
    m_PredictionWorld.m_pCollision = Collision();
    m_PredictionWorld.m_WorldConfig.m_IsDDRace = true;
    m_PredictionWorld.m_WorldConfig.m_PredictWeapons = true;
    m_PredictionWorld.m_pTuningList = m_pClient->m_aTuning;
    m_PredictionWorld.m_LocalClientId = m_pClient->m_Snap.m_LocalClientId;
}

void CLaserPrediction::ScanAngles(float StartAngle, float EndAngle, int Steps, int TargetID, 
    std::vector<std::pair<float, float>> &ValidAngles)
{
    float AngleStep = (EndAngle - StartAngle) / Steps;
    std::vector<std::pair<float, float>> LocalValidAngles;
    std::vector<PathVisualization> LocalPaths;

    for(int i = 0; i <= Steps; i++)
    {
        float CurrentAngle = StartAngle + i * AngleStep;
        auto Path = PredictLaserBounce(CurrentAngle);
        bool Valid = CheckLaserHit(Path, TargetID);

        // Convert Path to PredictionPoints
        std::vector<PredictionPoint> Points;
        for(size_t j = 0; j < Path.size(); j++)
        {
            Points.push_back({Path[j], j > 0 && j < Path.size()-1});
        }

        // Store path for visualization
        LocalPaths.push_back({Points, Valid});

        if(Valid)
        {
            LocalValidAngles.emplace_back(CurrentAngle, Path.size());
        }
    }

    // Thread safe update of visualization paths and valid angles
    std::lock_guard<std::mutex> Lock(m_AnglesMutex);
    if(!LocalPaths.empty())
    {
        m_VisualizePaths.insert(m_VisualizePaths.end(), LocalPaths.begin(), LocalPaths.end());
    }
    if(!LocalValidAngles.empty())
    {
        ValidAngles.insert(ValidAngles.end(), LocalValidAngles.begin(), LocalValidAngles.end());
    }
}

vec2 CLaserPrediction::PredictLaser(int TargetID, float CurrentAngle, float FOV)
{
    // Clear visualization paths before new prediction
    m_VisualizePaths.clear();

    // Convert FOV to radians
    float FovRadians = (FOV * pi) / 180.0f;
    float StartAngle = CurrentAngle - FovRadians / 2;
    float EndAngle = CurrentAngle + FovRadians / 2;

    // Number of threads to use
    const int NUM_THREADS = 4;
    const int TOTAL_STEPS = clamp(g_Config.m_ZrAimbotLaserAccuracy, 1, 200);
    const int STEPS_PER_THREAD = TOTAL_STEPS / NUM_THREADS;

    std::vector<std::pair<float, float>> ValidAngles;
    std::vector<std::shared_ptr<CScanJob>> Jobs;

    // Split the angle range into segments
    float AngleRange = EndAngle - StartAngle;
    float ThreadAngleStep = AngleRange / NUM_THREADS;

    // Create and queue jobs
    for(int i = 0; i < NUM_THREADS; i++)
    {
        float JobStartAngle = StartAngle + i * ThreadAngleStep;
        float JobEndAngle = JobStartAngle + ThreadAngleStep;

        auto pJob = std::make_shared<CScanJob>(
            JobStartAngle, JobEndAngle, 
            STEPS_PER_THREAD, TargetID, 
            &ValidAngles, this
        );
        
        Jobs.push_back(pJob);
        Kernel()->RequestInterface<IEngine>()->AddJob(std::shared_ptr<IJob>(pJob));
    }

    // Wait for all jobs to complete
    for(const auto &pJob : Jobs)
    {
        while(!pJob->Done())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Find the best angle based on bounce path selection
    if(!ValidAngles.empty())
    {
        float BestAngle = ValidAngles[0].first;
        float PathLength = ValidAngles[0].second;

        switch(g_Config.m_ZrAimbotLaserBouncePath)
        {
            case 0: // Closest path
            {
                float shortestPath = PathLength;
                for(const auto &pair : ValidAngles)
                {
                    if(pair.second < shortestPath)
                    {
                        shortestPath = pair.second;
                        BestAngle = pair.first;
                    }
                }
                break;
            }
            case 1: // Furthest path
            {
                float longestPath = PathLength;
                for(const auto &pair : ValidAngles)
                {
                    if(pair.second > longestPath)
                    {
                        longestPath = pair.second;
                        BestAngle = pair.first;
                    }
                }
                break;
            }
            case 2: // Random path
            {
                int randomIndex = rand() % ValidAngles.size();
                BestAngle = ValidAngles[randomIndex].first;
                break;
            }
        }

        m_LastPredictedPath = PredictLaserBounce(BestAngle);
        return direction(BestAngle) * 100.0f;
    }

    m_LastPredictedPath.clear();
    return vec2(0, 0);
}

void CLaserPrediction::OnReset()
{
    m_LastPredictedPath.clear();
    m_VisualizePaths.clear();
}

void CLaserPrediction::OnRender()
{
    // Only render if we have a local character and they're using laser and debug prediction is enabled
    if(!m_pClient->m_Snap.m_pLocalCharacter || 
       m_pClient->m_Snap.m_pLocalCharacter->m_Weapon != WEAPON_LASER ||
       !g_Config.m_ZrDebugPrediction)
    {
        std::lock_guard<std::mutex> Lock(m_AnglesMutex);
        m_VisualizePaths.clear();
        return;
    }

    std::vector<PathVisualization> PathsCopy;
    {
        std::lock_guard<std::mutex> Lock(m_AnglesMutex);
        if(m_VisualizePaths.empty())
            return;
        PathsCopy = m_VisualizePaths;
    }

    // Save screen mapping
    float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
    Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

    // Map screen to interface coordinates
    RenderTools()->MapScreenToInterface(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y);

    // Render all paths
    Graphics()->TextureClear();
    Graphics()->LinesBegin();
    
    for(const auto &Path : PathsCopy)
    {
        if(Path.m_Points.size() < 2)
            continue;

        ColorRGBA Color = Path.m_Valid ? 
            ColorRGBA(0.0f, 1.0f, 0.0f, 0.3f) :
            ColorRGBA(1.0f, 0.0f, 0.0f, 0.3f);

        const auto &Points = Path.m_Points;
        for(size_t i = 0; i < Points.size() - 1; i++)
        {
            Graphics()->SetColor(Color);
            IGraphics::CLineItem Line(
                Points[i].m_Pos.x, Points[i].m_Pos.y,
                Points[i+1].m_Pos.x, Points[i+1].m_Pos.y
            );
            Graphics()->LinesDraw(&Line, 1);
        }
    }
    
    Graphics()->LinesEnd();
    
    // Restore screen mapping
    Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}