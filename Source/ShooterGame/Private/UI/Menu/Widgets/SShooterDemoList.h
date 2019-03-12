// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateBasics.h"
#include "SlateExtras.h"
#include "ShooterGame.h"
#include "SShooterMenuWidget.h"

struct FDemoEntry
{
	FString		DemoName;
	FDateTime	DateTime;
	FString		Date;
	FString		Size;
	int32		ResultsIndex;
};

//class declare
class SShooterDemoList : public SShooterMenuWidget
{
public:
	SLATE_BEGIN_ARGS(SShooterDemoList)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<ULocalPlayer>, PlayerOwner)
	SLATE_ARGUMENT(TSharedPtr<SWidget>, OwnerWidget)

	SLATE_END_ARGS()

	/** needed for every widget */
	void Construct(const FArguments& InArgs);

	/** if we want to receive focus */
	virtual bool SupportsKeyboardFocus() const override { return true; }

	/** focus received handler - keep the ActionBindingsList focused */
	virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
	
	/** focus lost handler - keep the ActionBindingsList focused */
	virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;

	/** key down handler */
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent) override;

	/** SListView item double clicked */
	void OnListItemDoubleClicked(TSharedPtr<FDemoEntry> InItem);

	/** creates single item widget, called for every list item */
	TSharedRef<ITableRow> MakeListViewWidget(TSharedPtr<FDemoEntry> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** selection changed handler */
	void EntrySelectionChanged(TSharedPtr<FDemoEntry> InItem, ESelectInfo::Type SelectInfo);

	/** Updates the list until it's completely populated */
	void UpdateBuildDemoListStatus();

	/** Populates the demo list */
	void BuildDemoList();

	/** Called when demo list building finished */
	void OnBuildDemoListFinished();

	/** Play chosen demo */
	void PlayDemo();

	/** selects item at current + MoveBy index */
	void MoveSelection(int32 MoveBy);

	/**
	 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
	 *
	 * @param  AllottedGeometry The space allotted for this widget
	 * @param  InCurrentTime  Current absolute real time
	 * @param  InDeltaTime  Real time passed since last tick
	 */
	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );

protected:

	/** Whether we're building the demo list or not */
	bool bBuildingDemoList;

	/** action bindings array */
	TArray< TSharedPtr<FDemoEntry> > DemoList;

	/** action bindings list slate widget */
	TSharedPtr< SListView< TSharedPtr<FDemoEntry> > > DemoListWidget; 

	/** currently selected list item */
	TSharedPtr<FDemoEntry> SelectedItem;

	/** get current status text */
	FString GetBottomText() const;

	/** current status text */
	FString StatusText;

	/** pointer to our owner PC */
	TWeakObjectPtr<class ULocalPlayer> PlayerOwner;

	/** pointer to our parent widget */
	TSharedPtr<class SWidget> OwnerWidget;
};


