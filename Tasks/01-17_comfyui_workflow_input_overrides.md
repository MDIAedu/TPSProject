# Task 01-17 - ComfyUI workflow 입력값 치환 검증

## 설명
준비된 ComfyUI workflow 파일을 `/prompt` 요청에 사용하기 전에 Unreal 쪽 입력값으로 긍정 프롬프트와 이미지 크기 값을 치환할 수 있는지 검증한다. 치환 대상은 workflow 구조에서 자동으로 찾되, 필요하면 node id와 input key를 명시적으로 override할 수 있어야 한다. workflow 구조가 기대와 다를 때 어떤 node/key가 문제인지 에러 메시지로 확인할 수 있어야 한다.

## 구현 항목
- [x] Unreal Editor에서 긍정 프롬프트 입력값을 지정해 workflow의 대상 node/input 값으로 치환할 수 있다.
- [x] Unreal Editor에서 이미지 width, height 입력값을 지정해 workflow의 대상 node/input 값으로 치환할 수 있다.
- [x] 긍정 프롬프트 치환 대상을 workflow 구조에서 자동으로 찾고, 필요하면 node id와 input key로 override할 수 있다.
- [x] width, height 치환 대상을 workflow 구조에서 자동으로 찾고, 필요하면 node id와 input key로 override할 수 있다.
- [x] seed, negative prompt, batch count는 이번 task에서 직접 치환하지 않더라도 이후 추가할 수 있는 입력 대상 구조로 남겨 둔다.
- [x] workflow에 지정한 node id나 input key가 없을 때 어떤 node/key가 문제인지 확인할 수 있는 에러 메시지를 제공한다.

## 범위 밖
- ComfyUI workflow JSON 제작
- ComfyUI 서버 실행 또는 설치 자동화
- 생성 결과 이미지 다운로드
- Texture import
- Unreal Editor UI 제작
- 게임플레이 중 ComfyUI 요청
- seed, negative prompt, batch count 실제 치환 동작 구현
- 패키징 빌드에서의 ComfyUI 연동

## 사전 전제
- 01-16 단계의 Unreal Editor ComfyUI `/prompt` 요청 기능이 유지되어 있다.
- 로컬 ComfyUI 서버가 `http://127.0.0.1:8188`에서 실행 중이다.
- `/prompt` 요청에 사용할 workflow JSON 파일이 준비되어 있다.

## 수동 작업
- 로컬 ComfyUI 서버를 `http://127.0.0.1:8188`에서 실행한다.
- Unreal Editor에서 C++ 변경 사항을 컴파일한다.
- `ComfyUIPromptRequestTester`를 부모 클래스로 하는 Blueprint를 만들거나, `ComfyUIPromptRequestTester` C++ Actor를 레벨에 배치한다.
- 배치한 Actor의 `ComfyUI|Request` 값에서 `ServerBaseUrl`이 `http://127.0.0.1:8188`인지 확인한다.
- 배치한 Actor의 `ComfyUI|Request` 값에서 `WorkflowJsonFilePath`에 `/prompt` 요청에 사용할 workflow JSON 파일 경로를 지정한다.
- 배치한 Actor의 `ComfyUI|Overrides` 값에서 `PositivePrompt`에 요청에 사용할 긍정 프롬프트를 입력한다.
- 배치한 Actor의 `ComfyUI|Overrides` 값에서 `ImageWidth`와 `ImageHeight`에 요청할 이미지 크기를 입력한다.
- 자동 탐색 결과가 원하는 node가 아닐 때만 배치한 Actor의 `ComfyUI|Overrides` 값에서 `PositivePromptTarget`, `ImageWidthTarget`, `ImageHeightTarget`의 `NodeId`와 `InputKey`를 직접 입력한다.

## 완료 조건

### 에이전트 확인
- [x] 관련 코드 수정 완료
- [x] 로컬 정적 점검 또는 프로젝트 구조 기준 확인 완료
- [x] Unreal C++/Blueprint/에셋 규칙 위반 없음
- [x] 현재 task 문서가 실제 구현 기준으로 갱신됨

### 결과 확인
- [x] PIE를 실행하지 않은 상태에서 배치한 Actor를 선택한다.
- [x] Details 패널의 `ComfyUI` 항목에서 `SendPromptRequest`를 실행한다.
- [x] Output Log에 긍정 프롬프트, 이미지 width, 이미지 height 치환 대상을 자동으로 찾았다는 로그가 표시되는지 확인한다.
- [x] Output Log에 `ComfyUI /prompt 요청 성공` 로그가 표시되는지 확인한다.
- [x] 배치한 Actor의 `ComfyUI|Result` 값에서 `bLastRequestSucceeded`가 true이고 `LastHttpStatusCode`가 200번대인지 확인한다.
- [x] ComfyUI에서 요청된 workflow가 `PositivePrompt`, `ImageWidth`, `ImageHeight` 값으로 실행되는지 확인한다.
- [x] `PositivePromptTarget.InputKey`, `ImageWidthTarget.InputKey`, `ImageHeightTarget.InputKey` 중 하나를 workflow에 없는 값으로 직접 입력한 뒤 `SendPromptRequest`를 실행한다.
- [x] 배치한 Actor의 `ComfyUI|Result` 값에서 `LastStatusMessage`에 실패한 항목, node id, input key가 표시되는지 확인한다.
