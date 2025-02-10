#ifndef GAME_CLIENT_COMPONENTS_CHEAT_PREDICTION_LASER_PREDICTION_H
#define GAME_CLIENT_COMPONENTS_CHEAT_PREDICTION_LASER_PREDICTION_H

#include <base/vmath.h>
#include <game/client/component.h>
#include <game/client/prediction/gameworld.h>
#include <engine/shared/jobs.h>
#include <vector>
#include <mutex>
#include <memory>

class CLaserPrediction : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnInit() override;
    virtual void OnReset() override;

    std::vector<vec2> PredictLaserBounce(float Angle);
    bool CheckLaserHit(const std::vector<vec2> &Path, int TargetID);
    vec2 PredictLaser(int TargetID, float CurrentAngle, float FOV);

private:
    class CScanJob : public IJob
    {
    public:
        CScanJob(float StartAngle, float EndAngle, int Steps, int TargetID, 
                std::vector<std::pair<float, float>> *pValidAngles, CLaserPrediction *pLaserPred) :
            m_StartAngle(StartAngle),
            m_EndAngle(EndAngle),
            m_Steps(Steps),
            m_TargetID(TargetID),
            m_pValidAngles(pValidAngles),
            m_pLaserPred(pLaserPred),
            m_Completed(false)
        {}

        void Run() override
        {
            m_pLaserPred->ScanAngles(m_StartAngle, m_EndAngle, m_Steps, m_TargetID, *m_pValidAngles);
            m_Completed = true;
        }

        bool Done() const { return m_Completed; }

    private:
        float m_StartAngle;
        float m_EndAngle;
        int m_Steps;
        int m_TargetID;
        std::vector<std::pair<float, float>> *m_pValidAngles;
        CLaserPrediction *m_pLaserPred;
        bool m_Completed;
    };

    void ScanAngles(float StartAngle, float EndAngle, int Steps, int TargetID, std::vector<std::pair<float, float>> &ValidAngles);

    CGameWorld m_PredictionWorld;
    std::vector<vec2> m_LastPredictedPath;
    std::mutex m_AnglesMutex;
};

#endif 