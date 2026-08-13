# Task 01-18 - Editor Utility Widget ComfyUI 이미지 생성 요청

## 설명
Actor를 월드에 배치하고 Details 패널에서 실행하던 ComfyUI workflow 요청 기능을 Editor Utility Widget에서 사용할 수 있도록 확장한다. 사용자가 Unreal Editor에서 구성하는 위젯을 통해 긍정 프롬프트, 이미지 너비, 이미지 높이, workflow 파일을 지정하고 이미지 생성 요청을 실행할 수 있어야 한다.

## 구현 항목
- [x] Editor Utility Widget에서 `Send ComfyUI Workflow Prompt` Blueprint 노드를 사용해 ComfyUI 이미지 생성 요청에 필요한 값을 설정할 수 있다.
- [x] `Positive Prompt` 입력으로 긍정 프롬프트를 설정할 수 있다.
- [x] `Image Width`와 `Image Height` 입력으로 이미지 너비와 높이를 설정할 수 있다.
- [x] `Workflow Json File` 입력으로 요청에 사용할 workflow 파일을 설정할 수 있다.
- [x] Editor Utility Widget의 이미지 생성 버튼 이벤트에서 `Send ComfyUI Workflow Prompt`를 호출하면 설정한 값이 workflow에 반영되어 ComfyUI 요청이 전송된다.
- [x] 완료 이벤트의 성공 여부, HTTP 상태 코드, 응답 메시지, 적용 노드 ID와 Output Log로 요청 결과를 확인할 수 있다.

## 범위 밖
- Editor Utility Widget 자산 생성과 위젯 레이아웃 구성
- 부정 프롬프트 입력 UI 추가
- ComfyUI workflow 편집기 제작
- 이미지 생성 결과 다운로드, 저장, Unreal 에셋 임포트
- ComfyUI 설치 또는 실행 자동화
- 기존 Actor 기반 Details 패널 요청 기능 제거

## 사전 전제
- 01-16 단계의 Unreal Editor에서 로컬 ComfyUI workflow 요청 기능이 유지되어 있다.
- 01-17 단계의 긍정 프롬프트와 이미지 크기 노드 자동 변경 기능이 유지되어 있다.
- ComfyUI 서버는 로컬 PC에서 별도로 실행되어 있다.
- 요청에 사용할 ComfyUI workflow JSON 파일은 미리 준비되어 있다.
- Editor Utility Widget 자산과 위젯 구성은 사용자가 Unreal Editor에서 진행한다.

## 수동 작업
- Unreal Editor를 종료한 상태에서 프로젝트 파일을 다시 생성하고 `TPSProjectEditor` 타깃을 컴파일한다.
- Unreal Editor에서 사용할 Editor Utility Widget 자산을 만든다.
- 위젯 Designer에서 긍정 프롬프트, workflow 파일 경로, 이미지 너비, 이미지 높이를 입력할 UI와 이미지 생성 버튼을 구성한다.
- 위젯 Blueprint에 workflow 파일을 담을 `File Path` 값과 긍정 프롬프트, 이미지 너비, 이미지 높이 값을 준비하고 각 입력 UI의 값이 여기에 반영되도록 연결한다.
- 이미지 생성 버튼의 `On Clicked` 이벤트에서 `Send ComfyUI Workflow Prompt` 노드를 호출한다.
- `Workflow Json File`, `Positive Prompt`, `Image Width`, `Image Height` 핀에 위젯에서 준비한 값을 연결한다.
- 기본 서버 주소를 사용할 수 없으면 노드의 고급 핀을 열어 `Server Base Url`에 실제 ComfyUI 주소를 입력한다.
- `Completed` 핀에 완료 이벤트를 연결하고 성공 여부, HTTP 상태 코드, 응답 메시지 또는 override 메시지를 위젯에서 확인할 Text 값에 반영한다.

## 완료 조건

### 에이전트 확인
- [x] 관련 코드 수정 완료
- [x] 로컬 정적 점검 또는 프로젝트 구조 기준 확인 완료
- [x] Unreal C++/Blueprint/에셋 규칙 위반 없음
- [x] 현재 task 문서가 실제 구현 기준으로 갱신됨

### 결과 확인
- Unreal Editor에서 Editor Utility Widget을 실행한다.
- `Check ComfyUI Server Connection` 노드를 연결한 경우 ComfyUI 서버가 실행 중일 때 성공 응답이 반환되는지 확인한다.
- 준비한 workflow 파일, 긍정 프롬프트, 이미지 너비와 높이를 위젯에 입력한다.
- 이미지 생성 버튼을 눌렀을 때 완료 이벤트가 실행되고 성공 여부가 참이며 HTTP 2xx 상태 코드가 반환되는지 확인한다.
- 완료 이벤트의 적용 긍정 프롬프트 노드 ID와 이미지 크기 노드 ID가 `0`이 아닌지 확인한다.
- ComfyUI에서 입력한 긍정 프롬프트와 이미지 크기로 workflow가 실행되어 이미지가 생성되는지 확인한다.
- 존재하지 않는 workflow 파일 경로를 입력했을 때 완료 이벤트에 파일을 찾을 수 없다는 실패 메시지가 반환되는지 확인한다.
- 이미지 너비 또는 높이에 `0` 이하를 입력했을 때 요청이 전송되지 않고 실패 메시지가 반환되는지 확인한다.
