# Task 01-19 - ComfyUI 생성 이미지 Texture2D 자동 임포트

## 설명
ComfyUI 이미지 생성이 완료되면 생성 결과를 Unreal Content 폴더인 `/Game/GeneratedTextures`에 자동으로 임포트한다. 임포트된 이미지는 Unreal Editor에서 사용할 수 있는 Texture2D 자산이어야 하며, 기존 자산을 덮어쓰지 않도록 중복되지 않는 자산 이름을 자동으로 사용해야 한다.

## 구현 항목
- [x] ComfyUI `/prompt` 응답의 `prompt_id`로 생성 완료 상태를 조회하고 결과 이미지를 자동으로 다운로드한다.
- [x] 다운로드한 원본 이미지를 프로젝트 `Saved/ComfyUIDownloads`에 보관하고 `/Game/GeneratedTextures`에 Texture2D 자산으로 자동 임포트한다.
- [x] 하나의 생성 작업에 결과 이미지가 여러 개 있으면 각 이미지를 Texture2D 자산으로 임포트한다.
- [x] `T_ComfyUI`를 기준으로 기존 자산을 덮어쓰지 않는 고유 자산 이름을 자동으로 지정한다.
- [x] 이미지 생성부터 임포트까지 완료된 결과 또는 실패 사유를 기존 Editor Utility Widget 완료 이벤트로 전달한다.

## 범위 밖
- ComfyUI workflow 또는 이미지 생성 방식 변경
- Editor Utility Widget 자산과 위젯 레이아웃 구성
- 임포트된 Texture2D를 사용하는 Material 또는 게임 콘텐츠 제작
- 패키징된 게임의 런타임 이미지 다운로드와 Texture2D 생성
- ComfyUI 설치 또는 실행 자동화

## 사전 전제
- 01-18 단계의 Editor Utility Widget용 ComfyUI 이미지 생성 요청 기능이 유지되어 있다.
- ComfyUI 서버는 로컬 PC에서 별도로 실행되어 있으며 workflow가 결과 이미지를 생성할 수 있다.

## 수동 작업
- Unreal Editor를 종료한 상태에서 프로젝트 파일을 다시 생성하고 `TPSProjectEditor` 타깃을 컴파일한다.
- 로컬 ComfyUI 서버를 실행하고 결과 이미지를 생성할 수 있는 workflow JSON 파일을 준비한다.
- `Content/UI/EUW_ComfyUI`의 이미지 생성 버튼이 `Send ComfyUI Workflow Prompt` 노드를 호출하는 상태를 유지한다.

## 완료 조건

### 에이전트 확인
- [x] 관련 코드 수정 완료
- [x] 로컬 정적 점검 또는 프로젝트 구조 기준 확인 완료
- [x] Unreal C++/Blueprint/에셋 규칙 위반 없음
- [x] 현재 task 문서가 실제 구현 기준으로 갱신됨

### 결과 확인
- Unreal Editor에서 `Content/UI/EUW_ComfyUI`를 실행한다.
- 준비한 workflow 파일, 긍정 프롬프트, 이미지 너비와 높이를 입력하고 이미지 생성 버튼을 누른다.
- ComfyUI 이미지 생성이 끝난 뒤 완료 이벤트가 실행되고 성공 여부가 참인지 확인한다.
- 완료 메시지에 임포트한 Texture2D 자산 수와 `/Game/GeneratedTextures/T_ComfyUI...` 자산 경로가 표시되는지 확인한다.
- Content Browser의 `/Game/GeneratedTextures`에서 생성 결과가 Texture2D 자산으로 보이는지 확인한다.
- 임포트된 Texture2D를 열어 ComfyUI에서 생성된 이미지와 내용이 일치하는지 확인한다.
- 이미지 생성을 한 번 더 실행했을 때 기존 자산을 덮어쓰지 않고 다른 이름의 Texture2D가 추가되는지 확인한다.
- Unreal Editor를 다시 실행해 자동 임포트된 Texture2D 자산이 유지되는지 확인한다.
