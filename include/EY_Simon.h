#pragma once
#ifdef HAS_SIMON

void EY_Simon_Begin();
void EY_Simon_Activate();
bool EY_Simon_Tick();       // Returns true when all buttons locked (solved)
void EY_Simon_Reset();
void EY_Simon_ForceSolve(); // GM force solve — all LEDs on
uint8_t EY_Simon_GetProgress();     // 0-100
uint8_t EY_Simon_GetLockedCount();  // 0-SIMON_COUNT

#ifdef SIMON_TEST_MODE
void EY_Simon_TestBegin();  // dev wiring test: all LEDs solid on
void EY_Simon_TestTick();   // dev wiring test: press a lit button → it blinks 3x → solid
#endif

#endif
