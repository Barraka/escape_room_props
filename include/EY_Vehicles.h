#pragma once
#ifdef HAS_VEHICLES

void EY_Vehicles_Begin();
void EY_Vehicles_Activate();
bool EY_Vehicles_Tick();        // Returns true when all switches match the target combination
void EY_Vehicles_Reset();
void EY_Vehicles_ForceSolve();  // GM force solve
uint8_t EY_Vehicles_GetProgress();      // 0-100 (share of switches in correct position)
uint8_t EY_Vehicles_GetCorrectCount();  // 0-VEHICLE_COUNT

#ifdef VEHICLES_TEST_MODE
void EY_Vehicles_TestBegin();   // dev wiring test: print each switch position on change
void EY_Vehicles_TestTick();
#endif

#endif
