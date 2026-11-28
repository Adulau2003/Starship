#include "mouse_aim.h"

#include <libultraship/libultraship.h>
#include "libultraship/src/Context.h"
#include <algorithm>
#include <cmath>

extern "C" {
#include "sys.h"
#include <sf64context.h>
}

namespace {

constexpr float kTickSeconds = 1.0f / 30.0f; // game logic rate

float sTargetYaw = 0.0f;
float sTargetPitch = 0.0f;
float sYaw = 0.0f;
float sPitch = 0.0f;
float sAimWorldYaw = 0.0f; // all-range only: aim direction in world space (degrees)
bool sWasAllRange = false;
bool sCaptured = false;

bool Enabled() {
    return CVarGetInteger("gMouseAim", 0) != 0;
}

bool InFlight() {
    return gPlayer != nullptr && gGameState == GSTATE_PLAY &&
           (gLevelMode == LEVELMODE_ON_RAILS || gLevelMode == LEVELMODE_ALL_RANGE) && !gVersusMode &&
           gPlayer[0].form == FORM_ARWING && gPlayer[0].state == PLAYERSTATE_ACTIVE;
}

bool AllRange() {
    return gLevelMode == LEVELMODE_ALL_RANGE;
}

// wrap an angle in degrees to [-180, 180]
float Wrap180(float a) {
    while (a > 180.0f) {
        a -= 360.0f;
    }
    while (a < -180.0f) {
        a += 360.0f;
    }
    return a;
}

void SetCapture(bool capture) {
    if (capture == sCaptured) {
        return;
    }
    auto window = Ship::Context::GetInstance()->GetWindow();
    window->SetMouseCapture(capture);
    if (capture) {
        window->GetMouseDelta(); // discard whatever accumulated before capture
    }
    sCaptured = capture;
}

} // namespace

extern "C" void MouseAim_Update(void) {
    const bool flying = Enabled() && InFlight();
    auto window = Ship::Context::GetInstance()->GetWindow();
    const bool menuOpen = window->GetGui()->GetMenuOrMenubarVisible();

    SetCapture(flying && !menuOpen);

    if (!flying) {
        sTargetYaw = sTargetPitch = sYaw = sPitch = 0.0f;
        sWasAllRange = false;
        return;
    }

    // All-range: start aiming straight ahead whenever we (re)enter that mode.
    if (AllRange() && !sWasAllRange) {
        sAimWorldYaw = gPlayer[0].yRot_114;
        sTargetPitch = sPitch = gPlayer[0].rot.x;
    } else if (!AllRange() && sWasAllRange) {
        sTargetYaw = sTargetPitch = sYaw = sPitch = 0.0f;
    }
    sWasAllRange = AllRange();
    if (menuOpen) {
        return; // freeze aim while the menu is up
    }

    const Ship::Coords delta = window->GetMouseDelta(); // raw pixels moved since last tick, no OS acceleration
    const float sensX = CVarGetFloat("gMouseAimSensX", 0.15f);
    const float sensY = CVarGetFloat("gMouseAimSensY", 0.15f);
    const float invert = (CVarGetInteger("gInvertYAxis", 0) == 1) ? -1.0f : 1.0f;
    const float range = CVarGetFloat("gMouseAimRange", 40.0f);

    // Mouse right -> rot.y decreases, mouse up -> rot.x decreases (matches stick right / stick up).
    sTargetPitch += static_cast<float>(delta.y) * sensY * invert;
    sTargetPitch = std::clamp(sTargetPitch, -range, range);
    if (AllRange()) {
        // The ship turns toward the aim, so keep the aim in world space and express it relative to the heading.
        const float heading = gPlayer[0].yRot_114;
        sAimWorldYaw -= static_cast<float>(delta.x) * sensX;
        float rel = std::clamp(Wrap180(sAimWorldYaw - heading), -range, range);
        sAimWorldYaw = heading + rel; // the aim can drag the heading around once it hits the edge of the cone
        sTargetYaw = rel;
    } else {
        sTargetYaw -= static_cast<float>(delta.x) * sensX;
        sTargetYaw = std::clamp(sTargetYaw, -range, range);
    }

    // Optional frame-time-independent smoothing (time constant in ms, 0 = raw).
    const float tauMs = CVarGetFloat("gMouseAimSmoothing", 0.0f);
    if (tauMs > 0.5f) {
        const float alpha = 1.0f - std::exp(-kTickSeconds / (tauMs * 0.001f));
        sYaw += (sTargetYaw - sYaw) * alpha;
        sPitch += (sTargetPitch - sPitch) * alpha;
    } else {
        sYaw = sTargetYaw;
        sPitch = sTargetPitch;
    }
}

extern "C" int MouseAim_Active(void) {
    return Enabled() && InFlight() ? 1 : 0;
}

extern "C" void MouseAim_GetAngles(float* yaw, float* pitch) {
    *yaw = sYaw;
    *pitch = sPitch;
}

extern "C" float MouseAim_FollowStrength(void) {
    // All-range has no fixed path, so the ship has to turn toward the aim for mouse flight to work at all.
    if (AllRange()) {
        return std::clamp(CVarGetFloat("gMouseAimFollowAllRange", 1.0f), 0.0f, 1.0f);
    }
    return std::clamp(CVarGetFloat("gMouseAimFollow", 0.0f), 0.0f, 1.0f);
}
