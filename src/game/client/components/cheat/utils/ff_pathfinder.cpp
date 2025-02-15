#include "ff_pathfinder.h"
#include <game/client/gameclient.h>
#include <engine/shared/config.h>
#include <limits>
#include <game/mapitems.h>

// today i discovered that the pathfinder name KRX client is using is called Flow Field Pathfinder...
// so i decided to make it, and it works pretty well, but it's not perfect.

void CFlowFieldPathfinder::OnInit()
{
    m_PathFound = false;
}

void CFlowFieldPathfinder::OnReset()
{
    m_FinishTiles.clear();
    m_FlowField.clear();
    m_PathFound = false;
}

void CFlowFieldPathfinder::OnMapLoad()
{
    OnReset();
}

void CFlowFieldPathfinder::FindFinishTiles()
{
    m_FinishTiles.clear();
    int finishCount = 0;

    // Get game layer data directly
    CTile *pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));
    if(!pTiles)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "No tile data found in FindFinishTiles!");
        return;
    }

    // Verify dimensions
    int Width = m_pLayers->GameLayer()->m_Width;
    int Height = m_pLayers->GameLayer()->m_Height;
    
    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "Scanning for TILE_FINISH (0x%x) in %dx%d map", TILE_FINISH, Width, Height);
    Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "pathfinder", aBuf);
    
    // Scan entire map for finish tiles
    for(int y = 0; y < Height; y++)
    {
        for(int x = 0; x < Width; x++)
        {
            int Index = y * Width + x;
            
            // Check both tile layers (I don't know why it's like this, but it's like this)
            bool isFinish = false;
            
            // Check game layer tile
            if(pTiles[Index].m_Index == TILE_FINISH)
                isFinish = true;
                
            // Check front layer tile
            if(!isFinish)
            {
                vec2 pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
                int MapIndex = m_pClient->Collision()->GetMapIndex(pos);
                if(m_pClient->Collision()->GetTileIndex(MapIndex) == TILE_FINISH ||
                   m_pClient->Collision()->GetTileIndex(MapIndex) == TILE_FINISH)
                {
                    isFinish = true;
                }
            }

            if(isFinish)
            {
                vec2 pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
                m_FinishTiles.push_back(pos);
                finishCount++;
                str_format(aBuf, sizeof(aBuf), "Found finish tile at (%d,%d) [Index=%d]", x, y, Index);
                Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", aBuf);
            }
        }
    }

    str_format(aBuf, sizeof(aBuf), "Found total of %d finish tiles", finishCount);
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", aBuf);
}

void CFlowFieldPathfinder::InitializeGrid()
{
    m_FlowField.clear();
    m_FlowField.resize(m_Height);
    
    for(int y = 0; y < m_Height; y++)
    {
        m_FlowField[y].resize(m_Width);
        for(int x = 0; x < m_Width; x++)
        {
            vec2 pos(x*32.0f + 16.0f, y*32.0f + 16.0f);
            m_FlowField[y][x].pos = pos;
            
            // Get tile info
            int MapIndex = m_pClient->Collision()->GetMapIndex(pos);
            int TileIndex = m_pClient->Collision()->GetTileIndex(MapIndex);
            
            // Check for all blocking tiles
            m_FlowField[y][x].isWall = (
                TileIndex == TILE_DEATH || 
                TileIndex == TILE_FREEZE || 
                TileIndex == TILE_DFREEZE || 
                TileIndex == TILE_LFREEZE ||
                TileIndex == TILE_NOLASER || 
                TileIndex == TILE_SOLID ||
                TileIndex == TILE_NOHOOK ||
                TileIndex == TILE_STOP ||
                TileIndex == TILE_STOPS ||
                TileIndex == TILE_STOPA
            );
        }
    }
}

bool CFlowFieldPathfinder::IsValidTile(int x, int y) const
{
    return x >= 0 && x < m_Width && y >= 0 && y < m_Height && !m_FlowField[y][x].isWall;
}

