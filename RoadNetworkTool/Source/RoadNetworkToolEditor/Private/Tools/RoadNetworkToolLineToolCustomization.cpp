// RoadNetworkToolLineToolCustomization.cpp

#include "RoadNetworkToolLineToolCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "RoadNetworkToolLineTool.h"
#include "Engine/Selection.h"
#include "RoadMeshGenerator.h"

TSharedRef<IDetailCustomization> FRoadNetworkToolLineToolCustomization::MakeInstance()
{
	return MakeShareable(new FRoadNetworkToolLineToolCustomization);
}

void FRoadNetworkToolLineToolCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Create a new category named "Options"
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("New Road Network");

	// Add properties first
	TSharedPtr<IPropertyHandle> EnableDebugLineProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URoadNetworkToolLineToolProperties, EnableDebugLine));
	Category.AddProperty(EnableDebugLineProperty);

	TSharedPtr<IPropertyHandle> WidthProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URoadNetworkToolLineToolProperties, Width));
	Category.AddProperty(WidthProperty);

	TSharedPtr<IPropertyHandle> ThicknessProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URoadNetworkToolLineToolProperties, Thickness));
	Category.AddProperty(ThicknessProperty);

	TSharedPtr<IPropertyHandle> CheckLineIntersectionProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URoadNetworkToolLineToolProperties, CheckLineIntersection));
	Category.AddProperty(CheckLineIntersectionProperty);

	TSharedPtr<IPropertyHandle> SnapThresholdProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(URoadNetworkToolLineToolProperties, SnapThreshold));
	Category.AddProperty(SnapThresholdProperty);

	// Add the "Create" button
	Category.AddCustomRow(FText::FromString("Create Button"))
		.ValueContent()
		[
			SNew(SButton)
				.Text(FText::FromString("Create"))
				.OnClicked(FOnClicked::CreateSP(this, &FRoadNetworkToolLineToolCustomization::OnCreateButtonClicked))
		];
}

FReply FRoadNetworkToolLineToolCustomization::OnCreateButtonClicked()
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
				RoadMeshGenerator.GenerateRoadMesh(SelectedRoadActor);
				//RoadMeshGenerator.GenerateRoadMeshCurve(SelectedRoadActor);

				break;
			}
		}
	}

	// Handle the button click event here
	// You can call functions on the tool or do other actions
	UE_LOG(LogTemp, Warning, TEXT("Create button clicked!"));
	return FReply::Handled();
}
