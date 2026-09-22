#pragma once

#include "CoreMinimal.h"

/** Plain report. Safe for the editor module to include. */
struct HOWDIDIDIE_API FHDIDValidationReport
{
	TArray<FString> Errors;

	bool IsValid() const { return Errors.Num() == 0; }
};

/** Spec section 43. Mirrors Tools/narrative_data.py. */
class HOWDIDIDIE_API FHDIDNarrativeValidator
{
public:
	static FHDIDValidationReport ValidateProject();
	static void LogReport(const FHDIDValidationReport& Report);
};
