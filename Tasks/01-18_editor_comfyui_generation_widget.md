# Task 01-18 - Editor Utility Widget ComfyUI 이미지 생성

## 설명
Editor Utility Widget에서 positive prompt, 이미지 크기, workflow 파일 경로, Unreal Content 저장 폴더를 설정하고, 버튼 입력으로 ComfyUI 이미지 생성 요청을 시작할 수 있게 한다. 생성 요청의 진행 상태와 성공, 실패 결과는 위젯에서 확인할 수 있어야 한다.

## 구현 항목
- [x] Editor Utility Widget에서 positive prompt, width, height, workflow 파일 경로, Unreal Content 저장 폴더를 입력할 수 있다.
- [x] `이미지 만들기` 버튼을 누르면 현재 위젯 입력값으로 ComfyUI 생성 요청을 시작한다.
- [x] 생성 중, 성공, 실패 상태를 위젯에서 확인할 수 있다.
- [x] 생성 결과 파일을 Unreal Content 저장 폴더 기준으로 저장할 수 있다.

## 범위 밖
- 생성 결과를 Texture asset으로 import하는 단계는 다루지 않는다.
- 생성 결과 Texture asset을 게임 플레이 UI나 머티리얼에 연결하는 단계는 다루지 않는다.

## 사전 전제
- 01-16에서 Unreal Editor가 로컬 ComfyUI `/prompt` 요청을 보낼 수 있다.
- 01-17에서 ComfyUI workflow 요청 전 positive prompt와 이미지 크기 입력값을 치환할 수 있다.

## 수동 작업 (구현 후 구체화)
- Unreal Editor에서 Editor Utility Widget 자산을 새로 만들고 이름은 `WBP_ComfyUIImageGenerationWidget`처럼 `WBP_` 접두어로 지정한다.
- 위젯에 positive prompt, width, height, workflow 파일 경로, Unreal Content 저장 폴더를 입력할 UI를 만든다.
- 위젯 변수로 `UComfyUIImageGenerationWidgetController` Object를 만들고, 각 UI 입력값을 컨트롤러의 `PositivePrompt`, `ImageWidth`, `ImageHeight`, `WorkflowJsonFilePath`, `ContentSaveFolder`에 연결한다.
- `이미지 만들기` 버튼 클릭 이벤트에서 컨트롤러의 `CreateImage`를 호출한다.
- 상태 표시 UI가 컨트롤러의 `GenerationState`, `LastStatusMessage`, `LastSavedImagePath`를 보여주도록 연결한다.

## 완료 조건

### 에이전트 확인
- [x] 관련 코드 수정 완료
- [x] 로컬 정적 점검 또는 프로젝트 구조 기준 확인 완료
- [x] Unreal C++/Blueprint/에셋 규칙 위반 없음
- [x] 현재 task 문서가 실제 구현 기준으로 갱신됨

### 결과 확인 (구현 후 구체화)
- Unreal Editor를 실행하고 `WBP_ComfyUIImageGenerationWidget`을 연다.
- 위젯에서 positive prompt, width, height, workflow 파일 경로, Unreal Content 저장 폴더를 입력한다.
- 로컬 ComfyUI가 켜진 상태에서 `이미지 만들기` 버튼을 누른다.
- 버튼 입력 후 상태 표시가 생성 중 상태와 메시지를 보여주는지 확인한다.
- 생성이 완료되면 성공 상태와 저장된 이미지 파일 경로가 표시되는지 확인한다.
- 지정한 Content 하위 폴더에 생성 이미지 파일이 저장됐는지 확인한다.
- ComfyUI 서버가 꺼져 있거나 workflow 경로가 잘못됐을 때 실패 상태와 실패 메시지가 표시되는지 확인한다.
