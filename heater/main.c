#include <stdio.h>
#include <math.h>

// =======================
// Heater Control Loop
// =======================
//
// Write a function that computes heater power based on:
//   - current temperature
//   - target temperature
//
// Output:
//   - heater power as a percentage from 0.0 to 100.0
//
// Requirements:
//   - use proportional control
//   - add a deadband around the target temperature
//   - clamp output to [0, 100]
//
// Behavior:
//   - if current temp is below target, heater power should increase
//   - if current temp is above target, heater power should decrease
//   - inside the deadband, output should be 0
//
// Suggested interface:
//
// float heater_control(float current_temp,
//                      float target_temp,
//                      float kp,
//                      float deadband);
//
// Notes:
//   - error = target_temp - current_temp
//   - a simple proportional controller is enough for now
//   - do not let output go below 0 or above 100
//
// Bonus later:
//   - add integral and derivative terms
//   - keep controller state between calls

float heater_control(float current_temp,
                     float target_temp,
                     float kp,
                     float deadband)
{
    double min_deadband = target_temp - deadband;
    double max_deadband = target_temp + deadband;
    if (current_temp >= min_deadband && current_temp <= max_deadband) return 0.0f;

    double error = target_temp - current_temp;
    if (error < 0.0) return 0.0f;

    double output = error * kp;

    return (output > 100.0) ? 100.0f : (float)output;
}

int feq(float a, float b)
{
    return fabsf(a - b) < 1e-4f;
}

void run_test(const char* name,
              float current_temp,
              float target_temp,
              float kp,
              float deadband,
              float expected)
{
    float out = heater_control(current_temp, target_temp, kp, deadband);

    printf("%s:\n", name);
    printf("  current=%.2f target=%.2f kp=%.2f deadband=%.2f\n",
           current_temp, target_temp, kp, deadband);
    printf("  expected=%.2f got=%.2f %s\n\n",
           expected, out, feq(out, expected) ? "PASS" : "FAIL");
}

int main()
{
    run_test("below target, moderate heat", 20.0f, 25.0f, 10.0f, 0.5f, 50.0f);
    run_test("at target",                    25.0f, 25.0f, 10.0f, 0.5f, 0.0f);
    run_test("inside deadband low side",    24.7f, 25.0f, 10.0f, 0.5f, 0.0f);
    run_test("inside deadband high side",   25.3f, 25.0f, 10.0f, 0.5f, 0.0f);
    run_test("above target",                30.0f, 25.0f, 10.0f, 0.5f, 0.0f);
    run_test("clamp high",                  10.0f, 25.0f, 10.0f, 0.5f, 100.0f);
    run_test("small positive error",        24.0f, 25.0f, 15.0f, 0.2f, 15.0f);

    return 0;
}
