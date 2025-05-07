#include "hook_hitscan.h"
#include "game/client/gameclient.h"
#include <engine/engine.h>
#include <thread>
#include <chrono>

void CHookHitscan::ScanAngles(float StartAngle, float EndAngle, int Steps, int ClientId, std::vector<vec2> &ValidPoints)
{
    vec2 myPos = m_pClient->m_PredictedChar.m_Pos;
    vec2 targetPos = m_pClient->m_aClients[ClientId].m_Predicted.m_Pos;
    float AngleStep = (EndAngle - StartAngle) / Steps;

    std::vector<vec2> LocalValidPoints;
    std::vector<HookPathVisualization> LocalPaths;

    for(int i = 0; i <= Steps; i++)
    {
        float CurrentAngle = StartAngle + i * AngleStep;
        vec2 Direction = direction(CurrentAngle);
        vec2 HookDir = Direction * m_pClient->m_aTuning->m_HookLength;
        
        // Store hook path for visualization
        std::vector<HookPoint> Points;
        Points.push_back({myPos, false}); // Starting point
        
        bool Valid = HitScanHook(myPos, targetPos, HookDir);
        Points.push_back({myPos + HookDir, Valid}); // End point
        
        LocalPaths.push_back({Points, Valid});
        
        if(Valid)
        {
            std::lock_guard<std::mutex> Lock(m_ScanMutex);
            ValidPoints.push_back(HookDir);
        }
    }

    // Update visualization paths
    std::lock_guard<std::mutex> Lock(m_VisualizeMutex);
    if(!LocalPaths.empty())
    {
        m_VisualizePaths.insert(m_VisualizePaths.end(), LocalPaths.begin(), LocalPaths.end());
    }
}

void CHookHitscan::OnRender()
{
    // Only render if we have a local character and debug prediction is enabled
    if(!m_pClient->m_Snap.m_pLocalCharacter || !g_Config.m_ZrDebugPrediction || !m_pClient->m_Cheat.m_Active)
    {
        std::lock_guard<std::mutex> Lock(m_VisualizeMutex);
        m_VisualizePaths.clear();
        return;
    }

    std::vector<HookPathVisualization> PathsCopy;
    {
        std::lock_guard<std::mutex> Lock(m_VisualizeMutex);
        if(m_VisualizePaths.empty())
            return;
        PathsCopy = m_VisualizePaths;
    }

    // Save screen mapping
    float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
    Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

    // Map screen to interface coordinates
    RenderTools()->MapScreenToGroup(m_pClient->m_Camera.m_Center.x, m_pClient->m_Camera.m_Center.y, Layers()->GameGroup(), m_pClient->m_Camera.m_Zoom);

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
            vec2 Start = Points[i].m_Pos;
            vec2 End = Points[i+1].m_Pos;
            
            // Check for wall collision
            vec2 CollisionPoint;
            vec2 BeforeCollision;
            if(Collision()->IntersectLine(Start, End, &CollisionPoint, &BeforeCollision))
            {
                // Draw line only until collision point
                Graphics()->SetColor(Color);
                IGraphics::CLineItem Line(
                    Start.x, Start.y,
                    CollisionPoint.x, CollisionPoint.y
                );
                Graphics()->LinesDraw(&Line, 1);
                break; // Stop drawing this path after hitting a wall
            }
            else
            {
                // No collision, draw full line segment
                Graphics()->SetColor(Color);
                IGraphics::CLineItem Line(
                    Start.x, Start.y,
                    End.x, End.y
                );
                Graphics()->LinesDraw(&Line, 1);
            }
        }
    }
    
    Graphics()->LinesEnd();
    
    // Restore screen mapping
    Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}

void CHookHitscan::OnReset()
{
    m_VisualizePaths.clear();
}

vec2 CHookHitscan::EdgeScan(int ClientId)
{
    // Clear visualization paths before new scan
    m_VisualizePaths.clear();
    
    // Try direct hook first
    vec2 myPos = m_pClient->m_PredictedChar.m_Pos;
    vec2 targetPos = m_pClient->m_aClients[ClientId].m_Predicted.m_Pos;
    vec2 directHook = targetPos - myPos;
    
    if(HitScanHook(myPos, targetPos, directHook))
        return directHook;

    // Get current aim direction
    vec2 CurrentAim = normalize(vec2(
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetX,
        m_pClient->m_Controls.m_aInputData[g_Config.m_ClDummy].m_TargetY
    ));
    
    // Convert FOV from degrees to radians
    float FovRadians = (g_Config.m_ZrAimbotFoV * pi) / 180.0f;
    float CurrentAngle = angle(CurrentAim);
    float StartAngle = CurrentAngle - FovRadians / 2;
    float EndAngle = CurrentAngle + FovRadians / 2;

    // Setup multithreaded scan
    const int NUM_THREADS = 4;
    const int TOTAL_STEPS = clamp(g_Config.m_ZrAimbotHookAccuracy, 1, 200);
    const int STEPS_PER_THREAD = TOTAL_STEPS / NUM_THREADS;

    std::vector<vec2> ValidPoints;
    std::vector<std::shared_ptr<CScanJob>> Jobs;

    // Split the FOV range into segments
    float ThreadAngleStep = (EndAngle - StartAngle) / NUM_THREADS;

    // Create and queue jobs
    for(int i = 0; i < NUM_THREADS; i++)
    {
        float JobStartAngle = StartAngle + i * ThreadAngleStep;
        float JobEndAngle = JobStartAngle + ThreadAngleStep;

        auto pJob = std::make_shared<CScanJob>(
            JobStartAngle, JobEndAngle, 
            STEPS_PER_THREAD, ClientId, 
            &ValidPoints, this
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

    // Find the best hook point (closest to target)
    if(!ValidPoints.empty())
    {
        vec2 BestPoint = ValidPoints[0];
        float BestDist = distance(targetPos, myPos + BestPoint);

        for(const vec2 &Point : ValidPoints)
        {
            float Dist = distance(targetPos, myPos + Point);
            if(Dist < BestDist)
            {
                BestDist = Dist;
                BestPoint = Point;
            }
        }

        return BestPoint;
    }

    return vec2(0, 0);
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