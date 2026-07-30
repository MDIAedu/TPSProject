# Plans

## 목적

이 문서는 현재 진행 중인 단계 상태와 최근 작업 흐름을 빠르게 확인하기 위한 기록 문서입니다.
<!-- 새 프로젝트 시작 시 작업리스트와 최근 작업 로그의 기록을 초기화한다. -->

## 작업리스트
| 단계 | Task 문서 | 해야 할 항목 | 상태 |
| --- | --- | --- | --- |
| 01-1 | [Tasks/01-1_basic_play_map.md](../Tasks/01-1_basic_play_map.md) | 동작 검증용 기본 플레이 맵 구성 | 완료 |
| 01-2 | [Tasks/01-2_player_cube_movement.md](../Tasks/01-2_player_cube_movement.md) | WASD 입력으로 플레이어 큐브 이동 검증 | 완료 |
| 01-3 | [Tasks/01-3_player_character_base.md](../Tasks/01-3_player_character_base.md) | 플레이어 조작 대상을 Character 기반 구조로 전환 | 완료 |
| 01-4 | [Tasks/01-4_third_person_shoulder_camera.md](../Tasks/01-4_third_person_shoulder_camera.md) | 마우스 조작 3인칭 숄더뷰 카메라와 카메라 기준 이동 구성 | 완료 |
| 01-5 | [Tasks/01-5_basic_aim_and_fire.md](../Tasks/01-5_basic_aim_and_fire.md) | 마우스 우클릭 조준과 좌클릭 기본 사격 명중 판정 검증 | 완료 |
| 01-6 | [Tasks/01-6_wraith_locomotion_mesh.md](../Tasks/01-6_wraith_locomotion_mesh.md) | 플레이어 큐브를 Paragon Wraith 메시와 기본 이동 로코모션으로 교체 | 진행중 |

## 상태 범례

- `예정`: 아직 시작 전
- `진행중`: 구현 중이거나, 구현 후 사용자 수동 작업 또는 결과 확인을 기다리는 상태
- `완료`: 구현 및 현재 확인 모드 기준 확인 완료

## 최근 작업 로그
- 2026-07-23: 01-1 기본 플레이 맵 `Content/Maps/L_BasicPlayMap.umap` 생성, 기본 실행 맵 설정 갱신, 사용자 결과 확인 대기.
- 2026-07-23: 01-1 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-07-23: 사용자가 Unreal Editor에서 `Content/Maps/L_BattleMap.umap`을 직접 만들고 `PlayerStart`까지만 배치했다고 정정, task를 실제 상태 기준으로 갱신하고 남은 항목 확인 대기로 변경.
- 2026-07-23: 보스 시작 후보 위치와 보스 확인용 임시 형상 배치는 보스 제작 단계에서 진행하기로 범위 조정, 01-1 완료 처리.
- 2026-07-23: 01-2 Enhanced Input 기반 `APlayerCubePawn` C++ 코드 추가, IA/IMC/Blueprint 에디터 연결과 사용자 결과 확인 대기.
- 2026-07-23: 01-2 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-07-24: 01-3 `APlayerCubeCharacter` C++ 코드 추가, 기존 `APlayerCubePawn` C++ 코드 삭제, Character 기반 Blueprint 생성과 사용자 결과 확인 대기.
- 2026-07-24: 01-3 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-07-24: 01-4 `APlayerCubeCharacter`에 숄더뷰 카메라와 마우스 Look 입력, 카메라 기준 이동 코드 추가, 사용자 결과 확인 대기.
- 2026-07-24: 01-4 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-07-24: 사용자 요청에 따라 01-5 기본 조준과 사격 명중 판정 검증 task 문서 작성.
- 2026-07-24: 01-5 `APlayerCubeCharacter`에 조준 상태와 카메라 정면 라인 트레이스 사격 판정 추가, 사용자 수동 연결과 결과 확인 대기.
- 2026-07-24: 01-5 사용자 확인 중 사격 디버그 표시와 `Fire hit` 로그가 나오지 않아 입력/조준 상태 진단 로그와 디버그 표시 시간을 보강.
- 2026-07-24: 01-5 사용자 요청에 따라 좌클릭은 항상 사격하고, 우클릭은 확대 조준 카메라 상태로만 동작하도록 변경.
- 2026-07-24: 01-5 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-07-30: 사용자 요청에 따라 01-6 Paragon Wraith 메시 교체와 기본 이동 로코모션 검증 task 문서 작성.
- 2026-07-30: 01-6 `UPlayerWraithAnimInstance` 추가, `APlayerCubeCharacter` 큐브 메시 제거와 Character Mesh 사용 준비, 사용자 에디터 자산 연결과 결과 확인 대기.
