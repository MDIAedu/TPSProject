# Task 01-16 - Unreal Editor ComfyUI 요청 검증

## 설명
Unreal Editor에서 로컬 ComfyUI 서버에 준비된 workflow JSON을 `/prompt` 요청으로 전송하고, 서버 응답을 확인할 수 있는지 검증한다. ComfyUI 서버는 로컬에서 실행 중인 것으로 전제하며, 기본 주소는 `http://127.0.0.1:8188`을 사용한다.

## 구현 항목
- [x] Unreal Editor에서 로컬 ComfyUI 서버 주소를 기준으로 요청을 보낼 수 있다.
- [x] 준비된 workflow JSON 파일을 읽어 ComfyUI `/prompt` 요청에 사용할 수 있다.
- [x] ComfyUI UI workflow JSON을 기본적인 `/prompt` API 요청 본문으로 변환할 수 있다.
- [x] ComfyUI `/prompt` 요청 성공 여부와 응답 내용을 Unreal Editor에서 확인할 수 있다.
- [x] ComfyUI 서버가 응답하지 않거나 workflow JSON을 읽을 수 없을 때 실패 원인을 확인할 수 있다.

## 범위 밖
- ComfyUI workflow JSON 제작
- ComfyUI 서버 실행 또는 설치 자동화
- 생성 결과 이미지 다운로드
- Texture import
- Unreal Editor UI 제작
- 게임플레이 중 ComfyUI 요청
- 패키징 빌드에서의 ComfyUI 연동

## 사전 전제
- 로컬 ComfyUI 서버가 `http://127.0.0.1:8188`에서 실행 중이다.
- `/prompt` 요청에 사용할 workflow JSON 파일이 준비되어 있다.

## 수동 작업
- 로컬 ComfyUI 서버를 `http://127.0.0.1:8188`에서 실행한다.
- Unreal Editor에서 C++ 변경 사항을 컴파일한다.
- `ComfyUIPromptRequestTester`를 부모 클래스로 하는 Blueprint를 만들거나, `ComfyUIPromptRequestTester` C++ Actor를 레벨에 배치한다.
- 배치한 Actor의 `ComfyUI|Request` 값에서 `ServerBaseUrl`이 `http://127.0.0.1:8188`인지 확인한다.
- 배치한 Actor의 `ComfyUI|Request` 값에서 `WorkflowJsonFilePath`에 `/prompt` 요청에 사용할 workflow JSON 파일 경로를 지정한다.

## 완료 조건

### 에이전트 확인
- [x] 관련 코드 수정 완료
- [x] 로컬 정적 점검 또는 프로젝트 구조 기준 확인 완료
- [x] Unreal C++/Blueprint/에셋 규칙 위반 없음
- [x] 현재 task 문서가 실제 구현 기준으로 갱신됨

### 결과 확인
- [x] PIE를 실행하지 않은 상태에서 배치한 Actor를 선택한다.
- [x] Details 패널의 `ComfyUI` 항목에서 `SendPromptRequest`를 실행한다.
- [x] UI workflow JSON을 지정했을 때 Output Log에 `/prompt API 포맷으로 변환했습니다` 로그가 표시되는지 확인한다.
- [x] Output Log에 `ComfyUI /prompt 요청 성공` 로그가 표시되는지 확인한다.
- [x] 배치한 Actor의 `ComfyUI|Result` 값에서 `bLastRequestSucceeded`가 true이고 `LastHttpStatusCode`가 200번대인지 확인한다.
- [x] 배치한 Actor의 `ComfyUI|Result` 값에서 `LastResponseBody`에 ComfyUI 응답 내용이 표시되는지 확인한다.
- [x] ComfyUI 서버를 끄거나 잘못된 workflow JSON 경로를 지정했을 때 `LastStatusMessage`와 Output Log에서 실패 원인을 확인할 수 있는지 확인한다.
