#ifndef GAME_CLIENT_COMPONENTS_CHEAT_VISUALS_ANIMATIONS_H
#define GAME_CLIENT_COMPONENTS_CHEAT_VISUALS_ANIMATIONS_H

#include <game/client/component.h>
#include <base/vmath.h>
#include <vector>

class CAnimations : public CComponent
{
public:
    enum EAnimation
    {
        ANIM_CIRCLE = 0,
        ANIM_EXPANDING_CIRCLE,
        ANIM_WAVE,
        ANIM_EIGHT,
        ANIM_TRIANGLE,
        ANIM_DISK_X,
        ANIM_LEMNISCATE,
        ANIM_SPIRAL,
        ANIM_COPY_DISK,
        ANIM_SQUARE,
        ANIM_TOP_BOTTOM,
        ANIM_X,
        ANIM_RIGHT_LEFT,
        ANIM_RANDOM_TB,
        ANIM_ATOM,
        ANIM_VERTICAL_SECTOR,
        ANIM_HORIZONTAL_SECTOR,
        ANIM_RHOMBUS,
        ANIM_HEART,
        NUM_ANIMATIONS
    };

    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnRender() override;
    virtual void OnReset() override;

private:
    struct Point {
        vec2 m_Pos;
        float m_Angle;
    };

    std::vector<Point> m_Points;
    float m_Iteration = 0.0f;

    // Animation calculation functions
    vec2 CalcCircle(int index, int totalPoints, float radius, float speed);
    vec2 CalcExpandingCircle(int index, int totalPoints, float radius, float speed);
    vec2 CalcWave(int index, int totalPoints, float radius, float speed);
    vec2 CalcEight(int index, int totalPoints, float radius, float speed);
    vec2 CalcTriangle(int index, int totalPoints, float radius, float speed);
    vec2 CalcDiskX(int index, int totalPoints, float radius, float speed);
    vec2 CalcLemniscate(int index, int totalPoints, float radius, float speed);
    vec2 CalcSpiral(int index, int totalPoints, float radius, float speed);
    vec2 CalcCopyDisk(int index, int totalPoints, float radius, float speed);
    vec2 CalcSquare(int index, int totalPoints, float radius, float speed);
    vec2 CalcTopBottom(int index, int totalPoints, float radius, float speed);
    vec2 CalcX(int index, int totalPoints, float radius, float speed);
    vec2 CalcRightLeft(int index, int totalPoints, float radius, float speed);
    vec2 CalcRandomTB(int index, int totalPoints, float radius, float speed);
    vec2 CalcAtom(int index, int totalPoints, float radius, float speed);
    vec2 CalcVerticalSector(int index, int totalPoints, float radius, float speed);
    vec2 CalcHorizontalSector(int index, int totalPoints, float radius, float speed);
    vec2 CalcRhombus(int index, int totalPoints, float radius, float speed);
    vec2 CalcHeart(int index, int totalPoints, float radius, float speed);

    void InitializePoints();
    void UpdatePoints();
    void RenderGrenades();
};

#endif
