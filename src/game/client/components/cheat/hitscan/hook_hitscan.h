#ifndef GAME_CLIENT_COMPONENTS_CHEAT_HITSCAN_HOOK_HITSCAN_H
#define GAME_CLIENT_COMPONENTS_CHEAT_HITSCAN_HOOK_HITSCAN_H

#include <base/vmath.h>
#include <game/client/component.h>
#include <engine/shared/jobs.h>
#include <vector>
#include <mutex>
#include <memory>

class CHookHitscan : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }

    vec2 EdgeScan(int ClientId);

private:
    class CScanJob : public IJob
    {
    public:
        CScanJob(float StartAngle, float EndAngle, int Steps, int ClientId, 
                std::vector<vec2> *pValidPoints, CHookHitscan *pHookScan) :
            m_StartAngle(StartAngle),
            m_EndAngle(EndAngle),
            m_Steps(Steps),
            m_ClientId(ClientId),
            m_pValidPoints(pValidPoints),
            m_pHookScan(pHookScan),
            m_Completed(false)
        {}

        void Run() override
        {
            m_pHookScan->ScanAngles(m_StartAngle, m_EndAngle, m_Steps, m_ClientId, *m_pValidPoints);
            m_Completed = true;
        }

        bool Done() const { return m_Completed; }

    private:
        float m_StartAngle;
        float m_EndAngle;
        int m_Steps;
        int m_ClientId;
        std::vector<vec2> *m_pValidPoints;
        CHookHitscan *m_pHookScan;
        bool m_Completed;
    };

    void ScanAngles(float StartAngle, float EndAngle, int Steps, int ClientId, std::vector<vec2> &ValidPoints);
    bool HitScanHook(vec2 initPos, vec2 targetPos, vec2 scanDir);
    bool IntersectCharacter(vec2 hookPos, vec2 targetPos, vec2 &newPos);

    std::mutex m_ScanMutex;
};

#endif