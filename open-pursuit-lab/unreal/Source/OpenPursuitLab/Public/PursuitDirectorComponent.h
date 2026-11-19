#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "opl/core.hpp"
#include "PursuitDirectorComponent.generated.h"

UENUM(BlueprintType)
enum class EOPLPursuitState : uint8
{
    Patrol, Investigate, Chase, Search, Cooldown
};

// Integration scaffold only: no map, vehicle, perception system or RPC transport.
UCLASS(ClassGroup=(OpenPursuitLab), meta=(BlueprintSpawnableComponent))
class OPENPURSUITLAB_API UPursuitDirectorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPursuitDirectorComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Call from server perception only. This is deliberately not a client RPC.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Open Pursuit Lab")
    bool SubmitAuthorityObservation(bool bSuspectVisible, uint8 IncidentSeverity);

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Open Pursuit Lab")
    EOPLPursuitState AlertState = EOPLPursuitState::Patrol;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="Open Pursuit Lab")
    uint8 Heat = 0;

    UPROPERTY(BlueprintReadOnly, Category="Open Pursuit Lab")
    int64 DroppedPolicyTicks = 0;
private:
    opl::Pursuit Policy;
    double Accumulator = 0.0;
    bool bVisible = false;
    uint8 PendingSeverity = 0;
};
