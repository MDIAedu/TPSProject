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
| 01-13 | [Tasks/01-13_boss_jump_slam_attack.md](../Tasks/01-13_boss_jump_slam_attack.md) | 보스의 거리 기반 점프 내려찍기 공격 검증 | 완료 |
| 01-14 | [Tasks/01-14_boss_fsm_animation_state.md](../Tasks/01-14_boss_fsm_animation_state.md) | 보스 FSM 상태에 맞는 애니메이션 상태 전환 검증 | 완료 |
| 01-15 | [Tasks/01-15_circular_boss_battle_arena.md](../Tasks/01-15_circular_boss_battle_arena.md) | 산과 절벽으로 둘러싸인 원형 보스 전투장 검증 | 완료 |
| 01-16 | [Tasks/01-16_local_comfyui_workflow_request.md](../Tasks/01-16_local_comfyui_workflow_request.md) | Unreal Editor에서 로컬 ComfyUI workflow 요청 검증 | 완료 |

## 상태 범례

- `예정`: 아직 시작 전
- `진행중`: 구현 중이거나, 구현 후 사용자 수동 작업 또는 결과 확인을 기다리는 상태
- `완료`: 구현 및 현재 확인 모드 기준 확인 완료

## 최근 작업 로그
- 2026-08-05: 사용자 요청에 따라 01-12 보스 근접 일반 공격 상태 전환 검증 task 문서 작성.
- 2026-08-05: 01-12 `ABossCubeAIController`에 추적/근접 공격 FSM과 기본 피해량 20 일반 공격 판정을 추가하고 사용자 결과 확인 대기.
- 2026-08-05: 01-12 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-08-05: 사용자 요청에 따라 01-13 보스 거리 기반 점프 내려찍기 공격 검증 task 문서 작성.
- 2026-08-05: 01-13 `ABossCubeAIController`에 점프 내려찍기 상태, 고정 착지 지점 이동, 원형 범위 판정을 추가하고 사용자 결과 확인 대기.
- 2026-08-05: 01-13 점프 내려찍기 중 보스 Pawn 충돌을 Overlap으로 전환하고 플레이어 겹침 시 밀어낸 뒤 종료 시 충돌을 복구하도록 보정.
- 2026-08-05: 01-13 점프 중 Overlap 피격 시 즉시 피해를 주고, 점프 피해를 받은 플레이어는 충돌 여부와 상관없이 밀려나도록 판정 순서 보정.
- 2026-08-05: 01-13 점프 중 Overlap 피해 후 밀려난 경우에도 착지 디버그 원과 로그가 점프 공격 피해 기준으로 표시되도록 보정.
- 2026-08-05: 01-13 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-08-06: 사용자 요청에 따라 01-14 보스 FSM 상태에 맞는 애니메이션 상태 전환 검증 task 문서 작성.
- 2026-08-06: 01-14 `UBossCubeAnimInstance`를 추가해 보스 FSM 상태를 AnimBP에서 읽을 값으로 노출하고 사용자 결과 확인 대기.
- 2026-08-06: 01-14 점프 내려찍기 애니메이션 구간을 시작, 공중, 착지 상태로 나누고 AnimBP 노출 값과 타이밍 조정값을 추가.
- 2026-08-06: 01-14 사용자 결과 확인 완료, task 상태 완료 처리.
- 2026-08-06: 사용자 요청에 따라 01-15 산과 절벽으로 둘러싸인 원형 보스 전투장 검증 task 문서 작성.
- 2026-08-06: 01-15 `ABossBattleArenaActor`를 추가해 원형 전투장 바닥, 외곽 Static Mesh 링, 충돌 경계를 수치 기반으로 구성하고 사용자 결과 확인 대기.
- 2026-08-06: 01-15 외곽 경계 Mesh 세그먼트마다 높이, 두께, 위치 변화를 적용해 산/절벽 실루엣처럼 보이도록 보정.
- 2026-08-12: 사용자 요청에 따라 01-16 Unreal Editor에서 로컬 ComfyUI workflow 요청 검증 task 문서 작성.
- 2026-08-12: 01-16 `AComfyUIWorkflowRequestActor`를 추가해 Editor Details 패널에서 workflow JSON을 ComfyUI `/prompt`로 보내고 응답을 확인하도록 구현, 사용자 결과 확인 대기.
- 2026-08-12: 01-16 `Misc/FilePath.h` include 컴파일 오류를 `UObject/NoExportTypes.h` include 사용으로 수정.
- 2026-08-12: 01-16 ComfyUI 응답 없음 원인 분리를 위해 Editor Details 패널에서 `/system_stats`를 호출하는 서버 연결 확인 버튼을 추가.
- 2026-08-12: 01-16 `/prompt` 요청 시 workflow JSON을 ComfyUI API 형식의 `prompt` 필드로 감싸고 요청 타임아웃과 본문 크기 로그를 추가.
- 2026-08-12: 01-16 ComfyUI UI workflow 저장본을 API prompt JSON으로 착각해 보내는 경우를 감지해 로컬 실패 메시지를 표시하도록 보정.
- 2026-08-12: 01-16 저장된 ComfyUI UI workflow JSON을 `/prompt` API용 prompt 객체로 변환하고 프롬프트와 이미지 크기 override를 적용하도록 변경.
- 2026-08-12: 01-16 ComfyUI 검증 응답에 맞춰 `UNETLoader`, `CLIPLoader`, `InspyrenetRembg` widget 입력 매핑을 추가.
- 2026-08-12: 01-16 저장된 workflow의 원본 프롬프트가 실행되는 경우를 구분하기 위해 override가 실제 적용된 노드 ID와 메시지를 노출.
