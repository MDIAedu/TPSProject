# Task 01-21 - ComfyUI workflow JSON 파일 선택 창

## 설명
`EUW_ComfyUI`에서 workflow JSON 파일 경로를 직접 입력하는 대신 찾아보기 버튼을 눌러 Windows 파일 선택 창을 열고, 선택한 JSON 파일의 전체 경로를 기존 workflow 경로 입력값으로 설정할 수 있게 한다.

## 구현 항목
- [x] `Open ComfyUI Workflow File Dialog` Blueprint 노드를 호출하면 Windows 파일 선택 창이 열린다.
- [x] 파일 선택 창은 JSON 파일 필터를 사용하며 하나의 파일을 선택할 수 있다.
- [x] 선택한 파일이 실제로 존재하는 `.json` 파일이면 정규화된 전체 경로와 성공 여부를 반환한다.
- [x] 파일 선택을 취소하거나 올바른 JSON 파일을 선택하지 못하면 실패를 반환하고 기존 workflow 경로를 유지한다.
- [x] `EUW_ComfyUI`에서 반환된 경로를 기존 `EditText_Workflow`에 설정해 ComfyUI 이미지 생성 요청의 workflow 파일 값으로 사용할 수 있다.

## 범위 밖
- workflow JSON 내용 미리보기 또는 편집
- 최근 선택 파일 목록과 즐겨찾기 관리
- workflow 파일 자동 실행 또는 자동 유효성 검사
- Windows 이외 운영체제별 파일 선택 UI 검증
- Codex 프롬프트 생성과 ComfyUI 이미지 생성 방식 변경

## 사전 전제
- 01-18 단계의 `EUW_ComfyUI`와 Editor Utility Widget용 ComfyUI 이미지 생성 요청 기능이 유지되어 있다.
- 요청에 사용할 ComfyUI workflow JSON 파일이 로컬 PC에 준비되어 있다.

## 수동 작업
- Unreal Editor를 종료한 상태에서 `TPSProjectEditor` 타깃을 컴파일한다.
- Unreal Editor에서 `Content/UI/EUW_ComfyUI`를 열고 `EditText_Workflow` 옆에 찾아보기 용도의 Button과 버튼 내용을 표시할 Text를 추가한다.
- 추가한 Button을 변수로 설정하고 Graph에서 해당 Button의 `On Clicked` 이벤트를 만든다.
- `On Clicked`에서 `EditText_Workflow`의 `Get Text` 결과를 String으로 변환해 `Open ComfyUI Workflow File Dialog` 노드의 `Current File Path`에 연결한다.
- 노드의 Boolean 반환값을 `Branch`의 조건에 연결한다.
- `True` 실행 흐름에서 `Selected File Path`를 Text로 변환해 `EditText_Workflow`의 `Set Text`에 연결한다.
- `False` 실행 흐름은 `Set Text`를 호출하지 않도록 비워 두어 기존 경로가 유지되게 한다.
- 기존 `Button_Request` 요청 흐름이 `EditText_Workflow`의 값을 `Workflow Json File` 입력으로 사용하는 연결을 유지한다.

## 완료 조건

### 에이전트 확인
- [x] 관련 코드 수정 완료
- [x] 로컬 정적 점검 또는 프로젝트 구조 기준 확인 완료
- [x] Unreal C++/Blueprint/에셋 규칙 위반 없음
- [x] 현재 task 문서가 실제 구현 기준으로 갱신됨

### 결과 확인
- Unreal Editor에서 `Content/UI/EUW_ComfyUI`를 실행하고 추가한 찾아보기 버튼을 누른다.
- Windows 파일 선택 창이 열리고 파일 유형이 JSON 파일로 제한되는지 확인한다.
- JSON 파일을 선택했을 때 `EditText_Workflow`에 선택한 파일의 전체 경로가 표시되는지 확인한다.
- `EditText_Workflow`에 기존 경로가 있는 상태에서 파일 선택 창을 다시 열고 취소했을 때 기존 경로가 유지되는지 확인한다.
- 선택한 workflow 경로로 이미지 생성 요청을 실행했을 때 기존 ComfyUI 요청 흐름이 해당 파일을 사용하는지 확인한다.
