# Plans

## 목적

이 문서는 현재 진행 중인 단계 상태와 최근 작업 흐름을 빠르게 확인하기 위한 기록 문서입니다.
<!-- 새 프로젝트 시작 시 작업리스트와 최근 작업 로그의 기록을 초기화한다. -->

## 작업리스트
| 단계 | Task 문서 | 해야 할 항목 | 상태 |
| --- | --- | --- |----|
| 01-1 | [Tasks/01-1_basic_play_map.md](../Tasks/01-1_basic_play_map.md) | 동작 검증용 기본 플레이 맵 구성 | 완료 |
| 01-2 | [Tasks/01-2_player_cube_movement.md](../Tasks/01-2_player_cube_movement.md) | WASD 입력으로 플레이어 큐브 이동 검증 | 완료 |
| 01-3 | [Tasks/01-3_player_character_base.md](../Tasks/01-3_player_character_base.md) | 플레이어 조작 대상을 Character 기반 구조로 전환 | 완료 |
| 01-4 | [Tasks/01-4_third_person_shoulder_camera.md](../Tasks/01-4_third_person_shoulder_camera.md) | 마우스 조작 3인칭 숄더뷰 카메라와 카메라 기준 이동 구성 | 완료 |
| 01-5 | [Tasks/01-5_basic_aim_and_fire.md](../Tasks/01-5_basic_aim_and_fire.md) | 마우스 우클릭 조준과 좌클릭 기본 사격 명중 판정 검증 | 완료 |
| 01-6 | [Tasks/01-6_wraith_locomotion_mesh.md](../Tasks/01-6_wraith_locomotion_mesh.md) | 플레이어 큐브를 Paragon Wraith 메시와 기본 이동 로코모션으로 교체 | 완료 |
| 01-7 | [Tasks/01-7_three_step_fire_combo.md](../Tasks/01-7_three_step_fire_combo.md) | 좌클릭 연속 사격을 1타·2타·3타 콤보로 관리 | 완료 |
| 01-8 | [Tasks/01-8_anim_notify_combo_attacks.md](../Tasks/01-8_anim_notify_combo_attacks.md) | 3단 콤보 사격 단계별 공격 애니메이션과 Anim Notify 입력 허용 구간 연결 | 완료 |
| 01-9 | [Tasks/01-9_vertical_aim_offset.md](../Tasks/01-9_vertical_aim_offset.md) | 마우스 상하 움직임에 따른 상체 세로 조준 Aim Offset 검증 | 완료 |
| 01-10 | [Tasks/01-10_spacebar_jump.md](../Tasks/01-10_spacebar_jump.md) | 스페이스바 입력으로 플레이어 점프와 공중 단일 모션 검증 | 완료 |
| 01-11 | [Tasks/01-11_boss_cube_chase_movement.md](../Tasks/01-11_boss_cube_chase_movement.md) | 보스 큐브의 길찾기 기반 플레이어 추적 이동 검증 | 완료 |
| 01-12 | [Tasks/01-12_boss_melee_attack_state.md](../Tasks/01-12_boss_melee_attack_state.md) | 보스의 근접 일반 공격 상태 전환 검증 | 완료 |

## 상태 범례

- `예정`: 아직 시작 전
- `진행중`: 구현 중이거나, 구현 후 사용자 수동 작업 또는 결과 확인을 기다리는 상태
- `완료`: 구현 및 현재 확인 모드 기준 확인 완료

## 최근 작업 로그
- 2026-07-30: 사용자 요청에 따라 01-6 Paragon Wraith 메시 교체와 기본 이동 로코모션 검증 task 문서 작성.
- 2026-07-30: 01-6 `UPlayerWraithAnimInstance` 추가, `APlayerCubeCharacter` 큐브 메시 제거와 Character Mesh 사용 준비, 사용자 에디터 자산 연결과 결과 확인 대기.
- 2026-07-30: 01-6 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-07-30: 사용자 요청에 따라 01-7 좌클릭 3단 콤보 사격 상태 관리 task 문서 작성.
- 2026-07-30: 01-7 `APlayerCubeCharacter`에 3단 사격 콤보 단계, 단계별 피해량, 콤보 리셋 시간, HUD 참조용 조회 함수를 추가하고 사용자 결과 확인 대기.
- 2026-07-30: 사용자 요청에 따라 01-8 3단 콤보 공격 애니메이션과 Anim Notify 입력 허용 구간 task 문서 작성.
- 2026-07-30: 01-8 `APlayerCubeCharacter`에 단계별 공격 몽타주 연결값과 Anim Notify 입력 허용 함수를 추가하고 사용자 에디터 자산 연결과 결과 확인 대기.
- 2026-07-31: 사용자 요청에 따라 01-9 상체 세로 조준 Aim Offset 검증 task 문서 작성.
- 2026-07-31: 01-9 `APlayerCubeCharacter`와 `UPlayerWraithAnimInstance`에 Aim Offset용 `AimPitch` 전달 값을 추가하고 사용자 에디터 자산 연결과 결과 확인 대기.
- 2026-07-31: 01-9 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-07-31: 사용자 요청에 따라 01-10 스페이스바 점프와 공중 단일 모션 검증 task 문서 작성.
- 2026-07-31: 01-10 `APlayerCubeCharacter`에 점프 입력 바인딩을 추가하고 `UPlayerWraithAnimInstance`에 공중 상태 값을 추가한 뒤 사용자 에디터 자산 연결과 결과 확인 대기.
- 2026-08-05: 사용자 요청에 따라 01-11 보스 큐브의 길찾기 기반 플레이어 추적 이동 검증 task 문서 작성.
- 2026-08-05: 01-11 `ABossCubeCharacter`와 `ABossCubeAIController`를 추가해 플레이어 Pawn을 길찾기 이동 대상으로 갱신하고 사용자 에디터 자산 배치와 결과 확인 대기.
- 2026-08-05: 01-11 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-08-05: 사용자 요청에 따라 01-12 보스 근접 일반 공격 상태 전환 검증 task 문서 작성.
- 2026-08-05: 01-12 `ABossCubeAIController`에 추적/근접 공격 FSM과 기본 피해량 20 일반 공격 판정을 추가하고 사용자 결과 확인 대기.
- 2026-08-05: 01-12 사용자 결과 확인 완료, task 상태 완료 처리.
