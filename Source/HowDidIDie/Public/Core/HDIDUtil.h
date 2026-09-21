#pragma once

#include "CoreMinimal.h"
#include "Data/HDIDTypes.h"

/** Shared helpers. No chapter-specific rules live here. */

inline bool HDIDParseClock(const FString& Text, int32& OutMinutes)
{
	TArray<FString> Parts;
	Text.ParseIntoArray(Parts, TEXT(":"), true);
	if (Parts.Num() != 2 || Parts[0].Len() != 2 || Parts[1].Len() != 2)
	{
		return false;
	}
	const int32 Hours = FCString::Atoi(*Parts[0]);
	const int32 Minutes = FCString::Atoi(*Parts[1]);
	if (Hours < 0 || Hours > 23 || Minutes < 0 || Minutes > 59)
	{
		return false;
	}
	if (!FChar::IsDigit(Parts[0][0]) || !FChar::IsDigit(Parts[0][1]) || !FChar::IsDigit(Parts[1][0]) || !FChar::IsDigit(Parts[1][1]))
	{
		return false;
	}
	OutMinutes = Hours * 60 + Minutes;
	return true;
}

inline FLinearColor HDIDHexColor(const FString& Hex, float Alpha = 1.f)
{
	if (Hex.Len() != 6)
	{
		return FLinearColor(0.08f, 0.09f, 0.11f, Alpha);
	}
	auto Nibble = [](TCHAR C) -> int32 { return FParse::HexDigit(C); };
	const int32 R = Nibble(Hex[0]) * 16 + Nibble(Hex[1]);
	const int32 G = Nibble(Hex[2]) * 16 + Nibble(Hex[3]);
	const int32 B = Nibble(Hex[4]) * 16 + Nibble(Hex[5]);
	return FLinearColor(R / 255.f, G / 255.f, B / 255.f, Alpha);
}

inline FVector HDIDScaleOrOne(const FHDIDVec3& Scale)
{
	if (FMath::IsNearlyZero(Scale.X) && FMath::IsNearlyZero(Scale.Y) && FMath::IsNearlyZero(Scale.Z))
	{
		return FVector::OneVector;
	}
	return FVector(FMath::Max(Scale.X, 0.01f), FMath::Max(Scale.Y, 0.01f), FMath::Max(Scale.Z, 0.01f));
}

inline bool HDIDConditionsMet(const TArray<FHDIDCondition>& Conditions, const TMap<FString, FString>& Flags)
{
	for (const FHDIDCondition& Condition : Conditions)
	{
		const FString* Value = Flags.Find(Condition.Flag);
		if (!Value || *Value != Condition.Equals)
		{
			return false;
		}
	}
	return true;
}

inline bool HDIDAnyConditionMet(const TArray<FHDIDCondition>& Conditions, const TMap<FString, FString>& Flags)
{
	for (const FHDIDCondition& Condition : Conditions)
	{
		const FString* Value = Flags.Find(Condition.Flag);
		if (Value && *Value == Condition.Equals)
		{
			return true;
		}
	}
	return false;
}
