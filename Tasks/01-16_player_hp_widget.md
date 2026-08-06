# Task 01-16 - 플레이어 HP 위젯

## 설명
플레이어가 보스의 공격을 받았을 때 플레이어 HP가 줄어드는 상태를 화면 Widget으로 확인한다. HP 바는 임시 기본 도형이 아니라 Codex 실행으로 생성한 HP 바 프레임 이미지와 HP 게이지 바 이미지를 사용하며, 생성된 이미지는 `Content/Texture/` 폴더에 배치해 Widget 구성에 활용한다.

## 구현 항목
- [x] 플레이어에게 HP 값이 있고, 보스 공격을 받으면 HP가 감소한다.
- [x] 플레이어의 현재 HP와 최대 HP를 화면 Widget에서 읽어 표시할 수 있다.
- [x] Codex 실행으로 HP 바 프레임 이미지와 HP 게이지 바 이미지를 생성한다.
- [x] 생성된 HP 바 이미지를 `Content/Texture/` 폴더에 배치한다.
- [ ] 생성된 HP 바 이미지를 사용해 플레이어 HP Widget을 구성한다.
- [ ] 플레이어가 피해를 받을 때 HP 게이지가 현재 HP 비율에 맞게 줄어드는지 확인할 수 있다.
- [ ] 플레이어 HP Widget이 기존 보스 추적, 근접 공격, 점프 내려찍기, 보스 애니메이션 상태 전환, 전투장, 플레이어 이동, 카메라 조작, 조준, 사격, 점프 흐름을 깨지 않는다.

## 범위 밖
- 최종 퀄리티 UI 아트 제작
- 보스 HP UI
- 플레이어 사망 연출, 리스폰, 게임 오버 UI
- 회복 아이템, 방어력, 무적 시간
- UI 애니메이션, 사운드, 이펙트 연출

## 사전 전제
- 01-11 단계의 보스 큐브 길찾기 기반 플레이어 추적 이동이 유지되어 있다.
- 01-12 단계의 보스 근접 일반 공격 상태 전환이 유지되어 있다.
- 01-13 단계의 보스 점프 내려찍기 공격이 유지되어 있다.
- 01-14 단계의 보스 FSM 애니메이션 상태 전환이 유지되어 있다.
- 01-15 단계의 원형 보스 전투장 구성이 유지되어 있다.
- `Content/Maps/L_BattleMap.umap`에서 플레이어와 보스 전투 흐름을 확인할 수 있다.

## 수동 작업
- Unreal Editor에서 C++ 변경 사항을 컴파일한다.
- `PlayerHpWidget`을 부모 클래스로 하는 `WBP_PlayerHpWidget` Widget Blueprint를 만든다.
- `WBP_PlayerHpWidget`에 HP 바 프레임 Image와 HP 게이지 Image를 배치한다.
- HP 바 프레임 Image에는 `Content/Texture/T_PlayerHpFrame` Texture를 지정한다.
- HP 게이지 Image에는 `Content/Texture/T_PlayerHpGauge` Texture를 지정한다.
- HP 게이지 Image가 왼쪽에서 오른쪽으로 차오르고 줄어드는 방식으로 보이도록 배치, 정렬, 크롭 또는 마스크 방식을 구성한다.
- HP 게이지 표시값은 `PlayerHpWidget`의 `GetPlayerHealthPercent` 값을 사용하도록 연결한다.
- `BP_PlayerCubeCharacter`의 `Player|UI` 값에서 `PlayerHpWidgetClass`에 `WBP_PlayerHpWidget`을 지정한다.

## 완료 조건

### 에이전트 확인
- [x] 관련 코드 수정 완료
- [x] 로컬 정적 점검 또는 프로젝트 구조 기준 확인 완료
- [x] Unreal C++/Blueprint/에셋 규칙 위반 없음
- [x] 현재 task 문서가 실제 구현 기준으로 갱신됨

### 결과 확인
- [ ] Unreal Editor에서 `Content/Texture/T_PlayerHpFrame`과 `Content/Texture/T_PlayerHpGauge` Texture가 보이는지 확인한다.
- [ ] Unreal Editor에서 `Content/Maps/L_BattleMap.umap`을 연다.
- [ ] PIE를 실행했을 때 화면에 플레이어 HP Widget이 보이는지 확인한다.
- [ ] 보스 근접 공격을 맞았을 때 플레이어 HP 게이지가 줄어드는지 확인한다.
- [ ] 보스 점프 내려찍기 공격을 맞았을 때 플레이어 HP 게이지가 줄어드는지 확인한다.
- [ ] 플레이어 HP가 줄어들어도 플레이어 이동, 마우스 카메라 조작, 우클릭 조준, 좌클릭 사격, 스페이스바 점프가 기존처럼 동작하는지 확인한다.
- [ ] 플레이어 HP Widget 추가 후에도 보스 추적, 근접 공격, 점프 내려찍기, 보스 애니메이션 상태 전환, 전투장 경계가 기존처럼 동작하는지 확인한다.
