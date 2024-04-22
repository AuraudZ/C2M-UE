// Copyright Epic Games, Inc. All Rights Reserved.

#include "C2M_v2.h"
#include "C2M_v2Style.h"
#include "C2M_v2Commands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "C2MReader.h"

//File picker includes
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Runtime/Rawmesh/Public/RawMesh.h"
#include <IMeshReductionInterfaces.h>
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"


static const FName C2M_v2TabName("C2M_v2");

#define LOCTEXT_NAMESPACE "FC2M_v2Module"

void FC2M_v2Module::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FC2M_v2Style::Initialize();
	FC2M_v2Style::ReloadTextures();

	FC2M_v2Commands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FC2M_v2Commands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FC2M_v2Module::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FC2M_v2Module::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(C2M_v2TabName, FOnSpawnTab::CreateRaw(this, &FC2M_v2Module::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FC2M_v2TabTitle", "C2M_v2"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FC2M_v2Module::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FC2M_v2Style::Shutdown();

	FC2M_v2Commands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(C2M_v2TabName);
}

TSharedRef<SDockTab> FC2M_v2Module::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::FromString("Hello C2M_v2!");

	FString C2MText;
	C2MText = "C2M_v2!";

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(FText::FromString("Test Button"))
				.OnClicked(FOnClicked::CreateLambda([] () {
					UE_LOG(LogTemp, Warning, TEXT("Button Clicked!"));

					// Open a file dialog
					FString path;

					IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
					TSharedPtr<SWindow> MainWindow = MainFrameModule.GetParentWindow();
					if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid()) {
						void* ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();
						IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
						if (DesktopPlatform)
						{
							//Opening the file picker!
							const FString& DialogTitle = TEXT("Select a C2M File");
							const FString& DefaultPath = TEXT("");
							// only allow .c2m files
							const FString& FileTypes = TEXT("C2M Files (*.c2m)|*.c2m");
							TArray<FString> OutFileName;
							

							uint32 SelectionFlag = 0; //A value of 0 represents single file selection while a value of 1 represents multiple file selection
							bool result = DesktopPlatform->OpenFileDialog(ParentWindowHandle, DialogTitle, DefaultPath, FString(""), FileTypes, SelectionFlag, OutFileName);
								
							if (result) {
								path = OutFileName[0];
								UE_LOG(LogTemp, Warning, TEXT("File path: %s"), *path);
								std::string ewpath = TCHAR_TO_UTF8(*path);
								auto map = CodMap(ewpath);
								auto name_char = UTF8_TO_TCHAR(map.Name.c_str());
								UE_LOG(LogTemp, Warning, TEXT("Map Name: %s"), name_char);
								UE_LOG(LogTemp, Warning, TEXT("Mesh Name: %s"), *FString(UTF8_TO_TCHAR(map.meshes[0].Name.c_str())));
								auto mesh = map.meshes[0];
								FString NewPackageName = TEXT("/Game/") + (FString(UTF8_TO_TCHAR(map.Name.c_str()))) + TEXT("/") + (FString(UTF8_TO_TCHAR(mesh.Name.c_str())));
								UE_LOG(LogTemp, Warning, TEXT("New Package Name: %s"), *NewPackageName);
								UPackage* Package = CreatePackage(*NewPackageName);
								Package->FullyLoad();
								Package->Modify();
								FRawMesh rawMesh;
								rawMesh.VertexPositions.AddZeroed(mesh.Vertices.size());
								for (int i = 0; i < mesh.Vertices.size(); i++) {
									FVector vertex = FVector(mesh.Vertices[i].X, mesh.Vertices[i].Y, mesh.Vertices[i].Z);
									rawMesh.VertexPositions[i] = FVector3f(vertex);
								}
								for (int i = 0; i < mesh.Surfaces.size(); i++) {
									auto surface = mesh.Surfaces[i];
									for (int j = 0; j < surface.Faces.size(); j++) {
										auto face = surface.Faces[j];
										for (int k = 0; k < 3; k++) {
											int vertexIndex = face[k];
											FVector normal = FVector(mesh.Normals[vertexIndex]);
											rawMesh.WedgeIndices.Add(vertexIndex);
											rawMesh.WedgeTexCoords[0].Add(FVector2f(mesh.UVs[vertexIndex][0][0], mesh.UVs[vertexIndex][0][1]));
											rawMesh.WedgeTangentX.Add(FVector3f::ZeroVector);
											rawMesh.WedgeTangentY.Add(FVector3f::ZeroVector);
											rawMesh.WedgeTangentZ.Add(FVector3f(normal));
											rawMesh.WedgeColors.Add(mesh.Colors[vertexIndex]);
										}
									}
								}

								if (rawMesh.WedgeIndices.Num() == 0) {
									UE_LOG(LogTemp, Warning, TEXT("WedgeIndices size is 0!"));
									return FReply::Handled();
								}
								FName meshName = FName(*FString(UTF8_TO_TCHAR(mesh.Name.c_str())));
								FlushRenderingCommands();
								FStaticMeshComponentRecreateRenderStateContext RecreateRenderStateContext(FindObject<UStaticMesh>(InOuter, *InName.ToString()));
								auto StaticMesh = NewObject<UStaticMesh>(Package, meshName, RF_Public | RF_Standalone);
								// Add one LOD for the base mesh
								FStaticMeshSourceModel SrcModel;
								SrcModel.CreateSubObjects(StaticMesh);
								SrcModel.BuildSettings.bRecomputeNormals = false;
								SrcModel.BuildSettings.bRecomputeTangents = false;
								SrcModel.BuildSettings.bRemoveDegenerates = false;
								SrcModel.ReductionSettings.bRecalculateNormals = false;
								SrcModel.SaveRawMesh(rawMesh,false);

								//TArray<FStaticMeshSourceModel> temp;
								//temp.Add(MoveTemp(SrcModel));

							//	StaticMesh->SetSourceModels(MoveTemp(temp));

								TArray<FText> errorList;
							/*	StaticMesh->Build(false, &errorList);
								for (int i = 0; i < errorList.Num(); i++)
									UE_LOG(LogTemp, Error, TEXT("%s"), *(errorList[i].ToString()));

								StaticMesh->MarkPackageDirty();
								FAssetRegistryModule::AssetCreated(StaticMesh);

								StaticMesh->PostEditChange();

								Package->SetDirtyFlag(true);

								FString PackageFileName = FPackageName::LongPackageNameToFilename(NewPackageName, FPackageName::GetAssetPackageExtension());

								FSavePackageArgs SaveArgs;
								SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
								SaveArgs.Error = GError;
								SaveArgs.bWarnOfLongFilename = false;

								if (UPackage::SavePackage(Package, StaticMesh, *NewPackageName, SaveArgs)) {
									UE_LOG(LogTemp, Warning, TEXT("Saved Static Mesh!"));
								}
								else {
									UE_LOG(LogTemp, Warning, TEXT("Failed to save Static Mesh!"));
								}
							*/	
							}
							
						}
					}



					return FReply::Handled();
					}))

					[
						SNew(STextBlock)
						.Text(FText::FromString(C2MText))
					]

			]
		];
}

void FC2M_v2Module::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(C2M_v2TabName);
}

void FC2M_v2Module::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FC2M_v2Commands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FC2M_v2Commands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FC2M_v2Module, C2M_v2)