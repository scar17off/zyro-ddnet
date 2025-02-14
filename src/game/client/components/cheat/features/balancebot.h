#ifndef GAME_CLIENT_COMPONENTS_CHEAT_FEATURES_BALANCEBOT_H
#define GAME_CLIENT_COMPONENTS_CHEAT_FEATURES_BALANCEBOT_H

#include <game/client/component.h>
#include <base/vmath.h>
#include <engine/console.h>
#include <optional>

class CBalanceBot : public CComponent
{
    friend class CArrayList;

public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnConsoleInit() override;
    void Balance();

    struct TargetInfo {
        int id;
        vec2 pos;
        float distance;

        TargetInfo(int id, const vec2& pos, float distance) : 
            id(id), pos(pos), distance(distance) {}
    };
    
private:
    bool IsValidVerticalPosition(const vec2& targetPos, const vec2& localPos) const;
    bool CanReachTarget(const vec2& localPos, const vec2& targetPos) const;
    std::optional<CBalanceBot::TargetInfo> FindTarget(const vec2& localPos, float maxDistance) const;
    static void ConToggle(IConsole::IResult *pResult, void *pUserData);
    
    bool m_Active = false;
};

#endif
