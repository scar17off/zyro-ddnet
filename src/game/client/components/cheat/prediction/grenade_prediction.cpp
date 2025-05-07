#include "grenade_prediction.h"
#include <game/client/gameclient.h>
#include <game/collision.h>
#include <game/client/prediction/entities/projectile.h>
#include <engine/engine.h>
#include <chrono>
#include <thread>

static vec2 closest_point_on_line(vec2 line_start, vec2 line_end, vec2 point)
{
    vec2 line_vec = line_end - line_start;
    float len_sq = line_vec.x * line_vec.x + line_vec.y * line_vec.y;
    if(len_sq == 0.0f) return line_start;
    
    float t = maximum(0.0f, minimum(1.0f, dot(point - line_start, line_vec) / len_sq));
    return line_start + line_vec * t;
}

void CGrenadePrediction::OnReset()
{
    m_LastPrediction.clear();
    m_VisualizePaths.clear();
}

vec2 CGrenadePrediction::PredictGrenade(int TargetID, float CurrentAngle, float FOV)
{
    // Clear visualization paths before new prediction
    m_VisualizePaths.clear();

    // DONT HATE ME FOR THIS SHITCODE
    // TODO: Make this smarter and more efficient
    
    // Normalize the current angle to be between -pi and pi
    while(CurrentAngle < -pi) CurrentAngle += 2*pi;
    while(CurrentAngle > pi) CurrentAngle -= 2*pi;

    // Convert FOV to radians and calculate angle range
    float FovRadians = (FOV * pi) / 180.0f;
    float StartAngle = CurrentAngle - FovRadians/2;
    float EndAngle = CurrentAngle + FovRadians/2;

    // Ensure both start and end angles are properly normalized
    while(StartAngle < -pi) StartAngle += 2*pi;
    while(StartAngle > pi) StartAngle -= 2*pi;
    while(EndAngle < -pi) EndAngle += 2*pi;
    while(EndAngle > pi) EndAngle -= 2*pi;

    // Handle the case where the FOV range crosses the -pi/pi boundary
    if(EndAngle < StartAngle)
        EndAngle += 2*pi;

    // Setup multithreaded scan
    const int NUM_THREADS = 4;
    const int TOTAL_STEPS = clamp(g_Config.m_ZrAimbotGrenadeAccuracy, 1, 200);
    const int STEPS_PER_THREAD = TOTAL_STEPS / NUM_THREADS;

    std::vector<std::pair<float, float>> ValidAngles;
    std::vector<std::shared_ptr<CScanJob>> Jobs;

    // Split the FOV range into segments for each thread
    float ThreadAngleStep = (EndAngle - StartAngle) / NUM_THREADS;

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

    // Find the best angle based on path selection mode
    if(!ValidAngles.empty())
    {
        float BestAngle;
        
        switch(g_Config.m_ZrAimbotGrenadePath)
        {
            case 1: // Furthest path
            {
                float LongestPath = 0.0f;
                for(const auto &pair : ValidAngles)
                {
                    if(pair.second > LongestPath)
                    {
                        LongestPath = pair.second;
                        BestAngle = pair.first;
                    }
                }
                break;
            }
            
            case 2: // Random path
            {
                int RandomIndex = rand() % ValidAngles.size();
                BestAngle = ValidAngles[RandomIndex].first;
                break;
            }
            
            default: // Closest path (0)
            {
                float ShortestPath = ValidAngles[0].second;
                BestAngle = ValidAngles[0].first;
                
                for(const auto &pair : ValidAngles)
                {
                    if(pair.second < ShortestPath)
                    {
                        ShortestPath = pair.second;
                        BestAngle = pair.first;
                    }
                }
                break;
            }
        }

        return direction(BestAngle) * 100.0f;
    }

    return vec2(0, 0);
}

