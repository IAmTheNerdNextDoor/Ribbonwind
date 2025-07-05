#ifndef HL2MP_WEAPON_TAU_CANNON_H
#define HL2MP_WEAPON_TAU_CANNON_H
#pragma once

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "beam_shared.h"

#define MAX_TAU_CHARGE_TIME 0.0f   // seconds to full charge

#ifdef CLIENT_DLL
    #define CWeaponTauCannon C_WeaponTauCannon
#endif

class CWeaponTauCannon : public CBaseHL2MPCombatWeapon
{
public:
    DECLARE_CLASS( CWeaponTauCannon, CBaseHL2MPCombatWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponTauCannon();

    virtual void    Precache() OVERRIDE;
    virtual void    PrimaryAttack() OVERRIDE;
    virtual void    SecondaryAttack() OVERRIDE;
    virtual void    ItemPostFrame() OVERRIDE;
    virtual bool    CanPrimaryAttack();

    void DrawBeam( const Vector &start, const Vector &end, float width );

#ifndef CLIENT_DLL
private:
    void    FirePulse();            // weak uncharged shot
    void    FireChargedPulse();     // full‑power blast
    void    ChargePulse();          // begin charging
    void    StopChargeSound();      // fade out

    float           m_flNextPrimaryAttack;   // cooldown timer
    float           m_flChargeStartTime;     // when charge began
    bool            m_bCharging;             // are we charging?
    CSoundPatch*    m_pChargeSound;          // charging sfx
    bool            m_bUnableToFire;         // blocked by animation/state

    int             m_iAmmoType;             // GaussEnergy
#endif
};
#endif // HL2MP_WEAPON_TAU_CANNON_H
