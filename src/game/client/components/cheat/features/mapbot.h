#ifndef GAME_CLIENT_COMPONENTS_CHEAT_FEATURES_MAPBOT_H
#define GAME_CLIENT_COMPONENTS_CHEAT_FEATURES_MAPBOT_H

#include <engine/shared/jobs.h>
#include <game/client/component.h>
#include <base/vmath.h>
#include <vector>
#include <mutex>

// Forward declarations to fix C2061
class CGameWorld;
class CCharacter;
struct CNetObj_PlayerInput;

class CMapBot : public CComponent
{
public:
    virtual int Sizeof() const { return sizeof(*this); }
    virtual void OnInit();
    virtual void OnReset();
    virtual void OnMapLoad();
    virtual void OnRender();

    struct PredictionPoint
    {
        vec2 m_Pos;
        bool m_IsFrozen;
        PredictionPoint() = default;
        PredictionPoint(const vec2 &Pos, bool IsFrozen)
            : m_Pos(Pos), m_IsFrozen(IsFrozen) {}
    };

    void GeneratePath();
    void PredictCharacterMovement(
        vec2 pos, vec2 vel,
        int dir, int jump, int hook, float angle,
        int PredictionSteps,
        std::vector<PredictionPoint>& points);
    bool IsGenerating() const { return m_IsGenerating; }
    bool HasPath() const { return m_PathFound; }

private:
    struct MovementPath {
        std::vector<vec2> m_Positions;
        vec2 m_Velocity;
        bool m_IsHook;
        bool m_IsJump;
        int m_Direction;
        bool m_IsSelected;

        // Constructor
        MovementPath(vec2 velocity = vec2(0, 0), bool isHook = false, bool isJump = false, int direction = 0)
            : m_Velocity(velocity), m_IsHook(isHook), m_IsJump(isJump), m_Direction(direction), m_IsSelected(false) {}
    };
    const CMapBot::MovementPath &GetSelectedPath() const;

    class CScanJob : public IJob
    {
    public:
        CScanJob(
            vec2 pos, vec2 vel,
            const std::vector<int>& directions,
            const std::vector<int>& jumps,
            const std::vector<int>& hookStates,
            const std::vector<float>& angles,
            int PredictionSteps,
            size_t startIdx, size_t endIdx,
            std::vector<MovementPath>* pOutPaths,
            CMapBot* pMapBot)
            : m_Pos(pos), m_Vel(vel),
              m_Directions(directions), m_Jumps(jumps), m_HookStates(hookStates), m_Angles(angles),
              m_PredictionSteps(PredictionSteps),
              m_StartIdx(startIdx), m_EndIdx(endIdx),
              m_pOutPaths(pOutPaths), m_pMapBot(pMapBot), m_Completed(false)
        {}

        void Run() override
        {
            size_t totalComb = m_Directions.size() * m_Jumps.size() * m_HookStates.size() * m_Angles.size();
            for(size_t idx = m_StartIdx; idx < m_EndIdx && idx < totalComb; ++idx)
            {
                size_t a = idx % m_Angles.size();
                size_t h = (idx / m_Angles.size()) % m_HookStates.size();
                size_t j = (idx / (m_Angles.size() * m_HookStates.size())) % m_Jumps.size();
                size_t d = (idx / (m_Angles.size() * m_HookStates.size() * m_Jumps.size())) % m_Directions.size();

                int dir = m_Directions[d];
                int jump = m_Jumps[j];
                int hook = m_HookStates[h];
                float angle = m_Angles[a];

                std::vector<PredictionPoint> points;
                m_pMapBot->PredictCharacterMovement(m_Pos, m_Vel, dir, jump, hook, angle, m_PredictionSteps, points);

                if(points.size() > 1)
                {
                    MovementPath path;
                    path.m_Direction = dir;
                    path.m_IsJump = jump > 0;
                    path.m_IsHook = hook > 0;
                    for(const auto& pt : points)
                        path.m_Positions.push_back(pt.m_Pos);
                    if(path.m_Positions.size() > 1)
                        path.m_Velocity = path.m_Positions.back() - path.m_Positions[path.m_Positions.size() - 2];
                    else
                        path.m_Velocity = vec2(0, 0);

                    // Thread-safe append
                    {
                        static std::mutex s_Mutex;
                        std::lock_guard<std::mutex> lock(s_Mutex);
                        m_pOutPaths->push_back(path);
                    }
                }
            }
            m_Completed = true;
        }

        bool Done() const { return m_Completed; }

    private:
        vec2 m_Pos, m_Vel;
        std::vector<int> m_Directions, m_Jumps, m_HookStates;
        std::vector<float> m_Angles;
        int m_PredictionSteps;
        size_t m_StartIdx, m_EndIdx;
        std::vector<MovementPath>* m_pOutPaths;
        CMapBot* m_pMapBot;
        bool m_Completed;
    };

    bool m_IsGenerating;
    bool m_PathFound;
    std::vector<MovementPath> m_Paths;

    void RenderPaths();
    std::vector<MovementPath> PredictMovement(vec2 pos, vec2 vel);
    bool IsValidPosition(vec2 pos) const;
};

#endif
