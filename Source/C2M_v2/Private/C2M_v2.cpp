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
#include "tga.h"

//File picker includes
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Runtime/Rawmesh/Public/RawMesh.h"
#include <IMeshReductionInterfaces.h>
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include <Factories/TextureFactory.h>
#include <Misc/FileHelper.h>


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

bool OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, TArray<FString>& OutFileNames) {
	uint32 SelectionFlag = 0; //A value of 0 represents single file selection while a value of 1 represents multiple file selection
	IMainFrameModule& MainFrameModule = IMainFrameModule::Get();
	TSharedPtr<SWindow> MainWindow = MainFrameModule.GetParentWindow();
	if (MainWindow.IsValid() && MainWindow->GetNativeWindow().IsValid()) {
		void* ParentWindowHandle = MainWindow->GetNativeWindow()->GetOSWindowHandle();
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
		bool result = DesktopPlatform->OpenFileDialog(ParentWindowHandle, DialogTitle, DefaultPath, FString(""), FileTypes, SelectionFlag, OutFileNames);
		return result;
	}
	return false;
};

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
					.OnClicked(FOnClicked::CreateLambda([]() {
					UE_LOG(LogTemp, Warning, TEXT("Button Clicked!"));

					// Open a file dialog
					FString path;

					//Opening the file picker!
					const FString& DialogTitle = TEXT("Select a C2M File");
					const FString& DefaultPath = TEXT("");
					// only allow .c2m files
					const FString& FileTypes = TEXT("C2M Files (*.c2m)|*.c2m");
					TArray<FString> OutFileName;
					bool result = OpenFileDialog(DialogTitle, DefaultPath, FileTypes, OutFileName);
					IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

					if (result) {
						path = OutFileName[0];
						UE_LOG(LogTemp, Warning, TEXT("File path: %s"), *path);
						std::string ewpath = TCHAR_TO_UTF8(*path);

						auto map = CodMap(ewpath);
						auto name_char = UTF8_TO_TCHAR(map.Name.c_str());
						UE_LOG(LogTemp, Warning, TEXT("Map Name: %s"), name_char);
						UE_LOG(LogTemp, Warning, TEXT("Mesh Name: %s"), *FString(UTF8_TO_TCHAR(map.meshes[0].Name.c_str())));
						for (auto mesh: map.meshes) {
							FString ModelNamePackage = TEXT("/Game/") + (FString(UTF8_TO_TCHAR(map.Name.c_str()))) + TEXT("/") + (FString(UTF8_TO_TCHAR(mesh.Name.c_str())));
							UE_LOG(LogTemp, Warning, TEXT("New Package Name: %s"), *ModelNamePackage);
							UPackage* Package = CreatePackage(*ModelNamePackage);
							FName meshName = FName(*FString(UTF8_TO_TCHAR(mesh.Name.c_str())));
							auto StaticMesh = NewObject<UStaticMesh>(Package, meshName, RF_Public | RF_Standalone);
							Package->FullyLoad();
							Package->Modify();
							FRawMesh rawMesh;
							TArray<FVector> Vertices;
							rawMesh.VertexPositions.AddZeroed(mesh.Vertices.size());
							for (int i = 0; i < mesh.Vertices.size(); i++) {
								FVector vertex = FVector(mesh.Vertices[i].X, mesh.Vertices[i].Y, mesh.Vertices[i].Z);
								rawMesh.VertexPositions[i] = FVector3f(vertex);
								Vertices.Add(vertex);
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
									rawMesh.FaceMaterialIndices.Add(j);
									rawMesh.FaceSmoothingMasks.Add(0);
								}
							}
							FStaticMeshSourceModel SrcModel{};
							SrcModel.CreateSubObjects(StaticMesh);
							SrcModel.BuildSettings.bRecomputeNormals = false;
							SrcModel.BuildSettings.bRecomputeTangents = false;
							SrcModel.BuildSettings.bRemoveDegenerates = true;
							SrcModel.ReductionSettings.bRecalculateNormals = false;
							if (!rawMesh.IsValid()) {
								UE_LOG(LogTemp, Warning, TEXT("RawMesh is not valid!"));
								return FReply::Handled();
							}
							TArray<FStaticMeshSourceModel> temp{};
							SrcModel.SaveRawMesh(rawMesh);

							temp.Add(MoveTemp(SrcModel));

							StaticMesh->SetSourceModels(MoveTemp(temp));


							// Do the materials
							

							StaticMesh->Build(false);

							StaticMesh->MarkPackageDirty();
							FAssetRegistryModule::AssetCreated(StaticMesh);

							StaticMesh->PostEditChange();

							Package->SetDirtyFlag(true);

							FString PackageFileName = FPackageName::LongPackageNameToFilename(ModelNamePackage, FPackageName::GetAssetPackageExtension());

							FSavePackageArgs SaveArgs;
							SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
							SaveArgs.Error = GError;
							SaveArgs.bWarnOfLongFilename = false;

							if (UPackage::SavePackage(Package, StaticMesh, *ModelNamePackage, SaveArgs)) {
								UE_LOG(LogTemp, Warning, TEXT("Saved Static Mesh!"));
							}
							else {
								UE_LOG(LogTemp, Warning, TEXT("Failed to save Static Mesh!"));
							}

						}


						
						auto mesh = map.meshes[0];
						FString PackageName = TEXT("/Game/") + (FString(UTF8_TO_TCHAR("C2M/"))) + TEXT("Textures/");
						UPackage* CommonPackage = CreatePackage(*PackageName);
						CommonPackage->FullyLoad();
						CommonPackage->Modify();
						for (auto mat : map.Materials) {
							for (auto texture : mat.second.Textures) {
								try {
									auto tex = texture.Name;
									if (tex == "$black" || tex == "identitynormal")
										continue;
									FString lHalf;
									FString rHalf;
									path.Split(TEXT("exported"), &lHalf, &rHalf);
									UE_LOG(LogTemp, Warning, TEXT("Left Half: %s"), *lHalf);
									UE_LOG(LogTemp, Warning, TEXT("Right Half: %s"), *rHalf);
									FString images_path = lHalf + TEXT("exported_images/") + (FString(UTF8_TO_TCHAR(map.Version.c_str()))) + TEXT("/");
									FString texture_path = images_path + (FString(UTF8_TO_TCHAR(tex.c_str()))) + FString(".tga");
									UE_LOG(LogTemp, Warning, TEXT("Texture Path: %s"), *texture_path);

									// full path is map path up 2 irectories + exported_images + version + texture name



									// Create a new texture
									FString TextureName = (FString(UTF8_TO_TCHAR(tex.c_str())));
									TextureName = TextureName.Replace(TEXT("*"), TEXT("X"));
									TextureName = TextureName.Replace(TEXT("?"), TEXT("Q"));
									TextureName = TextureName.Replace(TEXT("!"), TEXT("I"));
									TextureName = TextureName.Replace(TEXT(":"), TEXT("_"));
									TextureName = TextureName.Replace(TEXT("."), TEXT("-"));
									TextureName = TextureName.Replace(TEXT("&"), TEXT("_"));
									TextureName = TextureName.Replace(TEXT(" "), TEXT("_"));
									TextureName = TextureName.Replace(TEXT("~"), TEXT("_"));

									


									FString TexturePackageName = TEXT("/Game/") + (FString(UTF8_TO_TCHAR("C2M/"))) + TextureName;
									UPackage* TexturePackage = CreatePackage(*TexturePackageName);
									TexturePackage->FullyLoad();
									TexturePackage->Modify();



									// Load the texture
									UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
									UTexture2D* Texture;
									Texture = TextureFactory->CreateTexture2D(TexturePackage, FName(TextureName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

									// Load the texture data from tga file
									TArray<uint8> RawFileData;
									TCHAR* Name = TextureName.GetCharArray().GetData();

									if (!PlatformFile.FileExists(*texture_path)) {
										UE_LOG(LogTemp, Warning, TEXT("File does not exist!"));
										break;
									}

									FILE* f = fopen(TCHAR_TO_ANSI(*texture_path), "rb");
									if (!f) {
										UE_LOG(LogTemp, Warning, TEXT("Failed to open file! %s"), TCHAR_TO_ANSI(*texture_path));
										break;
									}
									

									tga::StdioFileInterface file(f);
									tga::Decoder decoder(&file);

									tga::Header header;

									if (!decoder.readHeader(header)) {
										UE_LOG(LogTemp, Warning, TEXT("Failed to read header!"));
										break;
									}

							
									tga::Image image;
									image.bytesPerPixel = header.bytesPerPixel();
									image.rowstride = header.width * header.bytesPerPixel();
									TArray<uint8> Data;
									Data.SetNum(image.rowstride * header.height);
									image.pixels = &Data[0];
									if (!decoder.readImage(header, image, nullptr))
									{
										UE_LOG(LogTemp, Warning, TEXT("Failed to read image!"));
										break;
									}
									decoder.postProcessImage(header, image);
									Texture->PlatformData = new FTexturePlatformData();
									Texture->PlatformData->SizeX = header.width;
									Texture->PlatformData->SizeY = header.height;
									Texture->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;	
									Texture->PlatformData->Mips.Add(new FTexture2DMipMap());

									FTexture2DMipMap* MipMap = &Texture->PlatformData->Mips[0];
									MipMap->SizeZ = 1;
									MipMap->SizeX = header.width;
									MipMap->SizeY = header.height;
									FByteBulkData* ImageData = &MipMap->BulkData;
									ImageData->Lock(LOCK_READ_WRITE);
									auto image_data = ImageData->Realloc(image.rowstride * header.height);

									// Flip the colors
									for (int i = 0; i < header.height; i++)
									{
										for (int j = 0; j < header.width; j++)
										{
											// Swap red and blue channels
											uint8* pixel = &image.pixels[(i * header.width + j) * 4];
											uint8 temp = pixel[0];
											pixel[0] = pixel[2];
											pixel[2] = temp;
										}
									}

									FMemory::Memcpy(image_data, Data.GetData(),(uint64)image.rowstride * header.height);

									
									ImageData->Unlock();
									fclose(f);





									TexturePackage->MarkPackageDirty();
									FAssetRegistryModule::AssetCreated(Texture);
									Texture->PostEditChange();
									TexturePackage->SetDirtyFlag(true);

									FString PackageFileName = FPackageName::LongPackageNameToFilename(TexturePackageName, FPackageName::GetAssetPackageExtension());

									FSavePackageArgs SaveArgs;
									SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
									SaveArgs.Error = GError;
									SaveArgs.bWarnOfLongFilename = false;
									SaveArgs.SaveFlags = SAVE_None;

									if (UPackage::SavePackage(TexturePackage, Texture, *TexturePackageName, SaveArgs)) {
										UE_LOG(LogTemp, Warning, TEXT("Saved Texture!"));
									}
									else {
										UE_LOG(LogTemp, Warning, TEXT("Failed to save Texture!"));
									}
								}
								catch (...) {
									UE_LOG(LogTemp, Error, TEXT("Something went catastrophically wrong trying to save an unreal package, In unreal code."));
								}
							}
						}



						FString NewPackageName = TEXT("/Game/") + (FString(UTF8_TO_TCHAR(map.Name.c_str()))) + TEXT("/") + (FString(UTF8_TO_TCHAR(mesh.Name.c_str())));
						UE_LOG(LogTemp, Warning, TEXT("New Package Name: %s"), *NewPackageName);
						UPackage* Package = CreatePackage(*NewPackageName);
						Package->FullyLoad();
						Package->Modify();
						FRawMesh rawMesh;
						TArray<FVector> Vertices;
						rawMesh.VertexPositions.AddZeroed(mesh.Vertices.size());
						for (int i = 0; i < mesh.Vertices.size(); i++) {
							FVector vertex = FVector(mesh.Vertices[i].X, mesh.Vertices[i].Y, mesh.Vertices[i].Z);
							rawMesh.VertexPositions[i] = FVector3f(vertex);
							Vertices.Add(vertex);
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
								rawMesh.FaceMaterialIndices.Add(0);
								rawMesh.FaceSmoothingMasks.Add(0);
							}
						}


						TArray<FStaticMeshSourceModel> temp{};
						FName meshName = FName(*FString(UTF8_TO_TCHAR(mesh.Name.c_str())));
						FlushRenderingCommands();
						UE_LOG(LogTemp, Warning, TEXT("Mesh Name: %s"), *meshName.ToString());
						FStaticMeshComponentRecreateRenderStateContext RecreateRenderStateContext(FindObject<UStaticMesh>(Package, *meshName.ToString()));

						auto StaticMesh = NewObject<UStaticMesh>(Package, meshName, RF_Public | RF_Standalone);

						StaticMesh->SetLightingGuid(FGuid::NewGuid());
						StaticMesh->SetLightMapCoordinateIndex(1);
						StaticMesh->SetLightMapResolution(32);

						// Add one LOD for the base mesh
						FStaticMeshSourceModel SrcModel{};
						SrcModel.CreateSubObjects(StaticMesh);
						SrcModel.BuildSettings.bRecomputeNormals = false;
						SrcModel.BuildSettings.bRecomputeTangents = false;
						SrcModel.BuildSettings.bRemoveDegenerates = true;
						SrcModel.ReductionSettings.bRecalculateNormals = false;
						if (!rawMesh.IsValid()) {
							UE_LOG(LogTemp, Warning, TEXT("RawMesh is not valid!"));
							return FReply::Handled();
						}
						SrcModel.SaveRawMesh(rawMesh);

						temp.Add(MoveTemp(SrcModel));

						StaticMesh->SetSourceModels(MoveTemp(temp));

						StaticMesh->Build(false);
						
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


						}
					else {
						UE_LOG(LogTemp, Warning, TEXT("File picker failed!"));
					}
					return FReply::Handled();
					}))

					[
						SNew(STextBlock)
						.Text(FText::FromString(C2MText))
					]
					[
						SNew(SBox)
						.Padding(10)
					]
					[
						SNew(SButton)
						.Text(FText::FromString("Test Button 2"))
						.OnClicked(FOnClicked::CreateLambda([]() {
						UE_LOG(LogTemp, Warning, TEXT("Button 2 Clicked!"));


							// Place the models and shit into the map

							return FReply::Handled();
							}))
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