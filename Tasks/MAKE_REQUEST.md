# 사용자 요청

- 아래 내용을 기반으로 `docs/PLANS.md`의 작업리스트 단계 하나를 작성해줘
- 그 이후에 해당 단계의 task 파일을 새로 만들어줘
- 단, 아래 두 가지 경우에는 진행하지 말고 이유를 먼저 알려줘.
    - 이전 task 다음 순서로 오기에 적합하지 않을 때
    - 현재 task의 범위가 너무 클 때

## 원하는 동작
- ComfyUI가 만든 이미지 파일을 지정한 Unreal Content 폴더에 Texture2D `.uasset`으로 import한다.

## 세부 설명
- 예시 저장 위치는 `/Game/GeneratedTextures`로 둔다.
- 이미지 파일은 Unreal에서 사용할 수 있는 Texture2D asset으로 import한다.
- asset 이름은 중복되지 않게 자동 생성하거나 입력값 기반 prefix를 사용한다.
- import 후 sRGB, compression, mip 설정처럼 필요한 Texture 기본 설정을 적용할 수 있게 한다.


