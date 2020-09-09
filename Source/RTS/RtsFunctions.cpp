#include "RTS.h"
//#include "ImageWrapper.h"
//Runtime/ImageWrapper/Public/IImageWrapperModule.h #include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "RtsFunctions.h"

void URtsFunctions::WriteBinaryFile(TArray<uint8> data, FString file, bool & Success) {
	Success = FFileHelper::SaveArrayToFile(data, *(FPaths::ProjectDir() + file));
}

void URtsFunctions::ReadBinaryFile(FString file, TArray<uint8>& bytes, bool & Success) {
	Success = FFileHelper::LoadFileToArray(bytes, *(FPaths::ProjectDir() + file));
}

void URtsFunctions::WriteStringToFile(FString i,FString file,bool & Success) {
	Success = FFileHelper::SaveStringToFile(i, *(FPaths::ProjectDir() + file));
}

void URtsFunctions::ReadStringFromFile(FString file,FString &string) {
	FFileHelper::LoadFileToString(string, *(FPaths::ProjectDir() + file));
}

//Reading pngs
void URtsFunctions::ReadPNGRaw(FString file, TArray<uint8>& Data, int32 &Width, int32 &Height) {
	FString Path = FPaths::ProjectDir() + file;

	UTexture2D* LoadedT2D = NULL;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *Path))
	{
		return;
	}

	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		const TArray<uint8>* raw = NULL;
		if (ImageWrapper->GetRaw(ERGBFormat::Gray, 8, raw))
		{
			Width = ImageWrapper->GetWidth();
			Height = ImageWrapper->GetHeight();

			TArray<uint8>* reversed = new TArray<uint8>();
			reversed->SetNum(raw->Num());

			for (int r = 0; r < Height; r++)
				for (int c = 0; c < Width; c++)
					(*reversed)[(r * Width) + c] = (*raw)[(((Height - 1) - r) * Width) + c];

			Data = *reversed;
		}
	}
}

UTexture2D* URtsFunctions::ReadPNGFile(FString file) {
	FString Path = FPaths::ProjectDir() + file;

	if (!FPaths::FileExists(Path))return NULL;

	UTexture2D* LoadedT2D = NULL;

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *Path))
	{
		return NULL;
	}


	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		const TArray<uint8>* UncompressedBGRA = NULL;
		if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
		{
			LoadedT2D = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);

			if (!LoadedT2D)
			{
				return NULL;
			}

			LoadedT2D->Filter = TextureFilter::TF_Nearest;

			void* TextureData = LoadedT2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
			LoadedT2D->PlatformData->Mips[0].BulkData.Unlock();

			LoadedT2D->UpdateResource();
		}
	}

	return LoadedT2D;
}

//returns filenames in a directory
TArray<FString> URtsFunctions::FindAllFilesInDirectory(FString p,FString filename) {
	if (filename == "")
		filename = "*.*";
	else
		filename = (filename.Left(1) == ".") ? "*" + filename : "*." + filename;

	FString Path = FPaths::ProjectDir() + p + filename;
	
	IFileManager& FileManager = IFileManager::Get();

	TArray<FString> Files;
	FileManager.FindFiles(Files, *Path, true, false);
	return Files;
}

//
UTexture2D* URtsFunctions::ConstructRuntimeTexture2D(UTextureRenderTarget2D *target,int Width,int Height) {
	//return target->ConstructTexture2D(Outer,Name,RF_NoFlags,EConstructTextureFlags::CTF_SRGB);

	class UTexture2D* Result = UTexture2D::CreateTransient(
		Width,
		Height,
		PF_B8G8R8A8
	);

	FRenderTarget* RenderTarget = target->GameThread_GetRenderTargetResource();

	if (RenderTarget == NULL)
	{
		return Result;
	}

	// Lock the texture so it can be modified
	uint8* MipData = static_cast<uint8*>(Result->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	TArray<FColor> SurfData;
	RenderTarget->ReadPixels(SurfData);

	// Create base mip.
	uint8* DestPtr = NULL;
	const FColor* SrcPtr = NULL;
	for (int32 y = 0; y < Height; y++)
	{
		DestPtr = &MipData[(Height - 1 - y) * Width * sizeof(FColor)];
		SrcPtr = const_cast<FColor*>(&SurfData[(Height - 1 - y) * Width]);
		for (int32 x = 0; x < Width; x++)
		{
			*DestPtr++ = SrcPtr->B;
			*DestPtr++ = SrcPtr->G;
			*DestPtr++ = SrcPtr->R;
			*DestPtr++ = SrcPtr->A;
			SrcPtr++;
		}
	}

	// Unlock the texture
	Result->PlatformData->Mips[0].BulkData.Unlock();
	Result->UpdateResource();

	return Result;
}

//FINDING
UMaterial* URtsFunctions::FindMaterial(FString Path) {
	if (Path == "") return NULL;

	return Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), NULL, *Path));
}

UStaticMesh* URtsFunctions::FindStaticMesh(FString Path)
{
	if (Path == "") return NULL;

	return Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, *Path));
}

//CONVERSIONS
TArray<uint8> URtsFunctions::FloatToBytes(float f) {
	uint8* raw = (uint8*)&f;
	TArray<uint8> data;
	for (int i = 0; i < sizeof(float); i++)
		data.Add(raw[i]);
	return data;
}

float URtsFunctions::BytesToFloat(TArray<uint8> Bytes) {
	float f;
	memcpy(&f,Bytes.GetData(),sizeof(f));
	return f;
}