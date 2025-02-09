#include "hook_prediction.h"
#include "game/client/gameclient.h"

bool CHookPrediction::PredictHook(vec2 &myPos, vec2 myVel, vec2 &targetPos, vec2 targetVel) 
{
    /*
    Player (P) -----> Moving at myVel
           \
            \  Hook flying at hookSpeed
             \
              \
               \
                Enemy (E) -----> Moving at targetVel
    
    Need to solve:
    - Initial positions: P and E
    - Initial velocities: myVel and targetVel  
    - Hook speed: hookSpeed
    - Time factors: ping + hook duration
    
    Steps:
    1. Calculate relative position (delta) and velocity (deltaVel)
    2. Solve quadratic equation for intercept time:
       at² + bt + c = 0
       where:
       a = (relative velocity)² - hook_speed²
       b = 2 * (relative position · relative velocity) 
       c = (relative position)²
    3. Add ping and hook duration to final time
    4. Project enemy position forward by total time
    */

    const vec2 delta = targetPos - myPos;
    const vec2 deltaVel = targetVel - myVel;

    const float hookSpeed = m_pClient->m_aTuning->m_HookFireSpeed;
    const float hookReleaseTime = m_pClient->m_aTuning->m_HookDuration;
    
    // Quadratic equation coefficients
    const float a = dot(deltaVel, deltaVel) - powf(hookSpeed, 2);
    const float b = 2.f * dot(deltaVel, delta);
    const float c = dot(delta, delta);

    const float sol = powf(b, 2) - 4.f * a * c;
    if(sol > 0.f)
    {
        const float qTime = (-sqrtf(sol) - b) / (2.f * a);
        const float time = abs(qTime) + GetPing() + hookReleaseTime;
        
        targetPos += targetVel * time;
        return true;
    }
    return false;
}

float CHookPrediction::GetPing() const
{
    const auto realPing = static_cast<float>(m_pClient->Client()->GetPredictionTime());
    return realPing / 100.f;
}