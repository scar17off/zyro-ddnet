#ifndef GAME_CLIENT_COMPONENTS_CHEAT_UTILS_FF_PATHFINDER_H
#define GAME_CLIENT_COMPONENTS_CHEAT_UTILS_FF_PATHFINDER_H

#include <game/client/component.h>
#include <base/vmath.h>
#include <base/color.h>
#include <vector>
#include <queue>
#include <map>

class CFlowFieldPathfinder : public CComponent
{
public:
    virtual int Sizeof() const override { return sizeof(*this); }
    virtual void OnInit() override;
    virtual void OnReset() override;
    virtual void OnMapLoad() override;
    virtual void OnRender() override;

    void GenerateFlowField();
    bool FindPath();
    
    struct Node {
        vec2 pos;
        float distance;
        vec2 direction;
        bool isWall;
        
        Node() : pos(0,0), distance(std::numeric_limits<float>::infinity()), 
                direction(0,0), isWall(false) {}
    };

    // Inline methods for mapbot
    bool HasPath() const { return m_PathFound; }
    std::vector<vec2> GetPath() const { 
        std::vector<vec2> path;
        // Just return the flow field directions
        for(int y = 0; y < m_Height; y++) {
            for(int x = 0; x < m_Width; x++) {
                if(!m_FlowField[y][x].isWall)
                    path.push_back(m_FlowField[y][x].direction);
            }
        }
        return path;
    }
    vec2 GetStartPos() const { return m_StartPos; }
    vec2 GetFinishPos() const { return !m_FinishTiles.empty() ? m_FinishTiles[0] : vec2(0,0); }
    vec2 GetFlowDirection(int x, int y) const { 
        if(IsValidTile(x, y))
            return -m_FlowField[y][x].direction;
        return vec2(0,0);
    }

private:
    std::vector<vec2> m_FinishTiles;
    std::vector<std::vector<Node>> m_FlowField;
    vec2 m_StartPos;
    int m_Width;
    int m_Height;
    bool m_PathFound;
    CLayers *m_pLayers;

    std::vector<std::vector<bool>> m_ReachableArea;
    float m_LastPathfindingTime;
    void FloodFillFromFinish();
    std::vector<vec2> FindTiles(int TileType) const;
    void InitializeGrid();
    bool IsValidTile(int x, int y) const;
    std::vector<vec2> GetValidNeighbors(const vec2& pos) const;
    float GetHeuristic(const vec2& a, const vec2& b) const;
    void RenderArrow(vec2 pos, vec2 dir, float size, ColorRGBA color);
};

#endif
