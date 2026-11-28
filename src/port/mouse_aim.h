#pragma once

// Mouse look for Arwing rail stages (Starship port addition).
// Aim angles use the same convention/units as Player::rot.y / Player::rot.x (degrees).

#ifdef __cplusplus
extern "C" {
#endif

// Call once per game tick (from Controller_UpdateInput).
void MouseAim_Update(void);
// Non-zero while mouse look is enabled AND the player is flying an Arwing on rails.
int MouseAim_Active(void);
// Absolute aim angles (rot.y / rot.x convention). Only meaningful when MouseAim_Active().
void MouseAim_GetAngles(float* yaw, float* pitch);
// 0..1: how much the ship steers toward the aim point (0 = keyboard/stick flies, mouse only aims).
float MouseAim_FollowStrength(void);
// Non-zero when the L button should act as a wing tilt toward the side the mouse is aiming at.
int MouseAim_AimTiltEnabled(void);
// Landmaster: drive the cannon turret. bodyYaw = tank's player->rot.y. Writes the turret yaw (unk_180) and
// pitch (unk_17C), which the game already uses for both the reticle and the shot direction.
void MouseAim_TankTurret(float bodyYaw, float* turretYaw, float* turretPitch);

#ifdef __cplusplus
}
#endif
