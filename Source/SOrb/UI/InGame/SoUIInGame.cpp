// Copyright (c) Csaba Molnar & Daniel Butum. All Rights Reserved.

#include "SoUIInGame.h"

#include "Blueprint/UserWidget.h"
#include "Basic/Helpers/SoStaticHelper.h"

UUserWidget* USoUIInGame::GetNotUsedWidget(TSubclassOf<UUserWidget> Class)
{
	FSoUIWidgetArray* WidgetArray = WidgetStorage.Find(Class);

	if (WidgetArray == nullptr)
		WidgetArray = &WidgetStorage.Add(Class);

	for (UUserWidget* Widget : WidgetArray->Widgets)
		if (!Widget->IsVisible())
			return Widget;

	UUserWidget* Widget = CreateWidget(USoStaticHelper::GetPlayerController(this), Class);
	if (Widget != nullptr)
	{
		Widget->AddToViewport();
		WidgetArray->Widgets.Add(Widget);
	}

	return Widget;
}
