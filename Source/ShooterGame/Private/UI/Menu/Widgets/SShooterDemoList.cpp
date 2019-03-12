// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "SShooterDemoList.h"
#include "SHeaderRow.h"
#include "ShooterStyle.h"
#include "ShooterGameLoadingScreen.h"
#include "ShooterGameInstance.h"

#define LOCTEXT_NAMESPACE "ShooterGame.HUD.Menu"

void SShooterDemoList::Construct(const FArguments& InArgs)
{
	PlayerOwner			= InArgs._PlayerOwner;
	OwnerWidget			= InArgs._OwnerWidget;
	bBuildingDemoList	= false;
	StatusText			= FString();
	
	const int32 BoxWidth = 125;

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBox)  
			.WidthOverride(600)
			.HeightOverride(300)
			[
				SAssignNew(DemoListWidget, SListView<TSharedPtr<FDemoEntry>>)
				.ItemHeight(20)
				.ListItemsSource(&DemoList)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SShooterDemoList::MakeListViewWidget)
				.OnSelectionChanged(this, &SShooterDemoList::EntrySelectionChanged)
				.OnMouseButtonDoubleClick(this,&SShooterDemoList::OnListItemDoubleClicked)
				.HeaderRow(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("DemoName").FixedWidth(BoxWidth*2).DefaultLabel(NSLOCTEXT("DemoList", "DemoNameColumn", "Demo Name"))
					+ SHeaderRow::Column("Date").FixedWidth(BoxWidth*2).DefaultLabel(NSLOCTEXT("DemoList", "DateColumn", "Date"))
					+ SHeaderRow::Column("Size").HAlignHeader(HAlign_Left).HAlignCell(HAlign_Right).DefaultLabel(NSLOCTEXT("DemoList", "SizeColumn", "Size")))
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SShooterDemoList::GetBottomText)
				.TextStyle(FShooterStyle::Get(), "ShooterGame.MenuServerListTextStyle")
			]
		]
	];

	BuildDemoList();
}

/** Updates the list until it's completely populated 
  * While this function is currently a blocking call, it's designed to eventually allow 
  * the demos to come from the cloud and download asynchronously
  */
void SShooterDemoList::UpdateBuildDemoListStatus()
{
	check(bBuildingDemoList); // should not be called otherwise

	bool bFinished = true;

	const FString DemoPath = FPaths::GameSavedDir() + TEXT( "Demos/" );
	const FString WildCardPath = DemoPath + TEXT( "*.demo" );

	TArray<FString> FileNames;
	IFileManager::Get().FindFiles( FileNames, *WildCardPath, true, false );

	for ( int32 i = 0; i < FileNames.Num(); i++ )
	{
		TSharedPtr<FDemoEntry> NewDemoEntry = MakeShareable( new FDemoEntry() );

		float Size = (float)IFileManager::Get().FileSize( *( DemoPath + FileNames[i] ) ) / 1024;

		NewDemoEntry->DemoName		= FileNames[i];
		NewDemoEntry->DateTime		= IFileManager::Get().GetTimeStamp( *( DemoPath + FileNames[i] ) );
		NewDemoEntry->Date			= NewDemoEntry->DateTime.ToString( TEXT( "%m/%d/%Y %h:%m %A" ) );	// UTC time
		NewDemoEntry->Size			= Size >= 1024.0f ? FString::Printf( TEXT("%2.2f MB" ), Size / 1024.0f ) : FString::Printf( TEXT("%i KB" ), (int)Size );
		NewDemoEntry->ResultsIndex	= i;

		DemoList.Add( NewDemoEntry );
	}

	// Sort demo names by date
	struct FCompareDateTime
	{
		FORCEINLINE bool operator()( const TSharedPtr<FDemoEntry> & A, const TSharedPtr<FDemoEntry> & B ) const
		{
			return A->DateTime.GetTicks() > B->DateTime.GetTicks();
		}
	};

	Sort( DemoList.GetData(), DemoList.Num(), FCompareDateTime() );

	StatusText = "";

	if ( bFinished )
	{		
		OnBuildDemoListFinished();
	}
}

FString SShooterDemoList::GetBottomText() const
{
	 return StatusText;
}

/**
 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
 *
 * @param  AllottedGeometry The space allotted for this widget
 * @param  InCurrentTime  Current absolute real time
 * @param  InDeltaTime  Real time passed since last tick
 */
