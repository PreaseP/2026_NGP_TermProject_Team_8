# 로그인 화면 이미지

2026-11-09 사용자 요청으로 기본 내장 image_gen 도구를 사용했다. 원본 `NGP_TermProject/client/Title.png`는 보존했다.

- `NGP_TermProject/client/TitleBackground.png`: 기존 두 버튼을 제거하고 눈 지형으로 채운 배경.
- `NGP_TermProject/client/GameStart.png`: 제목과 비슷한 입체 글자, 실제 알파 채널을 가진 독립 PNG. 게임에서는 투명 여백을 제외해 그리며 원본 PNG는 수정하지 않는다.
- `title.cpp`: 기존 파일 안에 입력창·상태 문구와 PNG 버튼 표시 함수를 추가했다. 처음 분리했던 titleLogin.cpp는 사용자 요청으로 이 파일에 합쳤다. 버튼 크기는 `1 + 0.035 * sin(시간 * 0.0025)`이며 같은 크기로 클릭 영역을 계산한다.

## 배경 생성에 사용한 최종 프롬프트

Use case precise-object-edit. Edit target is supplied game title screen. Preserve the SNOW BATTLE logo exactly in its existing position and preserve landscape, pixelated Minecraft-style snowy ground, plants, lake, trees, sky, framing and 16:9 aspect. Remove ONLY the two grey menu bars in the lower center and all Korean text on those bars. Seamlessly fill both former rectangles with natural continuous pixelated snowy ground matching surrounding perspective, texture and lighting. No new words, buttons, objects or UI. This is the clean game background asset.

## GAME START 생성에 사용한 최종 프롬프트

Use case stylized-concept / game UI asset. Supplied image is STYLE REFERENCE ONLY. Create a standalone transparent PNG game button lettering reading exactly "GAME START", a SINGLE HORIZONTAL LINE. Match SNOW BATTLE reference's bold chunky Minecraft voxel/block 3D lettering: light grey stone faces, slight cracks, thick black outlines, dark extruded sides downward, very readable. Only the words GAME START, no rectangle, no scenery, no snow background, no extra text. True alpha transparency around the entire lettering and in holes. Wide composition with tight framing and modest transparent margin, lettering fills canvas horizontally. Intended as clickable animated sprite over snowy title screen.

## 범위와 확인

- 닉네임은 첫 화면에서 입력한다. 버튼 클릭 또는 Enter로 로그인 요청, 서버 성공 응답 후 CLIENT_LOBBY 전환.
- 실패 메시지·대기 상태·중복 요청 차단·10초 응답 제한을 포함한다.
- 로비는 로그인 성공 안내 화면까지다. 실제 방 목록·방 선택·입장은 이후 일정이며 구현 완료로 표시하지 않는다.
- Debug x64 빌드와 로컬 TCP 응답 테스트 통과. 실제 게임 창에서 배경·PNG 투명도·글자 입력·로비 표시를 확인했다. 실제 팀 서버와의 연동 검증은 아니다.

## 후속 렌더링 구조 변경

사용자 요청으로 title.cpp 안의 모든 메뉴 출력을 기존 shaderProgramID/VAO 기반으로 통일했다. 고정 기능 OpenGL 호출과 익명 namespace를 제거했으며, 글자도 메모리에서 만든 글자 텍스처를 같은 셰이더로 출력한다. 이미지는 변경하지 않았다.

## 기본 로그인 흐름으로 단순화

사용자 요청으로 응답 대기 플래그·중복 요청 차단·10초 타임아웃 및 대기 UI를 제거했다. 위의 해당 기능 설명은 이전 버전의 기록이다. 현재는 닉네임 입력 후 Enter/클릭으로 요청하고 서버 성공 응답 후 임시 로비 안내를 표시한다.
