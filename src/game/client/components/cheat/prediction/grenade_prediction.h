#ifndef GAME_CLIENT_COMPONENTS_CHEAT_PREDICTION_GRENADE_PREDICTION_H
#define GAME_CLIENT_COMPONENTS_CHEAT_PREDICTION_GRENADE_PREDICTION_H

#include <base/vmath.h>
#include <game/client/component.h>
#include <game/client/prediction/gameworld.h>
#include <engine/shared/jobs.h>
#include <vector>
#include <mutex>
#include <memory>

class CGrenadePrediction : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnReset() override;
    virtual void OnRender() override;

    struct PredictionPoint {
        vec2 m_Pos;
        bool m_Exploded;
    };

    vec2 PredictGrenade(int TargetID, float CurrentAngle, float FOV);
    std::vector<PredictionPoint> PredictGrenadePath(vec2 Pos, vec2 Direction);

private:
    class CScanJob : public IJob
    {
    public:
        CScanJob(float StartAngle, float EndAngle, int Steps, int TargetID, 
                std::vector<std::pair<float, float>> *pValidAngles, CGrenadePrediction *pGrenadePred) :
            m_StartAngle(StartAngle),
            m_EndAngle(EndAngle),
            m_Steps(Steps),
            m_TargetID(TargetID),
            m_pValidAngles(pValidAngles),
            m_pGrenadePred(pGrenadePred),
            m_Completed(false)
        {}

        void Run() override
        {
            m_pGrenadePred->ScanAngles(m_StartAngle, m_EndAngle, m_Steps, m_TargetID, *m_pValidAngles);
            m_Completed = true;
        }

        bool Done() const { return m_Completed; }

    private:
        float m_StartAngle;
        float m_EndAngle;
        int m_Steps;
        int m_TargetID;
        std::vector<std::pair<float, float>> *m_pValidAngles;
        CGrenadePrediction *m_pGrenadePred;
        bool m_Completed;
    };

    void ScanAngles(float StartAngle, float EndAngle, int Steps, int TargetID, std::vector<std::pair<float, float>> &ValidAngles);

    std::vector<PredictionPoint> m_LastPrediction;
    std::mutex m_AnglesMutex;

    struct PathVisualization {
        std::vector<PredictionPoint> m_Points;
        bool m_Valid;
    };
    std::vector<PathVisualization> m_VisualizePaths;
};

#endif
