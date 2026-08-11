# Task 01-19 - 생성 이미지 Texture2D Import

## 설명
ComfyUI가 만든 이미지 데이터를 지정한 Unreal Content 폴더에 Texture2D `.uasset`으로 import할 수 있게 한다. import 대상 폴더의 기본 예시는 `/Game/GeneratedTextures`로 두며, 생성되는 asset 이름은 중복되지 않아야 하고 Texture 기본 설정을 적용할 수 있어야 한다. ComfyUI 결과 원본 이미지 파일은 Content 폴더에 별도로 남기지 않는다.

## 구현 항목
- [x] ComfyUI 생성 결과 이미지를 Unreal에서 사용할 수 있는 Texture2D asset으로 import할 수 있다.
- [x] import 대상 Unreal Content 폴더를 지정할 수 있으며 기본 예시는 `/Game/GeneratedTextures`로 둔다.
- [x] Texture2D asset 이름은 기존 asset과 충돌하지 않게 자동 생성하거나 입력값 기반 prefix를 사용할 수 있다.
- [x] import 후 sRGB, compression, mip 설정 같은 Texture 기본 설정을 적용할 수 있다.
- [x] import 성공과 실패 결과를 사용자가 확인할 수 있다.

## 범위 밖
- Texture asset을 머티리얼, UI, 게임 플레이 오브젝트에 자동 연결하는 단계는 다루지 않는다.
- ComfyUI workflow 실행 방식 자체의 확장은 다루지 않는다.
- 생성 이미지의 후처리나 리사이즈는 다루지 않는다.

## 사전 전제
- 01-18에서 Editor Utility Widget이 ComfyUI 생성 요청을 실행하고 결과 이미지를 다운로드할 수 있다.

## 수동 작업 (구현 후 구체화)
- Unreal Editor에서 `WBP_ComfyUIImageGenerationWidget`을 연다.
- 위젯의 Content 저장 폴더 입력값을 컨트롤러의 `ContentSaveFolder`에 연결하고, 기본값은 `/Game/GeneratedTextures`로 둔다.
- 필요한 경우 위젯 입력값을 컨트롤러의 `TextureAssetNamePrefix`, `bImportedTextureSRGB`, `ImportedTextureCompressionSettings`, `ImportedTextureMipGenSettings`에 연결한다.
- `이미지 만들기` 버튼 클릭 이벤트가 컨트롤러의 `CreateImage`를 호출하도록 유지한다.
- 별도 이미지 파일 경로를 직접 import하는 UI가 필요하면 해당 경로 문자열을 컨트롤러의 `ImportImageFileAsTexture`에 연결한다.
- 상태 표시 UI가 컨트롤러의 `LastStatusMessage`와 `LastImportedTextureAssetPath`를 보여주도록 연결한다.

## 완료 조건

### 에이전트 확인
- [x] 관련 코드 수정 완료
- [x] 로컬 정적 점검 또는 프로젝트 구조 기준 확인 완료
- [x] Unreal C++/Blueprint/에셋 규칙 위반 없음
- [x] 현재 task 문서가 실제 구현 기준으로 갱신됨

### 결과 확인 (구현 후 구체화)
- Unreal Editor를 실행하고 `WBP_ComfyUIImageGenerationWidget`을 연다.
- Content 저장 폴더 입력값이 `/Game/GeneratedTextures` 또는 원하는 `/Game` 하위 폴더로 설정되어 있는지 확인한다.
- 위젯에서 ComfyUI 이미지를 생성하고, 생성 완료 후 별도 버튼 입력 없이 `LastStatusMessage`에 Texture2D import 성공 메시지가 표시되는지 확인한다.
- `LastImportedTextureAssetPath`에 생성된 Texture2D asset 경로가 표시되는지 확인한다.
- Content Browser에서 import 대상 폴더를 열어 Texture2D `.uasset`이 생성됐는지 확인한다.
- Content Browser에서 import 대상 폴더에 원본 `.png`, `.jpg` 같은 이미지 파일 asset이 별도로 생성되지 않았는지 확인한다.
- 같은 이미지 파일을 한 번 더 import했을 때 기존 asset을 덮어쓰지 않고 중복되지 않는 이름으로 새 asset이 생성되는지 확인한다.
- 생성된 Texture asset을 열어 sRGB, compression, mip 설정이 위젯 또는 컨트롤러 설정값대로 적용됐는지 확인한다.
- 잘못된 이미지 파일 경로나 `/Game` 밖의 import 폴더를 지정했을 때 실패 메시지가 표시되는지 확인한다.
