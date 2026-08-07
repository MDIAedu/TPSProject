# 사용자 요청

- 아래 내용을 기반으로 `docs/PLANS.md`의 작업리스트 단계 하나를 작성해줘
- 그 이후에 해당 단계의 task 파일을 새로 만들어줘
- 단, 아래 두 가지 경우에는 진행하지 말고 이유를 먼저 알려줘.
    - 이전 task 다음 순서로 오기에 적합하지 않을 때
    - 현재 task의 범위가 너무 클 때

## 원하는 동작
- Editor Utility Widget에서 프롬프트, 이미지 크기, workflow 파일, 저장 폴더를 설정하고 이미지 생성 버튼을 누를 수 있다.

## 세부 설명
- UI 입력 항목은 positive prompt, width, height, workflow 파일 경로, Unreal Content 저장 폴더를 기본으로 둔다.
- `이미지 만들기` 버튼을 누르면 현재 입력값으로 ComfyUI 생성 요청을 시작한다.
- 생성 중, 성공, 실패 상태를 위젯에서 확인할 수 있게 한다.
- 이 task에서는 생성 결과를 Texture asset으로 import하는 단계는 아직 다루지 않는다.