void CGrenadePrediction::ScanAngles(float StartAngle, float EndAngle, int Steps, int TargetID, 
    std::vector<std::pair<float, float>> &ValidAngles)
{
    if(!m_pClient->m_Snap.m_aCharacters[TargetID].m_Active)
        return;

    vec2 MyPos = m_pClient->m_LocalCharacterPos;
    vec2 TargetPos = m_pClient->m_aClients[TargetID].m_RenderPos;
    
    float AngleRange = EndAngle - StartAngle;
    float AngleStep = AngleRange / (Steps > 0 ? Steps : 1);

    std::vector<std::pair<float, float>> DirectHitAngles;
    std::vector<std::pair<float, float>> SplashHitAngles;
    std::vector<PathVisualization> LocalPaths;

    for(int i = 0; i <= Steps; i++)
    {
        float CurrentAngle = StartAngle + i * AngleStep;
        
        while(CurrentAngle < -pi) CurrentAngle += 2*pi;
        while(CurrentAngle > pi) CurrentAngle -= 2*pi;

        vec2 Direction = direction(CurrentAngle);
        
        auto Points = PredictGrenadePath(MyPos, Direction);
        bool Valid = false;
        
        // Initialize based on length mode
        float ShortestDirectDist = g_Config.m_ZrAimbotGrenadeLength ? 0.0f : 1000000.0f;
        float ShortestSplashDist = g_Config.m_ZrAimbotGrenadeLength ? 0.0f : 1000000.0f;

        // Check each segment of the path
        for(size_t j = 0; j < Points.size() - 1; j++)
        {
            vec2 Start = Points[j].m_Pos;
            vec2 End = Points[j+1].m_Pos;
            vec2 ClosestPoint;
            
            if(closest_point_on_line(Start, End, TargetPos, ClosestPoint))
            {
                float Dist = distance(ClosestPoint, TargetPos);
                float PathLength = distance(ClosestPoint, MyPos);

                // Check for direct hit (28.0f radius)
                if(Dist < 28.0f)
                {
                    Valid = true;
                    if(g_Config.m_ZrAimbotGrenadeLength)
                    {
                        // Longest path
                        if(PathLength > ShortestDirectDist)
                        {
                            ShortestDirectDist = PathLength;
                            DirectHitAngles.push_back({CurrentAngle, PathLength});
                        }
                    }
                    else
                    {
                        // Shortest path
                        if(PathLength < ShortestDirectDist)
                        {
                            ShortestDirectDist = PathLength;
                            DirectHitAngles.push_back({CurrentAngle, PathLength});
                        }
                    }
                }
                
                // Check for splash explosion if enabled
                if(g_Config.m_ZrAimbotGrenadeSplash && Dist < 64.0f)
                {
                    Valid = true;
                    if(g_Config.m_ZrAimbotGrenadeLength)
                    {
                        if(PathLength > ShortestSplashDist)
                        {
                            ShortestSplashDist = PathLength;
                            SplashHitAngles.push_back({CurrentAngle, PathLength});
                        }
                    }
                    else
                    {
                        if(PathLength < ShortestSplashDist)
                        {
                            ShortestSplashDist = PathLength;
                            SplashHitAngles.push_back({CurrentAngle, PathLength});
                        }
                    }
                }
            }
        }

        // Store path for visualization
        LocalPaths.push_back({Points, Valid});
    }

    // Update ValidAngles with results based on preference
    std::lock_guard<std::mutex> Lock(m_AnglesMutex);
    
    if(g_Config.m_ZrAimbotGrenadeSplash)
    {
        // Always use direct hits if available
        if(!DirectHitAngles.empty())
        {
            ValidAngles.insert(ValidAngles.end(), DirectHitAngles.begin(), DirectHitAngles.end());
        }
        // Only use splash hits if no direct hits are available
        else if(!SplashHitAngles.empty())
        {
            ValidAngles.insert(ValidAngles.end(), SplashHitAngles.begin(), SplashHitAngles.end());
        }
    }
    else
    {
        // Only use direct hits if splash is disabled
        if(!DirectHitAngles.empty())
        {
            ValidAngles.insert(ValidAngles.end(), DirectHitAngles.begin(), DirectHitAngles.end());
        }
    }

    // Update visualization paths
    if(!LocalPaths.empty())
    {
        m_VisualizePaths.insert(m_VisualizePaths.end(), LocalPaths.begin(), LocalPaths.end());
    }
}