void SShooterDemoList::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if ( bBuildingDemoList )
	{
		UpdateBuildDemoListStatus();
	}
}

/** Populates the demo list */
void SShooterDemoList::BuildDemoList()
{
	bBuildingDemoList = true;
	DemoList.Empty();
}

/** Called when demo list building is finished */
void SShooterDemoList::OnBuildDemoListFinished()
{
	bBuildingDemoList = false;

	int32 SelectedItemIndex = DemoList.IndexOfByKey(SelectedItem);

	DemoListWidget->RequestListRefresh();
	if (DemoList.Num() > 0)
	{
		DemoListWidget->UpdateSelectionSet();
		DemoListWidget->SetSelection(DemoList[SelectedItemIndex > -1 ? SelectedItemIndex : 0],ESelectInfo::OnNavigation);
	}
}

void SShooterDemoList::PlayDemo()
{
	if (bBuildingDemoList)
	{
		// We're still building the list
		return;
	}

	if (SelectedItem.IsValid())
	{
		UShooterGameInstance* const GI = Cast<UShooterGameInstance>(PlayerOwner->GetGameInstance());

		if ( GI != NULL )
		{
			FString DemoName = SelectedItem->DemoName;
			
			DemoName.RemoveFromEnd( TEXT(".demo") );

			// Play the demo
			GEngine->Exec( GI->GetWorld(), *FString::Printf( TEXT("DEMOPLAY %s"), *DemoName ) );
		}
	}
}

void SShooterDemoList::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	if (InFocusEvent.GetCause() != EFocusCause::SetDirectly)
	{
		FSlateApplication::Get().SetKeyboardFocus(SharedThis( this ));
	}
}

FReply SShooterDemoList::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::SetDirectly, true);
}

void SShooterDemoList::EntrySelectionChanged(TSharedPtr<FDemoEntry> InItem, ESelectInfo::Type SelectInfo)
{
	SelectedItem = InItem;
}

void SShooterDemoList::OnListItemDoubleClicked(TSharedPtr<FDemoEntry> InItem)
{
	SelectedItem = InItem;
	PlayDemo();
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
}

void SShooterDemoList::MoveSelection(int32 MoveBy)
{
	const int32 SelectedItemIndex = DemoList.IndexOfByKey(SelectedItem);

	if (SelectedItemIndex+MoveBy > -1 && SelectedItemIndex+MoveBy < DemoList.Num())
	{
		DemoListWidget->SetSelection(DemoList[SelectedItemIndex+MoveBy]);
	}
}

FReply SShooterDemoList::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent) 
{
	if (bBuildingDemoList) // lock input
	{
		return FReply::Handled();
	}

	FReply Result = FReply::Unhandled();
	const FKey Key = InKeyboardEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		PlayDemo();
		Result = FReply::Handled();
		FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
	}
	//hit space bar or left gamepad face button to search for demos again / refresh the list, only when not searching already
	else if (Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Left)
	{
		// Refresh demo list
		BuildDemoList();
	}
	else if (Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		MoveSelection(-1);
		Result = FReply::Handled();
	}
	else if (Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		MoveSelection(1);
		Result = FReply::Handled();
	}
	return Result;
}

TSharedRef<ITableRow> SShooterDemoList::MakeListViewWidget(TSharedPtr<FDemoEntry> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	class SDemoEntryWidget : public SMultiColumnTableRow< TSharedPtr<FDemoEntry> >
	{
	public:
		SLATE_BEGIN_ARGS(SDemoEntryWidget){}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FDemoEntry> InItem)
		{
			Item = InItem;
			SMultiColumnTableRow< TSharedPtr<FDemoEntry> >::Construct(FSuperRowType::FArguments(), InOwnerTable);
		}

		TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName)
		{
			FString ItemText;

			if (ColumnName == "DemoName")
			{
				ItemText = Item->DemoName;// + "extra stuff here";
			}
			else if (ColumnName == "Date")
			{
				ItemText = Item->Date;
			}
			else if (ColumnName == "Size")
			{
				ItemText = Item->Size;
			}

			return SNew(STextBlock)
				.Text(ItemText)
				.TextStyle(FShooterStyle::Get(), "ShooterGame.MenuServerListTextStyle");
		}
		TSharedPtr<FDemoEntry> Item;
	};
	return SNew(SDemoEntryWidget, OwnerTable, Item);
}

#undef LOCTEXT_NAMESPACE