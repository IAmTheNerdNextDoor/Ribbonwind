#include "cbase.h"
#include "weapon_taucannon.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "beam_shared.h"

#ifndef CLIENT_DLL
    #include "soundenvelope.h"
    #include "te_effect_dispatch.h"
    #include "IEffects.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponTauCannon, DT_WeaponTauCannon )

BEGIN_NETWORK_TABLE( CWeaponTauCannon, DT_WeaponTauCannon )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponTauCannon )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_taucannon, CWeaponTauCannon );
PRECACHE_WEAPON_REGISTER( weapon_taucannon );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponTauCannon::CWeaponTauCannon()
{
    m_flNextPrimaryAttack = 0.0f;
#ifndef CLIENT_DLL
    m_bCharging         = false;
    m_pChargeSound      = nullptr;
    m_bUnableToFire     = false;
    m_iAmmoType         = GetAmmoDef()->Index( "GaussEnergy" );
#endif
}

//-----------------------------------------------------------------------------
// Precache models & sounds
//-----------------------------------------------------------------------------
void CWeaponTauCannon::Precache()
{
    BaseClass::Precache();
#ifndef CLIENT_DLL
    PrecacheModel( "models/weapons/v_irifle.mdl" );
    PrecacheModel( "models/weapons/w_irifle.mdl" );
#endif
    PrecacheModel( "sprites/laserbeam.vmt" );

    PrecacheScriptSound( "Weapon_IonCannon.Single" );
    PrecacheScriptSound( "PropJeep.FireChargedCannon" );
    PrecacheScriptSound( "Jeep.GaussCharge" );
}

//-----------------------------------------------------------------------------
// Always allow firing (ignore clip/ammo count)
//-----------------------------------------------------------------------------
bool CWeaponTauCannon::CanPrimaryAttack()
{
#ifndef CLIENT_DLL
    return ( gpGlobals->curtime >= m_flNextPrimaryAttack && !m_bUnableToFire );
#else
    return true;
#endif
}

//-----------------------------------------------------------------------------
// Primary: charge → fire charged blast when ready
//-----------------------------------------------------------------------------
void CWeaponTauCannon::PrimaryAttack()
{
#ifndef CLIENT_DLL
    if ( !CanPrimaryAttack() )
        return;

    // base cooldown
    m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;

    if ( m_bCharging &&
         ( gpGlobals->curtime - m_flChargeStartTime >= MAX_TAU_CHARGE_TIME ) )
    {
        FireChargedPulse();
        m_bCharging = false;
        StopChargeSound();
        SetWeaponIdleTime( gpGlobals->curtime + 2.0f );
        m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
    }
    else if ( !m_bCharging )
    {
        ChargePulse();
    }
#endif
}

//-----------------------------------------------------------------------------
// Secondary: immediate weak pulse
//-----------------------------------------------------------------------------
void CWeaponTauCannon::SecondaryAttack()
{
#ifndef CLIENT_DLL
    if ( !CanPrimaryAttack() )
        return;

    // small pulse on right‑click
    FirePulse();
    m_flNextPrimaryAttack = gpGlobals->curtime + 0.3f;
    SetWeaponIdleTime( gpGlobals->curtime + 1.0f );
#endif
}

//-----------------------------------------------------------------------------
// Handle charging sound & auto‑fire on full charge
//-----------------------------------------------------------------------------
void CWeaponTauCannon::ItemPostFrame()
{
    BaseClass::ItemPostFrame();

#ifndef CLIENT_DLL
    if ( m_bCharging )
    {
        float t = ( gpGlobals->curtime - m_flChargeStartTime ) / MAX_TAU_CHARGE_TIME;
        if ( t >= 1.0f )
        {
            PrimaryAttack();
        }
        else if ( m_pChargeSound )
        {
            CSoundEnvelopeController::GetController().SoundChangePitch(
                m_pChargeSound, 100 + t * 150, 0.1f );
        }
    }
#endif
}

#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Beam tracer helper
//-----------------------------------------------------------------------------
void CWeaponTauCannon::DrawBeam( const Vector &start, const Vector &end, float width )
{
    CEffectData data;
    data.m_vStart   = start;
    data.m_vOrigin  = end;
    data.m_flScale  = width;
    data.m_fFlags   = 0;
    DispatchEffect( "GaussTracer", data );
}

