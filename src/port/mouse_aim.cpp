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
bool sCaptured = false;

bool Enabled() {
    return CVarGetInteger("gMouseAim", 0) != 0;
}

bool InFlight() {
    return gPlayer != nullptr && gGameState == GSTATE_PLAY && gLevelMode == LEVELMODE_ON_RAILS && !gVersusMode &&
           gPlayer[0].form == FORM_ARWING && gPlayer[0].state == PLAYERSTATE_ACTIVE;
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
        return;
    }
    if (menuOpen) {
        return; // freeze aim while the menu is up
    }

    const Ship::Coords delta = window->GetMouseDelta(); // raw pixels moved since last tick, no OS acceleration
    const float sensX = CVarGetFloat("gMouseAimSensX", 0.15f);
    const float sensY = CVarGetFloat("gMouseAimSensY", 0.15f);
    const float invert = (CVarGetInteger("gInvertYAxis", 0) == 1) ? -1.0f : 1.0f;
    const float range = CVarGetFloat("gMouseAimRange", 40.0f);

    // Mouse right -> rot.y decreases, mouse up -> rot.x decreases (matches stick right / stick up).
    sTargetYaw -= static_cast<float>(delta.x) * sensX;
    sTargetPitch += static_cast<float>(delta.y) * sensY * invert;
    sTargetYaw = std::clamp(sTargetYaw, -range, range);
    sTargetPitch = std::clamp(sTargetPitch, -range, range);

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
    return std::clamp(CVarGetFloat("gMouseAimFollow", 0.0f), 0.0f, 1.0f);
}
