#include "ff_pathfinder.h"
#include <game/client/gameclient.h>
#include <engine/shared/config.h>
#include <limits>
#include <game/mapitems.h>
#include <chrono>

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
    // First check map bounds
    if(x < 0 || x >= m_Width || y < 0 || y >= m_Height)
        return false;

    // Check if tile is marked as wall in our grid
    if(m_FlowField[y][x].isWall)
        return false;

    // Double check with collision
    vec2 pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
    int MapIndex = m_pClient->Collision()->GetMapIndex(pos);
    int TileIndex = m_pClient->Collision()->GetTileIndex(MapIndex);
    
    // Check for any solid/blocking tiles
    return !(
        TileIndex == TILE_DEATH || 
        TileIndex == TILE_SOLID ||
        TileIndex == TILE_NOHOOK ||
        TileIndex == TILE_STOP ||
        TileIndex == TILE_STOPS ||
        TileIndex == TILE_STOPA
    );
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
        
        // Check if neighbor is valid (not solid/wall)
        if(IsValidTile(newX, newY))
        {
            // For diagonal movement, check if both adjacent cells are valid
            if(dx[i] != 0 && dy[i] != 0)
            {
                if(IsValidTile(x + dx[i], y) && IsValidTile(x, y + dy[i]))
                {
                    // Also check the actual diagonal path
                    vec2 cornerPos((x + dx[i]) * 32.0f + 16.0f, (y + dy[i]) * 32.0f + 16.0f);
                    if(!m_pClient->Collision()->IntersectLine(pos, cornerPos, nullptr, nullptr))
                        neighbors.push_back(m_FlowField[newY][newX].pos);
                }
            }
            else
            {
                // For orthogonal movement, check direct line of sight
                vec2 neighborPos(newX * 32.0f + 16.0f, newY * 32.0f + 16.0f);
                if(!m_pClient->Collision()->IntersectLine(pos, neighborPos, nullptr, nullptr))
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

void CFlowFieldPathfinder::FloodFillFromFinish()
{
    // Initialize reachable area
    m_ReachableArea.clear();
    m_ReachableArea.resize(m_Height, std::vector<bool>(m_Width, false));

    // Queue for flood fill
    std::queue<vec2> queue;

    // Start from all finish tiles
    for(const auto& finish : m_FinishTiles)
    {
        int fx = static_cast<int>(finish.x/32.0f);
        int fy = static_cast<int>(finish.y/32.0f);
        if(!m_ReachableArea[fy][fx])
        {
            m_ReachableArea[fy][fx] = true;
            queue.push(finish);
        }
    }

    // Get player position in tile coordinates
    int playerX = static_cast<int>(m_StartPos.x/32.0f);
    int playerY = static_cast<int>(m_StartPos.y/32.0f);
    bool playerReachable = false;

    // Flood fill
    while(!queue.empty() && !playerReachable)
    {
        vec2 pos = queue.front();
        queue.pop();

        int x = static_cast<int>(pos.x/32.0f);
        int y = static_cast<int>(pos.y/32.0f);

        // Check if we reached player position
        if(x == playerX && y == playerY)
        {
            playerReachable = true;
            continue;
        }

        // Add valid neighbors to queue
        for(const auto& neighbor : GetValidNeighbors(pos))
        {
            int nx = static_cast<int>(neighbor.x/32.0f);
            int ny = static_cast<int>(neighbor.y/32.0f);

            if(!m_ReachableArea[ny][nx])
            {
                m_ReachableArea[ny][nx] = true;
                queue.push(neighbor);
            }
        }
    }

    // If player isn't reachable, clear everything
    if(!playerReachable)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "No valid path to finish!");
        m_ReachableArea.clear();
    }
}

