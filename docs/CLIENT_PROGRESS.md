# 현재 상태: 기존 게임 이식 단계

현재 진행: 기존 게임 이식과 10/31 초기화·종료 준비 이후, 아래의 **11/2 NetworkClient 연결 수명 관리**까지 구현했다. 이후 기능은 일정 순서대로 진행한다.

2026-11-02 사용자 요청으로 앞서 작성한 네트워크 구현을 되돌렸다. 사용자는 과정을 이해하며 단계별로 함께 작업하기를 원한다. 현재 범위 이후로 임의 진행하지 않는다.

- 원본: `C:/Users/user/source/repos/Computer-Grapics-Project/Computer-Grapics-Project`.
- 원본 .cpp/.h/.glsl/.png를 `NGP_TermProject/client/`에 그대로 복사했다. 원본 파일과 해시가 모두 같음을 확인했다. 원본 저장소는 수정하지 않았다.
- 팀 솔루션과 기존 client 프로젝트의 GUID·v145 설정을 유지했다. 원본 .sln/.vcxproj로 덮어쓰지 않았다.
- 원본 빌드 목록과 OpenGL 설정만 기존 client에 연결한다. Main.cpp가 시작 파일이며 cgproj.cpp는 참고 파일로 등록하고 빌드에서 제외한다.
- OpenGL 라이브러리는 `NGP_TermProject/external/`에 있다. 현재 의존성은 x64용이다. PNG·GLSL·DLL은 빌드 후 실행 폴더에 복사한다.
- server와 사용자 설계 문서 및 공용 프로토콜은 보존했다. protocol.h BOM도 작업 시작 전 상태로 복원했다.
- NetworkClient/GameClient/InputHandler/Inventory/ProtocolLayout, 네트워크용 Main 및 render/assets 구조, 테스트·스크립트는 리포에서 제거하고 임시 폴더에 백업했다.
- 백업: `C:/Users/user/AppData/Local/Temp/NGP-client-network-backup-20261102-204413`. 이전 진행 문서도 포함되어 있다. 사용자 지시 없이 복원하거나 네트워크 구현을 다시 진행하지 않는다.
- 실제 게임 화면 실행 확인은 아직 하지 않았다. 이전 네트워크 버전의 빌드·테스트 결과는 현재 원본 이식 단계의 검증 결과로 사용하지 않는다.

다음 단계는 사용자가 요청할 때 기존 Main.cpp의 흐름과 역할부터 함께 살펴보는 것이다.

검증: 원본 이식 상태의 Debug x64 MSBuild 빌드 성공. 원본 코드의 형변환 경고는 유지했다. 게임 창 실행·플레이 확인은 하지 않았다. 이전 네트워크 Release 실행 파일은 혼동을 피하도록 백업 폴더로 옮겼다.

## 다음 단계 1: InitOpenGL 분리 (사용자 승인)

- 사용자가 원본 이식 게임의 실행이 정상임을 확인했다.
- Main.cpp에서 창 생성, GLEW 초기화, OpenGL 버전 출력, 셰이더 준비, 타이틀 초기화를 `bool InitOpenGL(int& argc, char** argv)`로 옮겼다.
- main은 프로그램 시작 출력 후 이 함수를 호출한다. 기존 오류 종료는 함수의 false 반환 → main의 return 1로 유지한다.
- 코드의 `[변경 1]`은 선언, `[변경 2 시작/끝]`은 옮긴 함수, `[변경 3]`은 main 호출 위치다.
- 타이머·입력 콜백, 이동·공격·충돌·렌더링 로직은 수정하지 않았다. OnExit와 네트워크는 아직 구현하지 않았다.
- 이 단계부터 Main.cpp는 원본에서 초기화 함수를 분리한 상태이며, 다른 게임 소스·자산은 원본 그대로다.
- 검증: Debug x64 MSBuild 빌드 성공. 원본 Main.cpp와 대조해 이 함수 분리와 표시 주석만 달라졌음을 확인했다. 실행·플레이 재확인은 하지 않았다. 기존 형변환 경고는 유지된다.

## 다음 단계 2: OnExit 종료 연결 (사용자 승인)

