#ifndef GAME_CLIENT_COMPONENTS_CHEAT_FEATURES_MAPBOT_H
#define GAME_CLIENT_COMPONENTS_CHEAT_FEATURES_MAPBOT_H

#include <game/client/component.h>
#include <game/client/components/cheat/utils/ff_pathfinder.h>
#include <base/vmath.h>
#include <vector>
#include <game/client/render.h>
#include <engine/shared/jobs.h>
#include <engine/engine.h>
#include <mutex>

class CGameWorld;
class CCharacter;

class CMapBot : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnInit() override;
    virtual void OnReset() override;
    virtual void OnMapLoad() override;
    virtual void OnRender() override;
    
    void GeneratePath();
    bool IsGenerating() const { return m_IsGenerating; }
    bool HasPath() const { return m_PathFound; }
    
private:
    struct InputState {
        CNetObj_PlayerInput m_Input;
        vec2 m_MousePos;
        int m_Ticks;
        float m_PathScore;
        vec2 m_FinalPos;
    };

    CGameWorld* m_pGameWorld;
    bool m_IsGenerating;
    bool m_PathFound;
    std::vector<vec2> m_PathPoints;
    std::vector<vec2> m_CurrentPath;
    vec2 m_StartPos;
    vec2 m_FinishPos;
    
    // Game world simulation
    void InitGameWorld();
    void UpdateGameWorld();
    
    // Input generation
    std::vector<InputState> GenerateInputs(const vec2& pos, const vec2& vel);
    float ScorePosition(const vec2& pos, const vec2& vel);
    
    // State management
    InputState m_BestInput;
    bool m_HasBestInput;
    int64_t m_LastUpdateTime;

    void RenderPath();
    void UpdatePath();
    bool IsValidTile(int x, int y) const;

    class CInputJob : public IJob
    {
    public:
        CInputJob(int StartDir, int EndDir, const vec2& pos, const vec2& vel, 
                std::vector<InputState>* pInputs, CMapBot* pMapBot) :
            m_StartDir(StartDir),
            m_EndDir(EndDir),
            m_Pos(pos),
            m_Vel(vel),
            m_pInputs(pInputs),
            m_pMapBot(pMapBot),
            m_Completed(false)
        {}

        void Run() override
        {
            m_pMapBot->GenerateInputsThread(m_StartDir, m_EndDir, m_Pos, m_Vel, *m_pInputs);
            m_Completed = true;
        }

        bool Done() const { return m_Completed; }

    private:
        std::vector<vec2> m_CurrentPath;
        int m_StartDir;
        int m_EndDir;
        vec2 m_Pos;
        vec2 m_Vel;
        std::vector<InputState>* m_pInputs;
        CMapBot* m_pMapBot;
        bool m_Completed;
    };

    void GenerateInputsThread(int StartDir, int EndDir, const vec2& pos, const vec2& vel, std::vector<InputState>& inputs);
    std::mutex m_InputMutex;
};

#endif
