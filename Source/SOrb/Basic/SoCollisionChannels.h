// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

// Proper alias the collision channels
static constexpr ECollisionChannel ECC_Projectile = ECollisionChannel::ECC_GameTraceChannel1;
static constexpr ECollisionChannel ECC_CharacterLogic = ECollisionChannel::ECC_GameTraceChannel2;
static constexpr ECollisionChannel ECC_SplineMoverLogic = ECollisionChannel::ECC_GameTraceChannel3;
static constexpr ECollisionChannel ECC_Weapon = ECollisionChannel::ECC_GameTraceChannel4;
static constexpr ECollisionChannel ECC_Hide = ECollisionChannel::ECC_GameTraceChannel5;
static constexpr ECollisionChannel ECC_TriggerProjectile = ECollisionChannel::ECC_GameTraceChannel6;
static constexpr ECollisionChannel ECC_TestStone = ECollisionChannel::ECC_GameTraceChannel7;
static constexpr ECollisionChannel ECC_PawnOverlapChar = ECollisionChannel::ECC_GameTraceChannel8;