- Main.cpp의 `[종료 변경 1~7]`, Snow.h/.cpp와 title.cpp의 `[OnExit 5/6]` 주석으로 변경 위치를 표시했다.
- `OnExit()`을 추가했다. 키 상태·차징을 초기화하고 캐릭터, 눈덩이, 쌓인 눈, 맵, 화면, 얼굴 이미지, 셰이더 자원을 창이 살아 있을 때 정리한다.
- 사용자 피드백에 따라 중복 호출을 막는 shutdownStarted/openGLReady 플래그를 제거했다. main·Keyboard에서 OnExit를 직접 호출하지 않는다.
- 모든 화면의 ESC/Q는 `glutLeaveMainLoop()`로 종료만 요청한다. freeglut이 창을 정리할 때 `glutCloseFunc(OnExit)` 콜백을 한 번 실행한다. X도 같은 창 닫기 콜백을 사용한다. title.cpp에 있던 직접 exit 호출은 제거했다.
- close 콜백은 GLEW 성공 직후 한 번 등록한다. 초기화 실패는 창 파괴를 예약하고 `glutMainLoopEvent()`로 처리한다. GLEW가 실패한 경우에는 GL 자원을 만들기 전이고 콜백도 등록하지 않은 상태다.
- Snow의 기존 소멸자 정리를 `cleanup()`으로 분리했다. 삭제한 GL ID를 0으로 바꿔 창이 닫힌 후 소멸자에서 중복 해제하지 않게 한다.
- 자원 정리는 루프 종료/창 파괴 단계에서 수행하므로 렌더링·타이머의 별도 종료 플래그 검사도 제거했다. 기존 게임 플레이 규칙과 네트워크 코드는 변경하지 않았다.
- 검증: Debug x64 MSBuild 빌드 성공. 기존 형변환 경고는 유지된다. 이번 변경 후 실제 ESC/Q·X 클릭 종료 동작은 아직 실행 검증하지 않았다.
- 다음에는 사용자가 타이틀·플레이·결과 화면의 ESC/Q와 X 닫기를 확인할 수 있다. 네트워크 단계는 이후 사용자 요청에 따라 진행한다.
- 단순화 검증: 설치된 freeglut 3.8.0 원본 fg_main.c/fg_init.c/fg_structure.c에서 루프 종료 → 창 파괴 → Destroy 콜백 호출 → 컨텍스트 해제 순서를 확인했다. Debug x64 재빌드 성공. 실제 UI 종료 동작은 아직 실행 검증하지 않았다.

## 11/2: NetworkClient 초기화·연결·종료

- 최종 계획서의 HLD 그림(image11: Client 초기화 → Connect → Game)과 LLD 그림/클래스 목록, 별도 LLD 문서의 연결 절차를 확인했다.
- `client/NetworkClient.h/.cpp`를 추가하고 기존 client 프로젝트와 필터에 등록했다. 새 코드는 `[11.2]` 표시를 사용한다. 기존 Main.cpp/Snow/title의 사용자 주석은 수정하지 않았다.
- `InitNetwork()`: Winsock 2.2 준비. `initialized`는 실제 WSAStartup 성공 소유 여부이며 추가 WSAStartup 참조 횟수가 생기지 않도록 한다.
- `ConnectToServer(ip, port)`: 숫자 IPv4 확인 → TCP 소켓 생성 → blocking connect → TCP_NODELAY → FIONBIO. 각 실패는 소켓을 닫고 false를 반환한다. 이미 연결된 상태의 재호출은 기존 연결을 보존하고 거절한다.
- `IsConnected()`: 연결 소켓 소유 여부를 확인한다. 상대 종료의 실시간 확인은 추후 recv 결과로 처리한다.
- `Disconnect()`: 소켓만 닫고 INVALID_SOCKET으로 초기화한다. Winsock은 유지하므로 재접속할 수 있다.
- `Shutdown()`: Disconnect 후, 준비한 Winsock 사용을 WSACleanup으로 마무리한다. 별도 자동 소멸자 정리 호출은 추가하지 않았다. 이후 GameClient 종료 단계에서 소유자가 호출한다.
- LLD는 Disconnect에 WSACleanup까지 설명하지만 최종 계획서는 Disconnect/Shutdown을 분리한다. 최종 계획서 기준으로 구현했다.
- 공용 NetUtil은 아직 없으므로 이번 client 내부에서 setsockopt/ioctlsocket을 직접 호출한다. 공유 폴더에 NetUtil을 임의 생성하지 않았다. 추후 공용 구현이 생기면 해당 호출을 연결할 수 있다.
- `common/protocol.h` 앞의 중복 UTF-8 BOM 1개만 제거했다(기존 2개 → 1개). 패킷 구조체·값·주석·나머지 바이트는 바꾸지 않았다.
- 게임 Main/OnExit에는 아직 NetworkClient를 붙이지 않았다. GameClient Init/TryConnect는 11/4, 전체 종료 연결은 11/7 일정이다. 송신·수신·패킷 추출도 아직 구현하지 않았다.
- 검증: 기존 client Debug x64 MSBuild 성공. 기존 게임 코드의 형변환 경고만 남는다.
- 임시 loopback 검증 PASS: 초기화 전 연결 거절, 잘못된 IPv4 거절, TCP 연결, 연결 중 중복 요청 시 기존 연결 유지, 해제 후 재접속, 연결 거부 시 소켓 정리, WSAStartup/WSACleanup 참조 횟수 균형, Shutdown 후 재초기화. 테스트 파일/실행 파일은 `%TEMP%/NGP-network-1102-smoke`에만 있으며 게임 프로젝트에는 추가하지 않았다.
- 실제 팀 서버 접속/로그인과 UI 연결은 미검증이다. 최초 blocking connect는 OS 연결 타임아웃까지 기다릴 수 있다(설계서 절차 유지).
- 다음 일정(11/3): QueuePacket / FlushSendBuffer / ReceiveData. 사용자가 요청할 때 진행한다. 사용자 요청으로 이 단계의 변경을 커밋·푸시한다.
