# Task 01-20 - Codex CLI 기반 Flux.2 프롬프트 생성

## 설명
Unreal Editor에서 입력한 긍정 프롬프트를 ComfyUI에 그대로 보내지 않고, `codex exec`를 통해 Flux.2 이미지 생성에 적합한 영문 프롬프트로 변환한 뒤 기존 ComfyUI 이미지 생성 요청에 사용한다. 실제 ComfyUI 요청에 적용한 최종 프롬프트는 Unreal Editor의 Output Log에서 확인할 수 있어야 한다.

## 구현 항목
- [x] `CodexFluxPromptService`가 ComfyUI 이미지 생성 요청 전에 사용자가 입력한 긍정 프롬프트를 `codex exec`의 표준 입력으로 전달해 Flux.2용 프롬프트를 생성한다.
- [x] Codex 지시문과 결과 검증을 통해 최종 프롬프트를 ASCII 영문으로 제한한다.
- [x] 생성된 영문 프롬프트를 기존 workflow의 긍정 프롬프트 값에 반영해 ComfyUI 이미지 생성 요청을 전송한다.
- [x] ComfyUI 요청에 실제로 적용한 최종 영문 프롬프트를 `ComfyUI 최종 긍정 프롬프트` 로그로 Output Log에 출력한다.
- [x] `codex exec` 실행 실패, 시간 초과, 빈 결과 또는 영문 제한 위반 시 원본 긍정 프롬프트를 전송하지 않고 ComfyUI 요청을 중단하며 완료 이벤트와 Output Log에 실패 사유를 전달한다.

## 범위 밖
- Codex CLI 설치, 로그인 또는 인증 자동화
- Flux.2 모델 설치와 ComfyUI workflow의 모델 구성 변경
- 부정 프롬프트 생성 또는 변경
- 이미지 크기 등 긍정 프롬프트 외 workflow 입력값의 Codex 기반 생성
- Codex가 생성한 프롬프트를 사용자가 수정하는 별도 UI 제작
- 패키징된 게임의 런타임 Codex CLI 호출

## 사전 전제
- 01-18 단계의 Editor Utility Widget용 ComfyUI 이미지 생성 요청 기능이 유지되어 있다.
- 01-19 단계의 ComfyUI 생성 이미지 다운로드와 Texture2D 자동 임포트 기능이 유지되어 있다.
- 로컬 PC에서 `codex exec`를 실행할 수 있도록 Codex CLI 설치와 인증이 완료되어 있다.
- ComfyUI 서버는 로컬 PC에서 별도로 실행되어 있으며 Flux.2 workflow가 결과 이미지를 생성할 수 있다.

## 수동 작업
- Unreal Editor를 종료한 상태에서 `TPSProjectEditor` 타깃을 컴파일한다.
- 터미널에서 Codex CLI 설치와 로그인이 유지되어 있는지 확인한다.
- 로컬 ComfyUI 서버를 실행하고 Flux.2 모델로 이미지를 생성할 수 있는 workflow JSON 파일을 준비한다.
- `Content/UI/EUW_ComfyUI`의 이미지 생성 버튼이 기존 `Send ComfyUI Workflow Prompt` 노드를 호출하는 상태를 유지한다.

## 완료 조건

### 에이전트 확인
- [x] 관련 코드 수정 완료
- [x] 로컬 정적 점검 또는 프로젝트 구조 기준 확인 완료
- [x] Unreal C++/Blueprint/에셋 규칙 위반 없음
- [x] 현재 task 문서가 실제 구현 기준으로 갱신됨

### 결과 확인
- Unreal Editor에서 Output Log를 열고 `Content/UI/EUW_ComfyUI`를 실행한다.
- Flux.2 workflow 파일, 한국어 또는 짧은 긍정 프롬프트, 이미지 너비와 높이를 입력한 뒤 이미지 생성 버튼을 누른다.
- 프롬프트 생성 중에도 Unreal Editor UI가 멈추지 않는지 확인한다.
- Output Log의 `ComfyUI 최종 긍정 프롬프트` 로그에 원본을 그대로 복사한 값이 아닌 ASCII 영문 프롬프트가 표시되는지 확인한다.
- ComfyUI에 전달된 긍정 프롬프트가 Output Log의 최종 영문 프롬프트와 일치하는지 확인한다.
- ComfyUI 이미지 생성과 `/Game/GeneratedTextures` Texture2D 자동 임포트가 기존과 동일하게 완료되는지 확인한다.
- 긍정 프롬프트를 비운 채 요청했을 때 완료 이벤트와 Output Log에 실패 사유가 표시되고 ComfyUI 요청이 전송되지 않는지 확인한다.
