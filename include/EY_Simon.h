#pragma once
#ifdef HAS_SIMON

void EY_Simon_Begin();
void EY_Simon_Activate();
bool EY_Simon_Tick();       // Returns true when all buttons locked (solved)
void EY_Simon_Reset();
void EY_Simon_ForceSolve(); // GM force solve — all LEDs on
uint8_t EY_Simon_GetProgress();     // 0-100
uint8_t EY_Simon_GetLockedCount();  // 0-SIMON_COUNT

#endif