std::vector<vec2> CFlowFieldPathfinder::GetValidNeighbors(const vec2& pos) const
{
    std::vector<vec2> neighbors;
    int x = static_cast<int>(pos.x/32.0f);
    int y = static_cast<int>(pos.y/32.0f);
    
    // Check 8 directions
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    for(int i = 0; i < 8; i++)
    {
        int newX = x + dx[i];
        int newY = y + dy[i];
        
        if(IsValidTile(newX, newY))
        {
            // For diagonal movement, check if both adjacent cells are clear
            if(dx[i] != 0 && dy[i] != 0)
            {
                if(IsValidTile(x + dx[i], y) && IsValidTile(x, y + dy[i]))
                {
                    neighbors.push_back(m_FlowField[newY][newX].pos);
                }
            }
            else
            {
                neighbors.push_back(m_FlowField[newY][newX].pos);
            }
        }
    }
    
    return neighbors;
}

float CFlowFieldPathfinder::GetHeuristic(const vec2& a, const vec2& b) const
{
    return length(b-a);
}

void CFlowFieldPathfinder::GenerateFlowField()
{
    if(m_FinishTiles.empty())
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "GenerateFlowField: No finish tiles!");
        return;
    }

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "Generating flow field from %d finish tiles", (int)m_FinishTiles.size());
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", aBuf);
    
    auto startTime = time_get();
    
    // Get local player position
    m_StartPos = m_pClient->m_PredictedChar.m_Pos;
    
    // Reset distances
    for(auto& row : m_FlowField)
        for(auto& node : row)
            node.distance = std::numeric_limits<float>::infinity();

    // Custom comparator for the priority queue
    struct CompareDistance {
        bool operator()(const std::pair<float, vec2>& a, const std::pair<float, vec2>& b) const {
            return a.first > b.first;
        }
    };

    // Priority queue with custom comparator
    std::priority_queue<
        std::pair<float, vec2>,
        std::vector<std::pair<float, vec2>>,
        CompareDistance
    > queue;

    // Start from all finish tiles
    for(const auto& finish : m_FinishTiles)
    {
        int fx = static_cast<int>(finish.x/32.0f);
        int fy = static_cast<int>(finish.y/32.0f);
        m_FlowField[fy][fx].distance = 0;
        queue.push({0, finish});
    }

    // Dijkstra's algorithm (this is the only algorithm i know)
    while(!queue.empty())
    {
        auto current = queue.top();
        queue.pop();
        
        vec2 pos = current.second;
        float dist = current.first;
        
        int x = static_cast<int>(pos.x/32.0f);
        int y = static_cast<int>(pos.y/32.0f);
        
        // Skip if we found a better path already
        if(dist > m_FlowField[y][x].distance)
            continue;
            
        // Process neighbors
        for(const auto& neighbor : GetValidNeighbors(pos))
        {
            int nx = static_cast<int>(neighbor.x/32.0f);
            int ny = static_cast<int>(neighbor.y/32.0f);
            
            float newDist = dist + GetHeuristic(pos, neighbor);
            
            if(newDist < m_FlowField[ny][nx].distance)
            {
                m_FlowField[ny][nx].distance = newDist;
                // here if we want to reverse the direction because we want to move towards the finish tile
                // m_FlowField[ny][nx].direction = normalize(neighbor - pos); // from player to finish tile
                m_FlowField[ny][nx].direction = normalize(pos - neighbor); // from finish tile to player
                queue.push({newDist, neighbor});
            }
        }
    }
    
    m_PathFound = true;
    
    // Calculate time taken
    auto endTime = time_get();
    float timeTaken = (float)(endTime - startTime) / time_freq();
    
    // Send completion message
    str_format(aBuf, sizeof(aBuf), "Pathfinding complete in %.2f ms | Success: %s", 
        timeTaken * 1000.0f,
        m_PathFound ? "Yes" : "No"
    );
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", aBuf);
}

