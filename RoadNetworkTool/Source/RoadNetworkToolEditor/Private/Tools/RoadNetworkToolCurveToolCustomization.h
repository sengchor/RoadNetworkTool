// RoadNetworkToolLineToolCustomization.h

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "PropertyHandle.h"

/**
 * Customization for URoadNetworkToolLineToolProperties
 */
class FRoadNetworkToolCurveToolCustomization : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Callback for when the Create button is clicked */
	FReply OnDebugButtonClicked();
};