std::vector<CGrenadePrediction::PredictionPoint> CGrenadePrediction::PredictGrenadePath(
    vec2 Pos, vec2 Direction)
{
    std::vector<PredictionPoint> Points;
    
    static bool FirstPrediction = true;
    char aBuf[256];
    
    if(FirstPrediction)
    {
        str_format(aBuf, sizeof(aBuf), "Starting prediction from pos: %.2f, %.2f with dir: %.2f, %.2f",
            Pos.x, Pos.y, Direction.x, Direction.y);
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "grenade", aBuf);
    }
    
    // Create local game world instance
    CGameWorld PredictionWorld;
    PredictionWorld.m_pCollision = m_pClient->Collision();
    PredictionWorld.m_WorldConfig = m_pClient->m_PredictedWorld.m_WorldConfig;
    PredictionWorld.m_GameTick = m_pClient->m_PredictedWorld.GameTick();
    PredictionWorld.m_pTuningList = m_pClient->m_aTuning;
    
    // Normalize direction
    Direction = normalize(Direction);
    
    // Calculate proper starting position
    vec2 ProjStartPos = Pos + Direction * 28.0f * 0.75f; // 28.0f is the default proximity radius
    
    // Calculate lifetime
    int Lifetime = (int)(m_pClient->Client()->GameTickSpeed() * m_pClient->m_aTuning[g_Config.m_ClDummy].m_GrenadeLifetime);

    CProjectile *pProj = new CProjectile(
        &PredictionWorld,
        WEAPON_GRENADE,
        m_pClient->m_Snap.m_LocalClientId,
        ProjStartPos,
        Direction,
        Lifetime,
        false,
        true,
        SOUND_GRENADE_EXPLODE
    );

    if(!pProj)
        return Points;

    int InitialTick = PredictionWorld.GameTick();
    vec2 PrevPos = Pos;
    Points.push_back({Pos, false});

    for(int i = 1; i <= Lifetime && pProj; i++)
    {
        PredictionWorld.m_GameTick = InitialTick + i;
        pProj->Tick();
        
        float Time = (i / (float)m_pClient->Client()->GameTickSpeed());
        vec2 CurPos = pProj->GetPos(Time);

        if(FirstPrediction)
        {
            str_format(aBuf, sizeof(aBuf), "Tick %d - Pos: %.2f, %.2f",
                i, CurPos.x, CurPos.y);
            Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "grenade", aBuf);
        }

        vec2 CollisionPos = CurPos;
        if(m_pClient->Collision()->IntersectLine(PrevPos, CurPos, &CollisionPos, nullptr))
        {
            if(CollisionPos != PrevPos)
            {
                Points.push_back({CollisionPos, true});
                if(FirstPrediction)
                {
                    str_format(aBuf, sizeof(aBuf), "Collision at: %.2f, %.2f", CollisionPos.x, CollisionPos.y);
                    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "grenade", aBuf);
                }
            }
            break;
        }

        if(CurPos != PrevPos)
        {
            Points.push_back({CurPos, pProj->m_MarkedForDestroy});
            PrevPos = CurPos;
        }

        if(pProj->m_MarkedForDestroy)
        {
            if(FirstPrediction)
                Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "grenade", "Projectile destroyed");
            break;
        }
    }

    FirstPrediction = false;
    delete pProj;
    return Points;
}

void CGrenadePrediction::OnRender()
{
    // Only render if we have a local character and they're using grenade and debug prediction is enabled
    if(!m_pClient->m_Snap.m_pLocalCharacter || 
       m_pClient->m_Snap.m_pLocalCharacter->m_Weapon != WEAPON_GRENADE ||
       !g_Config.m_ZrDebugPrediction || !m_pClient->m_Cheat.m_Active)
    {
        m_VisualizePaths.clear();
        return;
    }

    if(m_VisualizePaths.empty())
        return;

    // Save screen mapping
    float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
    Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

    // Map screen to interface coordinates
    RenderTools()->MapScreenToGroup(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y, Layers()->GameGroup(), m_pClient->m_Camera.m_Zoom);

    // Render all paths
    Graphics()->TextureClear();
    Graphics()->LinesBegin();
    
    for(const auto &Path : m_VisualizePaths)
    {
        if(Path.m_Points.size() < 2)
            continue;

        ColorRGBA Color = Path.m_Valid ? 
            ColorRGBA(0.0f, 1.0f, 0.0f, 0.3f) : // Valid paths in green
            ColorRGBA(1.0f, 0.0f, 0.0f, 0.3f);  // Invalid paths in red

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