bool CFlowFieldPathfinder::FindPath()
{
    // If we haven't tried to find finish tiles yet, do it now
    if(m_FinishTiles.empty())
    {
        // Check if game is ready
        if(!m_pClient->m_Snap.m_pGameInfoObj)
        {
            Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "Game not ready yet!");
            return false;
        }

        m_pLayers = m_pClient->Layers();
        if(!m_pLayers || !m_pLayers->GameLayer() || !m_pLayers->Map())
        {
            Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "Map data not available!");
            return false;
        }

        m_Width = m_pLayers->GameLayer()->m_Width;
        m_Height = m_pLayers->GameLayer()->m_Height;

        // Initialize grid and find finish tiles
        InitializeGrid();
        FindFinishTiles();
    }

    if(!m_PathFound)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "Starting pathfinding...");
        if(m_FinishTiles.empty())
        {
            Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "No finish tiles found!");
            return false;
        }
        
        GenerateFlowField();
    }
    return m_PathFound;
}

void CFlowFieldPathfinder::RenderArrow(vec2 pos, vec2 dir, float size, ColorRGBA color)
{
    // Skip if direction is zero
    if(length(dir) < 0.001f)
        return;

    dir = normalize(dir);
    vec2 perp(-dir.y, dir.x);

    // Calculate arrow points with larger proportions
    vec2 tip = pos + dir * size;
    vec2 base = pos - dir * size;
    
    // Make arrow head larger relative to shaft
    float wingSize = size * 0.8f;
    vec2 wingBase = tip - dir * wingSize;
    vec2 wing1 = wingBase + perp * wingSize * 0.5f;
    vec2 wing2 = wingBase - perp * wingSize * 0.5f;

    // Draw arrow
    Graphics()->TextureClear();
    Graphics()->LinesBegin();
    Graphics()->SetColor(color);

    // Draw thicker arrow shaft
    IGraphics::CLineItem LineItem(base.x, base.y, tip.x, tip.y);
    Graphics()->LinesDraw(&LineItem, 1);

    // Draw arrow head
    IGraphics::CLineItem HeadLines[] = {
        IGraphics::CLineItem(tip.x, tip.y, wing1.x, wing1.y),
        IGraphics::CLineItem(tip.x, tip.y, wing2.x, wing2.y)
    };
    Graphics()->LinesDraw(HeadLines, 2);
    Graphics()->LinesEnd();
}

void CFlowFieldPathfinder::OnRender()
{
    // Only render if we have valid map data
    if(!m_pClient->m_Snap.m_pGameInfoObj || !m_pLayers || !m_pLayers->GameLayer())
        return;

    if(!m_PathFound)
        return;

    // The code below is stolen from render_map.cpp
    // Save current screen mapping
    float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
    Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

    // Calculate visible area in tiles
    int StartY = (int)(ScreenY0 / 32.0f) - 1;
    int StartX = (int)(ScreenX0 / 32.0f) - 1;
    int EndY = (int)(ScreenY1 / 32.0f) + 1;
    int EndX = (int)(ScreenX1 / 32.0f) + 1;

    // Clamp to map boundaries
    StartX = clamp(StartX, 0, m_Width - 1);
    StartY = clamp(StartY, 0, m_Height - 1);
    EndX = clamp(EndX, 0, m_Width);
    EndY = clamp(EndY, 0, m_Height);

    // Map screen coordinates
    Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);

    // Calculate arrow size based on tile size
    float TileSize = 32.0f;
    float ArrowSize = TileSize * 0.4f; // 40% of tile size

    // Render arrows for visible tiles
    for(int y = StartY; y < EndY; y++)
    {
        for(int x = StartX; x < EndX; x++)
        {
            const Node& node = m_FlowField[y][x];
            
            // Skip walls and nodes with no direction
            if(node.isWall || length(node.direction) < 0.001f)
                continue;

            // Calculate world position (center of tile)
            float WorldX = x * TileSize + TileSize/2;
            float WorldY = y * TileSize + TileSize/2;
            
            // Color based on distance (somehow it works, or maybe it doesn't)
            float alpha = 0.3f;
            if(node.distance < std::numeric_limits<float>::infinity())
            {
                float maxDist = m_Width * m_Height * TileSize;
                alpha = 0.3f + 0.4f * (1.0f - std::min(node.distance / maxDist, 1.0f));
            }

            ColorRGBA color(0.0f, 0.0f, 0.0f, alpha);
            RenderArrow(vec2(WorldX, WorldY), node.direction, ArrowSize, color);
        }
    }

    // Restore original screen mapping
    Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
}
