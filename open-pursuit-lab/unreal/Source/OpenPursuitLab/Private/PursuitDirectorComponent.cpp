#include "PursuitDirectorComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

static_assert(static_cast<uint8>(opl::PursuitState::Patrol) == static_cast<uint8>(EOPLPursuitState::Patrol));
static_assert(static_cast<uint8>(opl::PursuitState::Investigate) == static_cast<uint8>(EOPLPursuitState::Investigate));
static_assert(static_cast<uint8>(opl::PursuitState::Chase) == static_cast<uint8>(EOPLPursuitState::Chase));
static_assert(static_cast<uint8>(opl::PursuitState::Search) == static_cast<uint8>(EOPLPursuitState::Search));
static_assert(static_cast<uint8>(opl::PursuitState::Cooldown) == static_cast<uint8>(EOPLPursuitState::Cooldown));

UPursuitDirectorComponent::UPursuitDirectorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

bool UPursuitDirectorComponent::SubmitAuthorityObservation(bool bSuspectVisible, uint8 IncidentSeverity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || IncidentSeverity > 3) return false;
    bVisible = bSuspectVisible;
    PendingSeverity = FMath::Max(PendingSeverity, IncidentSeverity);
    return true;
}

void UPursuitDirectorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!GetOwner() || !GetOwner()->HasAuthority() || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f) return;
    constexpr double Step = 1.0 / opl::kTickHz;
    // A bring-up guard, not the production physics scheduler. Hitches slow policy time.
    const double SafeDelta = FMath::Min(static_cast<double>(DeltaTime), 0.25);
    Accumulator += SafeDelta;
    const int32 Available = FMath::FloorToInt(Accumulator / Step);
    const int32 Count = FMath::Min(Available, 4);
    DroppedPolicyTicks += Available - Count;
    if (DeltaTime > 0.25f) DroppedPolicyTicks += FMath::FloorToInt((DeltaTime - 0.25) / Step);
    Accumulator -= Available * Step;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        Policy.step(bVisible, PendingSeverity);
        PendingSeverity = 0; // Consume each incident once, even with multiple substeps.
    }
    AlertState = static_cast<EOPLPursuitState>(Policy.state());
    Heat = Policy.heat();
}

void UPursuitDirectorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UPursuitDirectorComponent, AlertState);
    DOREPLIFETIME(UPursuitDirectorComponent, Heat);
}
