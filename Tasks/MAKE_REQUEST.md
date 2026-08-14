# 사용자 요청

- 아래 내용을 기반으로 `docs/PLANS.md`의 작업리스트 단계 하나를 작성해줘
- 그 이후에 해당 단계의 task 파일을 새로 만들어줘
- 단, 아래 두 가지 경우에는 진행하지 말고 이유를 먼저 알려줘.
    - 이전 task 다음 순서로 오기에 적합하지 않을 때
    - 현재 task의 범위가 너무 클 때

## 원하는 동작
- ComfyUI 가 이미지를 만들면 자동으로 지정한 Unreal Content 폴더에 Texture2D .uasset 으로 import 되게 하고 싶어.

## 세부 설명
- 저장 위치는 /Game/GeneratedTextures 로 둔다.
- 이미지 파일은 Unreal 에서 사용할 수 있는 Texture2D asset 으로 import 해야 해.
- asset 이름은 중복되지 않게 자동 생성해 줘.


