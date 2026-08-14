// Copyright Epic Games, Inc. All Rights Reserved.

#include "ComfyUITextureImportService.h"

#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace ComfyUITextureImportService
{
	constexpr float HistoryPollIntervalSeconds = 0.5f;
	constexpr double GenerationTimeoutSeconds = 300.0;
	constexpr const TCHAR* GeneratedTexturePath = TEXT("/Game/GeneratedTextures");
	constexpr const TCHAR* GeneratedTextureBaseName = TEXT("T_ComfyUI");

	struct FComfyUIImageDescriptor
	{
		FString Filename;
		FString Subfolder;
		FString Type;
	};

	// 생성 완료 조회부터 다운로드와 임포트까지 필요한 비동기 상태를 유지한다.
	class FImportState : public TSharedFromThis<FImportState, ESPMode::ThreadSafe>
	{
	public:
		// 요청 설정과 완료 델리게이트를 생성 완료 시점까지 보관한다.
		FImportState(
			const FString& InServerBaseUrl,
			const FString& InPromptId,
			float InRequestTimeoutSeconds,
			FOnComfyUITextureImportComplete&& InCompletion)
			: ServerBaseUrl(InServerBaseUrl)
			, PromptId(InPromptId)
			, RequestTimeoutSeconds(FMath::Max(1.0f, InRequestTimeoutSeconds))
			, Completion(MoveTemp(InCompletion))
		{
			ServerBaseUrl.TrimStartAndEndInline();
			ServerBaseUrl.RemoveFromEnd(TEXT("/"));
		}

		// 입력값을 검증하고 첫 생성 완료 조회를 시작한다.
		void Start()
		{
			if (ServerBaseUrl.IsEmpty())
			{
				Fail(TEXT("ComfyUI 서버 주소가 비어 있습니다."));
				return;
			}
			if (PromptId.IsEmpty())
			{
				Fail(TEXT("ComfyUI 생성 결과를 조회할 prompt_id가 비어 있습니다."));
				return;
			}

			GenerationStartedAt = FPlatformTime::Seconds();
			RequestHistory();
		}

	private:
		// prompt_id에 해당하는 ComfyUI history를 요청한다.
		void RequestHistory()
		{
			if (HasGenerationTimedOut())
			{
				Fail(TEXT("ComfyUI 이미지 생성 완료를 300초 안에 확인하지 못했습니다."));
				return;
			}

			const FString HistoryUrl = BuildComfyUrl(TEXT("/history/") + FGenericPlatformHttp::UrlEncode(PromptId));
			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
			Request->SetURL(HistoryUrl);
			Request->SetVerb(TEXT("GET"));
			Request->SetTimeout(RequestTimeoutSeconds);
			Request->OnProcessRequestComplete().BindLambda(
				[Self = AsShared()](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
				{
					Self->HandleHistoryResponse(Response, bWasSuccessful);
				});

			if (!Request->ProcessRequest())
			{
				Fail(TEXT("ComfyUI 생성 완료 조회 요청을 시작하지 못했습니다."));
			}
		}

		// history 응답에서 완료 여부와 다운로드할 이미지 목록을 확인한다.
		void HandleHistoryResponse(const FHttpResponsePtr& Response, bool bWasSuccessful)
		{
			if (!bWasSuccessful || !Response.IsValid())
			{
				Fail(TEXT("ComfyUI 생성 완료 조회 응답을 받지 못했습니다."));
				return;
			}

			Result.HttpStatusCode = Response->GetResponseCode();
			if (Result.HttpStatusCode < 200 || Result.HttpStatusCode >= 300)
			{
				Fail(FString::Printf(TEXT("ComfyUI 생성 완료 조회가 HTTP %d로 실패했습니다: %s"), Result.HttpStatusCode, *Response->GetContentAsString()));
				return;
			}

			TSharedPtr<FJsonObject> HistoryRoot;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, HistoryRoot) || !HistoryRoot.IsValid())
			{
				Fail(TEXT("ComfyUI history 응답이 올바른 JSON 객체가 아닙니다."));
				return;
			}

			if (!HistoryRoot->HasTypedField<EJson::Object>(PromptId))
			{
				ScheduleHistoryPoll();
				return;
			}

			const TSharedPtr<FJsonObject> HistoryEntry = HistoryRoot->GetObjectField(PromptId);
			bool bCompleted = false;
			bool bHasStatus = false;
			FString StatusText;
			if (HistoryEntry->HasTypedField<EJson::Object>(TEXT("status")))
			{
				bHasStatus = true;
				const TSharedPtr<FJsonObject> StatusObject = HistoryEntry->GetObjectField(TEXT("status"));
				StatusObject->TryGetBoolField(TEXT("completed"), bCompleted);
				StatusObject->TryGetStringField(TEXT("status_str"), StatusText);
			}

			if (StatusText.Equals(TEXT("error"), ESearchCase::IgnoreCase))
			{
				Fail(TEXT("ComfyUI가 이미지 생성 작업을 실패 상태로 종료했습니다."));
				return;
			}

			ExtractImages(HistoryEntry);
			if (bHasStatus && !bCompleted)
			{
				ScheduleHistoryPoll();
				return;
			}
			if (Images.IsEmpty())
			{
				if (bCompleted)
				{
					Fail(TEXT("ComfyUI 생성 작업은 완료됐지만 history에서 결과 이미지를 찾지 못했습니다."));
				}
				else
				{
					ScheduleHistoryPoll();
				}
				return;
			}

			DownloadNextImage();
		}

		// history 결과의 모든 images 항목을 다운로드 목록으로 변환한다.
		void ExtractImages(const TSharedPtr<FJsonObject>& HistoryEntry)
		{
			Images.Reset();
			if (!HistoryEntry->HasTypedField<EJson::Object>(TEXT("outputs")))
			{
				return;
			}

			const TSharedPtr<FJsonObject> Outputs = HistoryEntry->GetObjectField(TEXT("outputs"));
			for (const TPair<FString, TSharedPtr<FJsonValue>>& OutputPair : Outputs->Values)
			{
				const TSharedPtr<FJsonObject> OutputObject = OutputPair.Value.IsValid() ? OutputPair.Value->AsObject() : nullptr;
				const TArray<TSharedPtr<FJsonValue>>* ImageValues = nullptr;
				if (!OutputObject.IsValid() || !OutputObject->TryGetArrayField(TEXT("images"), ImageValues))
				{
					continue;
				}

				for (const TSharedPtr<FJsonValue>& ImageValue : *ImageValues)
				{
					const TSharedPtr<FJsonObject> ImageObject = ImageValue.IsValid() ? ImageValue->AsObject() : nullptr;
					FComfyUIImageDescriptor Descriptor;
					if (!ImageObject.IsValid() || !ImageObject->TryGetStringField(TEXT("filename"), Descriptor.Filename) || Descriptor.Filename.IsEmpty())
					{
						continue;
					}

					ImageObject->TryGetStringField(TEXT("subfolder"), Descriptor.Subfolder);
					if (!ImageObject->TryGetStringField(TEXT("type"), Descriptor.Type) || Descriptor.Type.IsEmpty())
					{
						Descriptor.Type = TEXT("output");
					}
					Images.Add(MoveTemp(Descriptor));
				}
			}
		}

		// 짧은 간격 뒤 history를 다시 조회하도록 예약한다.
		void ScheduleHistoryPoll()
		{
			if (HasGenerationTimedOut())
			{
				Fail(TEXT("ComfyUI 이미지 생성 완료를 300초 안에 확인하지 못했습니다."));
				return;
			}

			(void)FTSTicker::GetCoreTicker().AddTicker(
				TEXT("ComfyUITextureImportHistoryPoll"),
				HistoryPollIntervalSeconds,
				[Self = AsShared()](float)
				{
					Self->RequestHistory();
					return false;
				});
		}

		// 현재 다운로드 대상 이미지를 요청하거나 모든 임포트를 완료한다.
		void DownloadNextImage()
		{
			if (!Images.IsValidIndex(CurrentImageIndex))
			{
				Succeed();
				return;
			}

			const FComfyUIImageDescriptor& Image = Images[CurrentImageIndex];
			const FString ViewQuery = FString::Printf(
				TEXT("/view?filename=%s&subfolder=%s&type=%s"),
				*FGenericPlatformHttp::UrlEncode(Image.Filename),
				*FGenericPlatformHttp::UrlEncode(Image.Subfolder),
				*FGenericPlatformHttp::UrlEncode(Image.Type));

			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
			Request->SetURL(BuildComfyUrl(ViewQuery));
			Request->SetVerb(TEXT("GET"));
			Request->SetTimeout(RequestTimeoutSeconds);
			Request->OnProcessRequestComplete().BindLambda(
				[Self = AsShared()](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
				{
					Self->HandleImageResponse(Response, bWasSuccessful);
				});

			if (!Request->ProcessRequest())
			{
				Fail(FString::Printf(TEXT("ComfyUI 결과 이미지 다운로드 요청을 시작하지 못했습니다: %s"), *Image.Filename));
			}
		}

		// 다운로드한 이미지 데이터를 임시 원본 파일로 저장하고 Texture2D로 임포트한다.
		void HandleImageResponse(const FHttpResponsePtr& Response, bool bWasSuccessful)
		{
			const FComfyUIImageDescriptor& Image = Images[CurrentImageIndex];
			if (!bWasSuccessful || !Response.IsValid())
			{
				Fail(FString::Printf(TEXT("ComfyUI 결과 이미지 응답을 받지 못했습니다: %s"), *Image.Filename));
				return;
			}

			Result.HttpStatusCode = Response->GetResponseCode();
			if (Result.HttpStatusCode < 200 || Result.HttpStatusCode >= 300 || Response->GetContent().IsEmpty())
			{
				Fail(FString::Printf(TEXT("ComfyUI 결과 이미지 다운로드가 HTTP %d로 실패했습니다: %s"), Result.HttpStatusCode, *Image.Filename));
				return;
			}

			FString UniquePackageName;
			FString UniqueAssetName;
			FAssetToolsModule::GetModule().Get().CreateUniqueAssetName(
				FString(GeneratedTexturePath) + TEXT("/") + GeneratedTextureBaseName,
				TEXT(""),
				UniquePackageName,
				UniqueAssetName);

			const FString DownloadDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ComfyUIDownloads"));
			if (!IFileManager::Get().MakeDirectory(*DownloadDirectory, true) && !IFileManager::Get().DirectoryExists(*DownloadDirectory))
			{
				Fail(FString::Printf(TEXT("ComfyUI 이미지 원본 저장 폴더를 만들지 못했습니다: %s"), *DownloadDirectory));
				return;
			}

			FString Extension = FPaths::GetExtension(Image.Filename, false).ToLower();
			if (Extension.IsEmpty())
			{
				Extension = TEXT("png");
			}
			const FString SourceFilePath = FPaths::Combine(DownloadDirectory, UniqueAssetName + TEXT(".") + Extension);
			if (!FFileHelper::SaveArrayToFile(Response->GetContent(), *SourceFilePath))
			{
				Fail(FString::Printf(TEXT("ComfyUI 이미지 원본을 저장하지 못했습니다: %s"), *SourceFilePath));
				return;
			}

			UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
			ImportTask->Filename = SourceFilePath;
			ImportTask->DestinationPath = FPackageName::GetLongPackagePath(UniquePackageName);
			ImportTask->DestinationName = UniqueAssetName;
			ImportTask->bAutomated = true;
			ImportTask->bSave = true;
			ImportTask->bReplaceExisting = false;
			ImportTask->bReplaceExistingSettings = false;
			ImportTask->bAsync = false;

			TArray<UAssetImportTask*> ImportTasks;
			ImportTasks.Add(ImportTask);
			FAssetToolsModule::GetModule().Get().ImportAssetTasks(ImportTasks);

			UTexture2D* ImportedTexture = nullptr;
			for (UObject* ImportedObject : ImportTask->GetObjects())
			{
				ImportedTexture = Cast<UTexture2D>(ImportedObject);
				if (ImportedTexture)
				{
					break;
				}
			}

			if (!ImportedTexture)
			{
				Fail(FString::Printf(TEXT("다운로드한 이미지를 Texture2D로 임포트하지 못했습니다: %s"), *SourceFilePath));
				return;
			}

			Result.ImportedTexturePaths.Add(ImportedTexture->GetPathName());
			UE_LOG(LogTemp, Display, TEXT("ComfyUI Texture2D 임포트 완료: %s"), *ImportedTexture->GetPathName());
			++CurrentImageIndex;
			DownloadNextImage();
		}

		// 서버 기본 주소와 ComfyUI 엔드포인트를 결합한다.
		FString BuildComfyUrl(const FString& EndpointPath) const
		{
			return ServerBaseUrl + EndpointPath;
		}

		// 생성 완료 대기 제한 시간을 지났는지 확인한다.
		bool HasGenerationTimedOut() const
		{
			return FPlatformTime::Seconds() - GenerationStartedAt >= GenerationTimeoutSeconds;
		}

		// 모든 Texture2D 경로를 결과 메시지로 전달한다.
		void Succeed()
		{
			Result.bSucceeded = true;
			Result.ResponseMessage = FString::Printf(
				TEXT("ComfyUI 이미지 %d개를 Texture2D로 임포트했습니다: %s"),
				Result.ImportedTexturePaths.Num(),
				*FString::Join(Result.ImportedTexturePaths, TEXT(", ")));
			UE_LOG(LogTemp, Display, TEXT("%s"), *Result.ResponseMessage);
			Complete();
		}

		// 실패 상태와 사유를 결과에 기록한다.
		void Fail(const FString& Message)
		{
			Result.bSucceeded = false;
			Result.ResponseMessage = Message;
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
			Complete();
		}

		// 완료 델리게이트를 한 번만 실행한다.
		void Complete()
		{
			if (Completion.IsBound())
			{
				FOnComfyUITextureImportComplete LocalCompletion = MoveTemp(Completion);
				LocalCompletion.Execute(Result);
			}
		}

		FString ServerBaseUrl;
		FString PromptId;
		float RequestTimeoutSeconds = 30.0f;
		double GenerationStartedAt = 0.0;
		TArray<FComfyUIImageDescriptor> Images;
		int32 CurrentImageIndex = 0;
		FComfyUITextureImportResult Result;
		FOnComfyUITextureImportComplete Completion;
	};
}

void FComfyUITextureImportService::ImportGeneratedTextures(
	const FString& ServerBaseUrl,
	const FString& PromptId,
	float RequestTimeoutSeconds,
	FOnComfyUITextureImportComplete Completion)
{
	MakeShared<ComfyUITextureImportService::FImportState, ESPMode::ThreadSafe>(
		ServerBaseUrl,
		PromptId,
		RequestTimeoutSeconds,
		MoveTemp(Completion))->Start();
}
