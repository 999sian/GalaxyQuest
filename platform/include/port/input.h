// Controller state handed from the XR/input layer to the Wii remote emulation.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PortPadState {
    int connected;         // a Wii remote + Nunchuk is present
    uint32_t buttons;      // WPAD_BUTTON_* bits (remote and Nunchuk C/Z)
    float stickX, stickY;  // Nunchuk stick, -1..1 (right / up positive)
    float accX, accY, accZ;        // remote accelerometer in g (remote frame)
    float nunAccX, nunAccY, nunAccZ;  // Nunchuk accelerometer in g
    int pointerValid;      // pointer is on screen
    float pointerX, pointerY;  // -1..1 screen space (right / down positive, like KPAD)
    float pointerDist;     // metres from the "sensor bar"
} PortPadState;

// Called by the input layer (any thread).
void port_input_set(int chan, const PortPadState* state);

// Rumble requested by the game (0/1) for a channel; read by the input layer.
int port_input_rumble(int chan);

// Remote tilt (see port_input_use_tilt in port/compat.h): nonzero while the
// game reads the accelerometer for steering; *neutralPitchDeg is the pitch of
// the Wii remote that a level-held controller stands for.
int port_input_tilt_active(float* neutralPitchDeg);

#ifdef __cplusplus
}
#endif
