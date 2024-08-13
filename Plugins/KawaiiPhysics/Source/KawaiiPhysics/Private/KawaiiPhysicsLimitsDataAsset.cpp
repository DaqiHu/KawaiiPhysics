// Fill out your copyright notice in the Description page of Project Settings.


#include "KawaiiPhysicsLimitsDataAsset.h"
#include "AnimNode_KawaiiPhysics.h"
#include "KawaiiPhysics.h"

DEFINE_LOG_CATEGORY(LogKawaiiPhysics);

struct FCollisionLimitDataCustomVersion
{
	enum Type
	{
		// FNameからFBoneReferenceに移行
		ChangeToBoneReference = 0,
		DeprecateLimitData,

		// ------------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FCollisionLimitDataCustomVersion()
	{
	}
};

const FGuid FCollisionLimitDataCustomVersion::GUID(0x3A1F7B2E, 0x7B9D6E8C, 0x4C2A9F1D, 0x85B3E4F1);
FCustomVersionRegistration GRegisterCollisionLimitDataCustomVersion(FCollisionLimitDataCustomVersion::GUID,
                                                                    FCollisionLimitDataCustomVersion::LatestVersion,
                                                                    TEXT("CollisionLimitData"));

#if WITH_EDITOR
template <typename CollisionLimitType>
void UpdateCollisionLimit(TArray<CollisionLimitType>& CollisionLimitsData, const CollisionLimitType& NewLimit)
{
	for (auto& LimitData : CollisionLimitsData)
	{
		if (LimitData.Guid == NewLimit.Guid)
		{
			LimitData = NewLimit;
			break;
		}
	}
}


void UKawaiiPhysicsLimitsDataAsset::UpdateLimit(FCollisionLimitBase* Limit)
{
	switch (Limit->Type)
	{
	case ECollisionLimitType::Spherical:
		UpdateCollisionLimit(SphericalLimits, *static_cast<FSphericalLimit*>(Limit));
		break;
	case ECollisionLimitType::Capsule:
		UpdateCollisionLimit(CapsuleLimits, *static_cast<FCapsuleLimit*>(Limit));
		break;
	case ECollisionLimitType::Box:
		UpdateCollisionLimit(BoxLimits, *static_cast<FBoxLimit*>(Limit));
		break;
	case ECollisionLimitType::Planar:
		UpdateCollisionLimit(PlanarLimits, *static_cast<FPlanarLimit*>(Limit));
		break;
	case ECollisionLimitType::None:
		break;
	default:
		break;
	}

	MarkPackageDirty();
}

template <typename CollisionLimitDataType, typename CollisionLimitType>
void SyncCollisionLimits(const TArray<CollisionLimitDataType>& CollisionLimitData,
                         TArray<CollisionLimitType>& CollisionLimits)
{
	CollisionLimits.Empty();
	for (const auto& Data : CollisionLimitData)
	{
		CollisionLimits.Add(Data.Convert());
	}
}

void UKawaiiPhysicsLimitsDataAsset::Sync()
{
	SyncCollisionLimits(SphericalLimitsData, SphericalLimits);
	SyncCollisionLimits(CapsuleLimitsData, CapsuleLimits);
	SyncCollisionLimits(BoxLimitsData, BoxLimits);
	SyncCollisionLimits(PlanarLimitsData, PlanarLimits);
}


void UKawaiiPhysicsLimitsDataAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.MemberProperty
		                           ? PropertyChangedEvent.MemberProperty->GetFName()
		                           : NAME_None;

	if (PropertyName == FName(TEXT("SphericalLimitsData")))
	{
		if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Duplicate)
		{
			SphericalLimits[PropertyChangedEvent.GetArrayIndex(PropertyName.ToString())].Guid = FGuid::NewGuid();
		}
	}
	else if (PropertyName == FName(TEXT("CapsuleLimitsData")))
	{
		if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Duplicate)
		{
			CapsuleLimits[PropertyChangedEvent.GetArrayIndex(PropertyName.ToString())].Guid = FGuid::NewGuid();
		}
	}
	else if (PropertyName == FName(TEXT("BoxLimitsData")))
	{
		if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Duplicate)
		{
			BoxLimits[PropertyChangedEvent.GetArrayIndex(PropertyName.ToString())].Guid = FGuid::NewGuid();
		}
	}
	else if (PropertyName == FName(TEXT("PlanarLimitsData")))
	{
		if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Duplicate)
		{
			PlanarLimits[PropertyChangedEvent.GetArrayIndex(PropertyName.ToString())].Guid = FGuid::NewGuid();
		}
	}

	OnLimitsChanged.Broadcast(PropertyChangedEvent);
}
#endif

#if WITH_EDITORONLY_DATA
void UKawaiiPhysicsLimitsDataAsset::Serialize(FStructuredArchiveRecord Record)
{
	Super::Serialize(Record);

	Record.GetUnderlyingArchive().UsingCustomVersion(FCollisionLimitDataCustomVersion::GUID);
}
#endif

USkeleton* UKawaiiPhysicsLimitsDataAsset::GetSkeleton(bool& bInvalidSkeletonIsError,
                                                      const IPropertyHandle* PropertyHandle)
{
#if WITH_EDITORONLY_DATA
	return Skeleton;
#else
	return nullptr;
#endif
}

void UKawaiiPhysicsLimitsDataAsset::PostLoad()
{
	Super::PostLoad();

	if (GetLinkerCustomVersion(FCollisionLimitDataCustomVersion::GUID) <
		FCollisionLimitDataCustomVersion::ChangeToBoneReference)
	{
#if WITH_EDITORONLY_DATA
		for (auto& Data : SphericalLimitsData)
		{
			Data.DrivingBoneReference = FBoneReference(Data.DrivingBoneName);
		}
		for (auto& Data : CapsuleLimitsData)
		{
			Data.DrivingBoneReference = FBoneReference(Data.DrivingBoneName);
		}
		for (auto& Data : BoxLimitsData)
		{
			Data.DrivingBoneReference = FBoneReference(Data.DrivingBoneName);
		}
		for (auto& Data : PlanarLimitsData)
		{
			Data.DrivingBoneReference = FBoneReference(Data.DrivingBoneName);
		}
		UE_LOG(LogKawaiiPhysics, Log, TEXT("Update : BoneName -> BoneReference (%s)"), *this->GetName());
#endif
	}

	if (GetLinkerCustomVersion(FCollisionLimitDataCustomVersion::GUID) <
		FCollisionLimitDataCustomVersion::DeprecateLimitData)
	{
#if WITH_EDITORONLY_DATA
		Sync();
		UE_LOG(LogKawaiiPhysics, Log, TEXT("Update : Deprecate LimitData (%s)"), *this->GetName());
#endif
	}
}
