# 사용자 요청

- 아래 내용을 기반으로 `docs/PLANS.md`의 작업리스트 단계 하나를 작성해줘
- 그 이후에 해당 단계의 task 파일을 새로 만들어줘
- 단, 아래 두 가지 경우에는 진행하지 말고 이유를 먼저 알려줘.
    - 이전 task 다음 순서로 오기에 적합하지 않을 때
    - 현재 task의 범위가 너무 클 때

## 원하는 동작
- 준비된 ComfyUI workflow 파일에서 긍정 프롬프트와 이미지 크기 값을 Unreal 쪽 입력값으로 바꿔 요청한다.

## 세부 설명
- positive prompt 값을 치환할 node id와 input key를 명시적으로 관리한다.
- width, height 값을 치환할 node id와 input key를 명시적으로 관리한다.
- seed, negative prompt, batch count는 필요하면 추가할 수 있도록 구조만 열어 둔다.
- workflow 구조가 맞지 않을 때는 에러 메시지로 어떤 node/key가 문제인지 확인할 수 있게 한다.