//-----------------------------------------------------------------------------
// Begin charging sfx
//-----------------------------------------------------------------------------
void CWeaponTauCannon::ChargePulse()
{
    m_flChargeStartTime = gpGlobals->curtime;
    m_bCharging         = true;

    CPASAttenuationFilter filter( this );
    m_pChargeSound = CSoundEnvelopeController::GetController().SoundCreate(
        filter, entindex(), CHAN_STATIC, "Jeep.GaussCharge", ATTN_NORM );
    CSoundEnvelopeController::GetController().Play( m_pChargeSound, 1.0f, 50 );
}

//-----------------------------------------------------------------------------
// Fade out the charging sfx
//-----------------------------------------------------------------------------
void CWeaponTauCannon::StopChargeSound()
{
    if ( m_pChargeSound )
    {
        CSoundEnvelopeController::GetController().SoundFadeOut( m_pChargeSound, 0.1f );
        m_pChargeSound = nullptr;
    }
}

//-----------------------------------------------------------------------------
// Uncharged pulse: light damage + thin beam
//-----------------------------------------------------------------------------
void CWeaponTauCannon::FirePulse()
{
    CBasePlayer* pPlayer = ToBasePlayer( GetOwner() );
    if ( !pPlayer ) return;

    Vector src;
    QAngle ang;
    src = pPlayer->Weapon_ShootPosition();
    ang = pPlayer->EyeAngles();
    Vector dir;
    AngleVectors( ang, &dir );

    trace_t tr;
    UTIL_TraceLine( src, src + dir * 8192, MASK_SHOT, this,
                    COLLISION_GROUP_NONE, &tr );

    ClearMultiDamage();

    const float dmg = 20.0f;
    if ( tr.m_pEnt && !tr.DidHitWorld() )
    {
        CTakeDamageInfo info( this, pPlayer, dmg, DMG_SHOCK );
        CalculateBulletDamageForce( &info, m_iAmmoType, dir, tr.endpos, 1.0f );
        tr.m_pEnt->DispatchTraceAttack( info, dir, &tr );
    }
    else if ( tr.DidHitWorld() && !( tr.surface.flags & SURF_SKY ) )
    {
        UTIL_ImpactTrace( &tr, m_iAmmoType, "ImpactJeep" );
        UTIL_DecalTrace( &tr, "RedGlowFade" );
    }
    ApplyMultiDamage();

    CPASAttenuationFilter snd( this );
    EmitSound( snd, entindex(), "Weapon_IonCannon.Single" );

    DrawBeam( src, tr.endpos, 4.0f );

    Vector forward;
    AngleVectors( ang, &forward );
    pPlayer->ApplyAbsVelocityImpulse( -forward * ( dmg * 15.0f ) );
}

//-----------------------------------------------------------------------------
// Charged blast: heavy damage + thick beam + explosion
//-----------------------------------------------------------------------------
void CWeaponTauCannon::FireChargedPulse()
{
    CBasePlayer* pPlayer = ToBasePlayer( GetOwner() );
    if ( !pPlayer ) return;

    m_bCharging = false;
    StopChargeSound();

    CPASAttenuationFilter snd( this );
    EmitSound( snd, entindex(), "PropJeep.FireChargedCannon" );

    Vector src = pPlayer->Weapon_ShootPosition();
    QAngle ang = pPlayer->EyeAngles();
    Vector dir;
    AngleVectors( ang, &dir );

    trace_t tr;
    UTIL_TraceLine( src, src + dir * 8192, MASK_SHOT, this,
                    COLLISION_GROUP_NONE, &tr );

    ClearMultiDamage();

    float frac     = clamp( ( gpGlobals->curtime - m_flChargeStartTime ) / MAX_TAU_CHARGE_TIME, 0.0f, 1.0f );
    float flDamage = 30 + ( 270 * frac );  // 30 → 300

    if ( tr.m_pEnt )
    {
        CTakeDamageInfo info( this, pPlayer, flDamage, DMG_SHOCK );
        CalculateBulletDamageForce( &info, m_iAmmoType, dir, tr.endpos, 1.0f + frac * 4.0f );
        tr.m_pEnt->DispatchTraceAttack( info, dir, &tr );
    }
    ApplyMultiDamage();

    if ( tr.DidHitWorld() && !( tr.surface.flags & SURF_SKY ) )
    {
        UTIL_ImpactTrace( &tr, m_iAmmoType, "ImpactJeep" );
        UTIL_DecalTrace( &tr, "RedGlowFade" );
        CPVSFilter filter( tr.endpos );
        te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
    }

    DrawBeam( src, tr.endpos, 9.6f );

    Vector forward;
    AngleVectors( ang, &forward );
    pPlayer->ApplyAbsVelocityImpulse( -forward * ( flDamage * 500.0f ) );
}

#endif // !CLIENT_DLL
