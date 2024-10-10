// RoadNetworkToolLineToolCustomization.cpp

#include "RoadNetworkToolCurveToolCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "RoadNetworkToolLineTool.h"
#include "Engine/Selection.h"
#include "RoadMeshGenerator.h"

TSharedRef<IDetailCustomization> FRoadNetworkToolCurveToolCustomization::MakeInstance()
{
	return MakeShareable(new FRoadNetworkToolCurveToolCustomization);
}

void FRoadNetworkToolCurveToolCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Create a new category named "Options"
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Straight Road to Curved Road");

	// Add the "Create" button
	Category.AddCustomRow(FText::FromString("Debug Button"))
		.ValueContent()
		[
			SNew(SButton)
				.Text(FText::FromString("Visualize"))
				.OnClicked(FOnClicked::CreateSP(this, &FRoadNetworkToolCurveToolCustomization::OnDebugButtonClicked))
		];
}

FReply FRoadNetworkToolCurveToolCustomization::OnDebugButtonClicked()
{
	// Get the currently selected actors in the editor
	USelection* SelectedActors = GEditor->GetSelectedActors();

	if (SelectedActors)
	{
		for (FSelectionIterator It(*SelectedActors); It; ++It)
		{
			ARoadActor* SelectedRoadActor = Cast<ARoadActor>(*It);
			if (SelectedRoadActor)
			{
				FRoadMeshGenerator RoadMeshGenerator;
				RoadMeshGenerator.DrawDebugCurvedRoad(SelectedRoadActor);
				//RoadMeshGenerator.GenerateRoadMeshCurve(SelectedRoadActor);

				break;
			}
		}
	}

	// Handle the button click event here
	// You can call functions on the tool or do other actions
	UE_LOG(LogTemp, Warning, TEXT("Debug button clicked!"));
	return FReply::Handled();
}