void CFlowFieldPathfinder::GenerateFlowField()
{
    if(m_FinishTiles.empty())
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "GenerateFlowField: No finish tiles!");
        return;
    }

    // Get local player position
    m_StartPos = m_pClient->m_PredictedChar.m_Pos;

    // First do flood fill to find reachable area
    FloodFillFromFinish();
    
    if(m_ReachableArea.empty())
    {
        m_PathFound = false;
        return;
    }

    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();

    // Reset distances only for reachable tiles
    for(int y = 0; y < m_Height; y++)
    {
        for(int x = 0; x < m_Width; x++)
        {
            if(m_ReachableArea[y][x])
                m_FlowField[y][x].distance = std::numeric_limits<float>::infinity();
        }
    }

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
        int fx = static_cast<int>(finish.x / 32.0f);
        int fy = static_cast<int>(finish.y / 32.0f);
        m_FlowField[fy][fx].distance = 0;
        queue.push({0, finish});
    }

    // Dijkstra's algorithm (reverse flow field)
    while(!queue.empty())
    {
        auto current = queue.top();
        queue.pop();
        
        vec2 pos = current.second;
        float dist = current.first;
        
        int x = static_cast<int>(pos.x / 32.0f);
        int y = static_cast<int>(pos.y / 32.0f);
        
        // Skip if we found a better path already
        if(dist > m_FlowField[y][x].distance)
            continue;
            
        // Process neighbors
        for(const auto& neighbor : GetValidNeighbors(pos))
        {
            int nx = static_cast<int>(neighbor.x / 32.0f);
            int ny = static_cast<int>(neighbor.y / 32.0f);
            
            float newDist = dist + GetHeuristic(pos, neighbor);
            
            if(newDist < m_FlowField[ny][nx].distance && m_ReachableArea[ny][nx])
            {
                m_FlowField[ny][nx].distance = newDist;
                m_FlowField[ny][nx].direction = normalize(pos - neighbor); // Reverse direction
                queue.push({newDist, neighbor});
            }
        }
    }
    
    m_PathFound = true;
    
    // End timing and calculate duration
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> duration = endTime - startTime;
    m_LastPathfindingTime = duration.count();
    
    // Send completion message
    char aBuf[64];
    str_format(aBuf, sizeof(aBuf), "Pathfinding complete in %.3f ms | Success: %s", 
        m_LastPathfindingTime,
        m_PathFound ? "Yes" : "No"
    );
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", aBuf);
}

bool CFlowFieldPathfinder::FindPath()
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

    // Always update dimensions in case of map change
    m_Width = m_pLayers->GameLayer()->m_Width;
    m_Height = m_pLayers->GameLayer()->m_Height;

    // Reset and reinitialize everything
    m_PathFound = false;
    InitializeGrid();
    m_FinishTiles = FindTiles(TILE_FINISH);

    if(m_FinishTiles.empty())
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "No finish tiles found!");
        return false;
    }

    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "Starting pathfinding...");
    GenerateFlowField();
    
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
    if(!m_pClient->m_Snap.m_pGameInfoObj || !m_pLayers || !m_pLayers->GameLayer() || !m_pClient->m_Cheat.m_Active)
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
            if(!m_ReachableArea[y][x])
                continue;

            const Node& node = m_FlowField[y][x];
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

std::vector<vec2> CFlowFieldPathfinder::FindTiles(int TileType) const
{
    std::vector<vec2> tiles;

    // Get game layer data
    CTile *pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));
    if(!pTiles)
    {
        Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", "No tile data found!");
        return tiles;
    }

    // Verify dimensions
    int Width = m_pLayers->GameLayer()->m_Width;
    int Height = m_pLayers->GameLayer()->m_Height;
    
    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "Scanning for tile type 0x%x in %dx%d map", TileType, Width, Height);
    Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "pathfinder", aBuf);
    
    // Scan entire map for tiles
    for(int y = 0; y < Height; y++)
    {
        for(int x = 0; x < Width; x++)
        {
            int Index = y * Width + x;
            bool found = false;
            
            // I'm not sure why we check both layers, but it works so i'm not gonna change it
            // Check game layer tile
            if(pTiles[Index].m_Index == TileType)
                found = true;
                
            // Check front layer tile
            if(!found)
            {
                vec2 pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
                int MapIndex = m_pClient->Collision()->GetMapIndex(pos);
                if(m_pClient->Collision()->GetTileIndex(MapIndex) == TileType)
                {
                    found = true;
                }
            }

            if(found)
            {
                vec2 pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
                tiles.push_back(pos);
                str_format(aBuf, sizeof(aBuf), "Found tile at (%d,%d) [Index=%d]", x, y, Index);
                Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "pathfinder", aBuf);
            }
        }
    }

    str_format(aBuf, sizeof(aBuf), "Found total of %d tiles", (int)tiles.size());
    Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "pathfinder", aBuf);
    
    return tiles;
}