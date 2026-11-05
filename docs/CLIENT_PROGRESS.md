# 현재 상태: 기존 게임 이식 단계

현재 진행: 기존 게임 이식과 초기화·종료 준비, 11/2 NetworkClient 연결 수명 관리에 이어 **11/3 QueuePacket / FlushSendBuffer / ReceiveData**와 **11/4 ExtractPacket 및 GameClient::Init / TryConnect**까지 구현했고, **11/5 PollNetwork의 송수신·패킷 추출·전달과 ProcessPacket의 검사·종류별 분기 틀**을 추가했다. 각 Handle 함수의 게임 처리, Tick과 Main 연결은 아직 하지 않았다. 이후 기능은 사용자 요청에 따라 단계별로 진행한다.

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

현재 설명 대상은 `ReceiveData()`의 문법과 수신 버퍼 누적 흐름이다. 아래에는 이전 단계부터의 작업 이력을 남긴다.

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

## 11/3 첫 단계: sendBuffer와 QueuePacket만 추가

- 사용자 요청: 한꺼번에 세 함수를 구현하지 않고, 각 단계를 설명하며 진행한다. 현재 승인 범위는 송신 버퍼와 QueuePacket까지다.
- NetworkClient.h에 vector include, `std::vector<unsigned char> sendBuffer`, `bool QueuePacket(const void* packet)` 선언을 추가했다.
- NetworkClient.cpp의 QueuePacket은 연결 여부/null 확인 → 첫 바이트의 전체 길이 확인(최소 PacketHeader 크기) → sendBuffer 뒤에 해당 바이트를 복사 → 성공 반환 순서다. size가 1바이트이므로 최대 255다. 호출자는 size만큼 유효한 패킷 메모리를 전달해야 한다.
- Disconnect는 연결 소켓 정리 후 sendBuffer도 비운다. 이전 연결의 미전송 요청을 다음 연결로 넘기지 않도록 한 것이다.
- 새 부분은 `[11.3]` 표시를 달았다. 기존 사용자 주석은 수정하지 않았다. 게임 소스, 프로젝트 설정, 프로토콜은 이번 단계에서 수정하지 않았다.
- 검증: 기존 client Debug x64 MSBuild 성공. 아직 QueuePacket을 호출하는 실행 테스트나 실제 송신은 하지 않았다. FlushSendBuffer와 ReceiveData는 미구현이다.
- 다음 설명 대상: QueuePacket의 첫 바이트(size), 포인터 변환, vector::insert로 범위를 복사하는 과정. 이후 사용자 요청에 따라 FlushSendBuffer로 진행한다.

## 11/3 두 번째 단계: FlushSendBuffer 추가

- 사용자와 QueuePacket은 완성된 패킷을 바이트 벡터에 복사만 하고, TCP 수신 쪽은 size로 패킷 경계를 복원한다는 점을 확인했다. 이후 FlushSendBuffer만 추가하도록 요청받았다.
- NetworkClient.h에 NET_RESULT(정상/일시 불가/종료/오류)와 FlushSendBuffer 선언을 추가했다. 공용 protocol.h는 변경하지 않았다.
- NetworkClient.cpp에서 기존 sendBuffer를 send()에 전달한다. 별도 송신 버퍼는 추가하지 않았다. send 길이는 int 범위 안에서 전달한다.
- 양수 반환 시 처리된 앞부분만 erase하고 남은 데이터를 반복해서 시도한다. WSAEWOULDBLOCK은 남은 바이트를 유지한 채 NET_WOULD_BLOCK으로 반환한다. 0은 NET_CLOSED, 다른 소켓 오류는 NET_ERROR다. 버퍼가 모두 비면 NET_OK다.
- 오류/종료에 따른 Disconnect 호출은 추후 호출자인 GameClient에서 판단한다. 이 함수는 바이트 송신과 결과 반환만 맡는다.
- 검증: 기존 client Debug x64 MSBuild 성공. 임시 테스트 `%TEMP%/NGP-network-1103-flush` 실행 PASS.
- 실제 loopback 소켓으로 패킷 두 개의 바이트·순서 일치, QueuePacket 직후 아직 미송신, 빈 버퍼 재호출 시 send 미호출을 확인했다.
- 부분 송신과 WSAEWOULDBLOCK은 테스트에서 send 결과를 모의하여 검증했다. 첫 2바이트 처리 뒤 일시 불가 → 다음 호출에서 남은 바이트만 보내며 중복/유실이 없음을 확인했다. 0/오류 반환과 해제 후 재접속 시 이전 요청 제거도 확인했다. 프로덕션 코드에 테스트용 장치는 추가하지 않았다.
- 기존 게임 코드와 사용자 주석은 수정하지 않았다. ReceiveData, recvBuffer, ExtractPacket, GameClient·Main 연결은 아직 구현하지 않았다. 커밋·푸시는 사용자 요청 시 진행한다.

## 11/3 세 번째 단계: ReceiveData 추가

- 사용자가 FlushSendBuffer 설명을 이해한 뒤, 수신 단계의 목적과 변경 파일 설명을 듣고 ReceiveData 구현을 요청했다. 이번 승인 범위는 수신 바이트 누적까지다.
- NetworkClient.h에 `NET_RESULT ReceiveData()`와 `std::vector<unsigned char> recvBuffer`를 추가했다. 기존 코드 주석은 보존하고 새 부분에 `[11.3]` 표시를 사용했다.
- ReceiveData는 연결 확인 후 4096바이트 임시 배열로 recv를 한 번 호출한다. 양수 반환이면 실제 받은 바이트만 recvBuffer 뒤에 복사하고 NET_OK를 반환한다. 한 번에 모두 읽지 못한 데이터는 다음 호출에서 이어 받는다.
- WSAEWOULDBLOCK은 NET_WOULD_BLOCK, 0은 서버의 정상 종료를 나타내는 NET_CLOSED, 다른 소켓 오류는 NET_ERROR다. 기존 수신 바이트는 이 결과들로 삭제하지 않는다. 연결 종료와 오류에 대한 Disconnect 판단은 이후 호출자가 맡는다.
- Disconnect에 recvBuffer.clear()를 추가해 재접속 시 이전 연결의 바이트가 섞이지 않게 했다. Shutdown도 Disconnect를 통해 두 버퍼를 비운다.
- 검증: 기존 client Debug x64 MSBuild 빌드 성공. 임시 테스트 `%TEMP%/NGP-network-1103-receive` 컴파일·실행 PASS. Windows SDK 접근 제한으로 빌드와 테스트 실행은 권한 승인 후 수행했다.
- 실제 loopback 소켓으로 미연결 결과, 데이터 없는 상태의 WSAEWOULDBLOCK, 패킷 일부 보관 후 나머지와 다음 패킷 바이트 누적, 0/128/255를 포함한 바이트 순서, 10000바이트를 여러 호출로 수신할 때의 무손실 누적, 서버 정상 종료 시 기존 바이트 보존, 해제·재접속 시 수신 버퍼 초기화, Shutdown 정리를 확인했다. 소켓을 테스트에서 무효화하여 일반 recv 오류 반환과 기존 바이트 보존도 확인했다.
- 테스트의 버퍼 확인 장치는 임시 테스트 파일에만 두었고 게임 프로젝트에는 추가하지 않았다. ExtractPacket, GameClient·Main 연결은 구현하지 않았다. 실제 팀 서버·게임 UI 통합은 미검증이다. 커밋·푸시는 하지 않았다.
- 다음 설명 대상: 임시 배열과 recvBuffer의 차이, recv 반환값, 실제 받은 길이만 vector::insert로 누적하는 문법. 이후 구현은 사용자 요청에 맞춰 진행한다.

### 기존 주석 보존 재확인

- 사용자가 기존 주석을 건드리지 말고 다시 작업하도록 요청했다. ReceiveData 추가 직전에 확인한 기존 코드 주석은 현재도 유지되어 있으며, 이전부터 있던 사용자 수정은 되돌리지 않았다.
- 새 ReceiveData 구현 블록을 NetworkClient.cpp 맨 아래로 옮겼다. 이동한 블록 외 기존 코드와 주석은 이동 전후 바이트가 동일함을 확인했다. 함수 내용과 동작은 변경하지 않았다.

### ReceiveData 본문만 다시 추가
- 사용자가 저장한 최신 FlushSendBuffer 주석을 기준으로 NetworkClient.cpp 끝에 ReceiveData 본문만 추가했다. 추가 전 파일 전체가 추가 후에도 바이트 단위로 그대로 유지됨을 확인했다. 헤더와 Disconnect 등 기존 함수는 수정하지 않았다.
- 이번 요청에서는 Disconnect의 recvBuffer 초기화를 추가하지 않았다. 앞선 기록의 버퍼 초기화 검증은 이전 구현에 대한 기록이다. 현재 변경은 Debug x64 MSBuild 빌드 성공으로 확인했다.

### 11/3 범위 코드 검토
- 사용자 요청으로 현재 NetworkClient.h/.cpp를 추진계획서와 LLD에 대조했다. QueuePacket / FlushSendBuffer / ReceiveData 선언·정의와 두 버퍼는 모두 존재한다. ExtractPacket은 11/4 범위다.
- 발견 1: Disconnect는 sendBuffer만 비우고 recvBuffer를 비우지 않는다. 이전 연결의 수신 바이트가 재접속 후에도 남을 수 있다. 현재 코드로 기존 임시 loopback 테스트를 다시 컴파일·실행해 "FAIL: disconnect clears receive buffer"를 재현했다. 이 검사 이전의 부분 수신, 바이트 누적·순서, 여러 호출 수신, 정상 종료 시 보존 검사는 통과했다.
- 발견 2: LLD는 ReceiveData를 WSAEWOULDBLOCK까지 반복 수신하도록 설명하지만 현재는 recv 한 번 후 NET_OK를 반환한다. 데이터는 다음 호출로 이어 받을 수 있으나 설계와 차이가 있다.
- 이전 범위 추가 확인: ConnectToServer의 TCP_NODELAY 설정이 현재 없다. LLD의 연결 절차와 다르며, 사용자가 이전에 수정한 영역이므로 임의 복원하지 않았다.
- 검증: 현재 client Debug x64 MSBuild 성공. 이번 검토에서는 소스 코드와 기존 주석을 수정하지 않았고 검토 결과만 문서 끝에 기록했다.

### 검토 항목 1 수정: 연결 종료 시 수신 버퍼 초기화
- 사용자 요청으로 Disconnect에 recvBuffer.clear() 한 줄만 추가했다. 추가한 줄을 제외한 기존 소스 전체가 바이트 단위로 동일함을 확인했고 기존 주석은 수정하지 않았다.
- 현재 소스를 포함한 임시 수신 테스트를 다시 컴파일·실행해 PASS를 확인했다. 이전에 실패한 연결 종료 후 수신 버퍼 초기화와 재접속 시 이전 바이트 제거 검사도 통과했다.
- ReceiveData의 반복 수신 방식과 TCP_NODELAY 설정은 변경하지 않았다. 반복 수신과 한 번 수신의 차이는 사용자에게 설명하는 단계다.

### ReceiveData를 반복 수신으로 변경
- 사용자 요청으로 ReceiveData 내부에 while 반복문을 추가해 WSAEWOULDBLOCK이 나올 때까지 수신 바이트를 recvBuffer에 누적한다. 기존 주석 문구는 보존했고 다른 함수와 헤더는 수정하지 않았다.
- receivedAny로 이번 호출의 수신 여부를 기록한다. WSAEWOULDBLOCK 시 한 번이라도 받았으면 NET_OK, 처음부터 받은 바이트가 없으면 NET_WOULD_BLOCK을 반환한다. 0과 일반 오류는 각각 NET_CLOSED / NET_ERROR를 반환하며 누적된 바이트는 유지한다.
- 검증: client Debug x64 MSBuild 성공. 임시 loopback 테스트에서 10000바이트가 소켓에 준비된 것을 확인한 뒤 ReceiveData 한 번으로 전부 읽는지 검증했다. 2000바이트만 준비된 경우에도 4096바이트가 채워질 때까지 기다리지 않고 반환함을 확인했다. 빈 수신, 분할 데이터 누적·순서, 정상 종료, 오류, 재접속·Shutdown 버퍼 정리 검사도 PASS다.

- 사용자 요청으로 ReceiveData의 WSAEWOULDBLOCK 처리에서 삼항 연산자를 if문으로 변경했다. 기존 주석과 반환 동작은 유지했다.

### 11/3 송신·수신 기능 커밋
- 사용자 승인에 따라 송신 기능(QueuePacket / FlushSendBuffer)과 수신 기능(ReceiveData / 수신 버퍼 초기화)을 두 커밋으로 나눈다. 커밋 대상은 해당 NetworkClient 기능과 진행 기록이며, 별도로 수정된 기존 주석·연결 설정과 다른 게임 파일·문서는 작업 폴더에 유지한다. 현재 CJH 브랜치에서 origin/CJH로 푸시한다.


### 11/4 첫 단계 ExtractPacket 구현

- 사용자 요청으로 client/NetworkClient.h/.cpp에 `NET_RESULT ExtractPacket(unsigned char* out)` 선언과 정의를 추가했다. 추진계획서의 함수 형태를 따르고 새 코드에는 [11.4] 표시를 사용했다. 기존 함수와 사용자 주석은 보존했다.
- 호출자는 `unsigned char packet[BUF_SIZE]`처럼 BUF_SIZE(256) 이상인 출력 배열을 전달한다. 첫 바이트 size가 PacketHeader 크기(2) 이상인지 확인하고, size만큼 수신됐다면 해당 바이트만 출력 배열에 복사한 뒤 recvBuffer에서 제거한다. 여러 패킷은 반복 호출로 하나씩 꺼낸다.
- NET_OK는 추출 성공, NET_WOULD_BLOCK은 빈 버퍼 또는 패킷 수신 부족, NET_ERROR는 null 출력 포인터 또는 잘못된 길이(0/1)다. 성공한 경우에만 버퍼와 출력을 변경한다. 길이 오류를 받은 호출자는 이후 연결 오류 처리 단계에서 대응한다.
- 이미 받은 바이트를 처리하므로 IsConnected 검사 없이 수신 버퍼를 확인한다. 패킷 type별 구조체 크기 검사와 게임 처리는 이후 GameClient 단계에서 담당한다.
- 검증: 실제 NetworkClient.cpp를 함께 컴파일한 임시 C++ 테스트 PASS. 길이 2/7/255의 모든 분할 위치, 빈 입력, 헤더 일부, 여러 패킷과 뒤쪽 미완성 패킷, 출력 범위 및 원본 바이트 일치, 잘못된 길이, null 출력, 연결 소켓 없이 보관된 패킷 추출을 확인했다. 버퍼 접근 장치는 임시 테스트에만 있으며 프로젝트에는 추가하지 않았다.
- 테스트 위치: %TEMP%/NGP-network-1104-extract-vuhumiuo. 임시 파일 생성·컴파일·실행은 샌드박스 권한 승인 후 수행했다.
- 전체 client Debug x64 MSBuild는 현재 체크아웃에 external/vcpkg_installed/x64-windows가 없어 GLM/freeglut 헤더를 찾지 못해 실패했다. NetworkClient.cpp 컴파일과 별도 테스트의 컴파일·링크·실행은 성공했다. 게임 실행 및 실제 팀 서버 연동은 이번 검증에 포함하지 않았다.
- 이번 구현은 ExtractPacket까지다. GameClient Init/TryConnect 및 Main 연결은 다음 사용자 요청에 맞춰 단계별로 진행한다.


### ExtractPacket 주석을 쉬운 말로 수정
- 사용자 요청으로 새로 추가한 ExtractPacket의 헤더와 함수 내부 주석을 쉬운 말로 바꿨다. 출력 배열, 패킷 전체 길이, 덜 받은 데이터 보관, 복사 후 꺼낸 부분만 지우는 과정을 설명했다.
- 연결 종료 확인은 ReceiveData가 맡고 ExtractPacket은 이미 받은 데이터만 확인한다는 설명을 추가했다.
- 주석과 공백을 제외한 두 소스 파일의 내용이 수정 전후 동일함을 확인했다. 함수 동작은 바꾸지 않았으며 주석 변경이므로 빌드와 실행 테스트는 반복하지 않았다.


### 11/4 GameClient Init 구현

- 사용자가 GameClient::Init까지만 구현하도록 요청했다. 새 client/GameClient.h/.cpp를 만들고 기존 client.vcxproj와 filters에 등록했다. 이번 단계는 [11.4] 표시와 쉬운 한국어 주석을 사용했다.
- 추진계획서의 CLIENT_STATE 다섯 상태와 Init에 필요한 currentState, mySessionId, myRoomId, mySlot, serverIp, errorMessage, NetworkClient network만 추가했다. Inventory/InputHandler/월드 객체 등은 해당 기능 단계에서 추가한다.
- Init(const char* ip)은 연결 중 재초기화와 null/빈 IP를 거절한다. 서버 주소 문자열을 복사하고 NetworkClient::InitNetwork로 Winsock을 준비한 뒤 CLIENT_LOGIN과 번호 -1(미정)을 설정하고 이전 오류 메시지를 지운다. 실패하면 errorMessage를 저장하고 false를 반환한다. 실제 IPv4 형식 검사는 이후 ConnectToServer에서 담당한다.
- Init에서는 서버 연결을 시도하지 않는다. TryConnect, GameClient 종료 함수와 Main 연동은 아직 추가하지 않았으며 다음 사용자 요청에 따라 진행한다.
- 검증: 기존 프로젝트 Debug x64의 GameClient.cpp 선택 컴파일(MSBuild ClCompile) 성공, 경고 없음. 새 파일을 UTF-8 BOM으로 저장해 한글 주석의 코드 페이지 경고를 해결했다.
- 별도 임시 테스트 .vs/GameClientInitCheck에서 실제 GameClient.cpp와 NetworkClient.cpp를 컴파일·링크·실행해 PASS. null/빈 IP 거절, IP 문자열 복사 보관, Winsock 준비 후 미연결 상태, 초기 게임 상태, 반복 Init의 WSAStartup 중복 방지, 연결 중 재초기화 거절 및 상태 보존, WSAStartup 실패와 재시도를 검증했다. 실패 경로는 테스트에서만 WSAStartup 호출을 대체했고 성공 경로는 실제 Winsock을 사용했다.
- 현재 GameClient::Shutdown은 이후 일정이므로 테스트 정리는 테스트에서만 network.Shutdown을 호출했다. 전체 게임 빌드는 앞서 확인한 GLM/freeglut 의존성 누락으로 이번 단계에서 반복하지 않았으며, 게임 실행과 실제 서버 접속은 미검증이다.
- 기존 NetworkClient의 코드와 사용자 주석은 수정하지 않았다. NetworkClient.h에 이미 있던 공백 줄의 trailing whitespace도 보존했다. 커밋과 푸시는 하지 않았다.


### 11/4 GameClient TryConnect 구현

- 사용자 요청으로 GameClient.h/.cpp에 bool TryConnect() 선언과 정의를 추가했다. 새 부분은 [11.4] 표시와 쉬운 한국어 주석을 사용했으며 사용자가 수정한 Init 및 기존 멤버 주석은 보존했다.
- 이미 연결되어 있으면 기존 소켓과 게임 상태를 유지하고 오류 메시지만 지운 뒤 true를 반환한다. 저장한 serverIp가 없으면 Init을 먼저 호출하라는 오류 메시지를 저장하고 false를 반환한다.
- network.ConnectToServer(serverIp.c_str())로 기본 SERVER_PORT(9000)에 접속한다. 접속 실패 시 서버 IP가 포함된 오류 메시지를 저장하고 false를 반환한다. 초기화 여부와 IPv4 형식, 소켓 오류 처리는 NetworkClient가 담당한다.
- 새 접속에 성공하면 currentState를 CLIENT_LOGIN으로 설정하고 오류 메시지를 지운 뒤 true를 반환한다. 로그인 패킷 송신은 이후 단계다. 최초 연결은 기존 ConnectToServer 설계대로 blocking connect다.
- 검증: 기존 client 프로젝트 Debug x64의 GameClient.cpp 선택 컴파일 성공, 경고 없음.
- .vs/GameClientConnectCheck의 임시 테스트를 실제 GameClient.cpp와 NetworkClient.cpp로 컴파일·링크·실행해 PASS. Init 전 호출, 잘못된 IPv4, 실제 loopback TCP 접속, 중복 호출 시 소켓 재사용과 상태 보존, 접속 거절 후 소켓 정리·오류 기록, 이후 재시도 성공과 오류 제거를 확인했다.
- 테스트는 다른 서버와 충돌하지 않도록 OS가 선택한 loopback 임시 포트를 사용했다. 테스트 내부에서만 connect 호출의 목적지 포트를 바꾸고 원래 요청 포트가 SERVER_PORT인지 검사했다. 프로덕션 코드에는 테스트 장치를 추가하지 않았다.
- 전체 게임 빌드는 앞서 확인한 GLM/freeglut 누락으로 반복하지 않았다. 실제 팀 서버와 게임 UI 연결은 미검증이다. Main, PollNetwork, Tick, 로그인 처리, GameClient 종료 연결은 이번 단계에서 추가하지 않았다. 커밋과 푸시는 하지 않았다.


### 11/5 PollNetwork 송신·수신 단계 구현

- 사용자가 PollNetwork부터 요청했다. 아직 없는 ProcessPacket과의 의존성을 설명하고 이번에는 송신·수신·오류 처리부터 구현한 뒤 다음 단계에서 패킷 추출·전달을 연결하는 순서를 확인받았다.
- GameClient.h/.cpp에 void PollNetwork() 선언과 정의만 추가하고 [11.5]와 쉬운 한국어 주석을 사용했다. 연결 전에는 바로 반환한다. FlushSendBuffer로 보낼 데이터를 전송하고 ReceiveData로 받은 바이트를 recvBuffer에 누적한다.
- NET_WOULD_BLOCK은 오류로 취급하지 않는다. 송신이 잠시 막혀도 수신은 시도하며, 보내지 못한 데이터는 다음 PollNetwork 호출에서 다시 보낸다. 정상 수신 데이터는 이번 단계에서 추출하거나 버리지 않고 버퍼에 보관한다.
- 송신·수신의 NET_CLOSED/NET_ERROR는 원인을 errorMessage에 저장하고 network.Disconnect로 소켓과 버퍼를 정리한다. 게임 상태와 월드 전체의 연결 종료 처리는 이후 HandleDisconnect 단계에서 연결한다.
- 이번 함수는 단계별 구현의 중간 상태다. 다음 ProcessPacket 단계에서 ExtractPacket 반복 호출과 패킷 전달을 추가해야 한다. 현재 종료 경로는 Disconnect가 버퍼를 비우므로, 그 단계에서는 수신 종료를 감지해도 완성된 마지막 패킷을 먼저 처리한 뒤 연결을 정리하도록 순서를 확장해야 한다. 현재 Main에서는 호출하지 않는다.
- 검증: 기존 client 프로젝트 Debug x64의 GameClient.cpp 선택 컴파일 성공, 경고 없음. .vs/PollNetworkCheck의 임시 테스트 컴파일·링크·실행 PASS.
- 실제 loopback TCP로 송신 바이트 일치, 분할 수신 누적과 데이터 보관, 받을 데이터가 없을 때 연결 유지, 서버의 정상 종료를 확인했다. 테스트 내부에서만 send/recv 결과를 대체해 송신 WSAEWOULDBLOCK 중 수신 진행·재송신, 송신 0/오류 및 수신 오류의 연결 정리와 오류 메시지를 검증했다.
- 이번 작업 전 코드와 비교해 기존 GameClient 코드·사용자 주석이 바이트 단위로 보존됐고 NetworkClient.h/.cpp가 바뀌지 않았음을 확인했다. 기존에 사용자가 바꾼 TryConnect 헤더 주석도 유지했다.
- 전체 게임 빌드는 기존 GLM/freeglut 의존성 누락으로 반복하지 않았다. 팀 서버·UI 통합, ProcessPacket, Tick, Main 및 종료 흐름 연결은 이번 범위에 포함하지 않았다. 커밋과 푸시는 하지 않았다.


### PollNetwork 종료·오류 주석 추가
- 사용자 요청으로 송신·수신의 NET_CLOSED와 NET_ERROR 분기에 연결 종료, 송신 오류, 서버 정상 종료, 수신 오류를 짧게 설명하는 주석 네 줄을 추가했다. 기존 코드와 사용자 주석은 보존했으며 동작 변경은 없다.


### 11/5 ProcessPacket 검사·분기 틀과 PollNetwork 패킷 전달 연결

- 사용자 승인 범위는 ProcessPacket의 기본 검사·종류별 분기 틀과 PollNetwork의 패킷 추출·전달 연결까지다. GameClient.h/.cpp에 private void ProcessPacket(const unsigned char* packet)을 추가하고 새 부분에 [11.5] 표시와 쉬운 한국어 주석을 사용했다.
- ProcessPacket은 ExtractPacket으로 완성된 패킷을 전달받는다. null 또는 헤더 길이 부족, 알 수 없는 서버 패킷 종류(C2S 포함), type별 구조체와 일치하지 않는 size는 오류 메시지를 저장하고 연결을 정리한다.
- protocol.h의 서버 패킷 13종을 switch로 구분해 각각 sizeof(S2C_...)와 전체 길이를 비교한다. 아직 각 Handle 함수의 본문은 없으므로 정상 패킷은 검사까지만 하고 소비한다. 로그인 결과·방·위치 등 실제 게임 상태 반영은 이후 일정에서 추가한다.
- PollNetwork는 ReceiveData 다음에 ExtractPacket을 반복 호출해 NET_WOULD_BLOCK까지 완성된 패킷을 ProcessPacket으로 전달한다. 미완성 패킷은 수신 버퍼에 남기며 추출 길이 오류는 연결 정리로 처리한다.
- 수신 NET_CLOSED/NET_ERROR 처리를 패킷 추출 루프 뒤에 두어 그 호출에서 이미 수신한 완성된 패킷부터 처리한 뒤 연결을 정리한다. 끝에 남은 미완성 데이터는 종료된 연결에서 더 채울 수 없으므로 Disconnect와 함께 비워진다. 패킷 검사에서 연결을 끊었으면 즉시 반환해 그 오류 이유를 수신 종료 메시지로 덮어쓰지 않는다.
- 검증: 기존 client 프로젝트 Debug x64의 GameClient.cpp 선택 컴파일 성공, 경고 없음. .vs/ProcessPacketCheck 임시 테스트 컴파일·링크·실행 PASS.
- 서버 패킷 13종의 정상 길이 및 각각 짧거나 긴 size, null·짧은 헤더, 알 수 없는 type 및 C2S 거절을 확인했다. 실제 loopback TCP로 분할 수신 보관, 여러 패킷의 순서·일회 전달과 미수신 시 기존 완성 패킷 처리를 확인했다.
- 테스트 내부에서만 send/recv 결과를 대체해 송신 WSAEWOULDBLOCK 중 패킷 처리, 수신 0/오류 직전 완성 패킷 처리와 마지막 미완성 데이터 제외, 잘못된 패킷 뒤 처리 중단 및 오류 메시지 보존을 검증했다. ProcessPacket 호출 관찰 코드는 임시 사본에만 넣었으며 실제 소스에는 테스트 장치를 추가하지 않았다.
- 작업 직전 GameClient의 모든 기존 줄과 주석이 순서대로 보존됐고 NetworkClient.h/.cpp가 바이트 단위로 동일함을 확인했다. 공용 protocol.h와 server, 원본 게임 코드는 수정하지 않았다.
- 전체 게임 빌드는 기존 GLM/freeglut 누락으로 반복하지 않았다. 팀 서버·UI 연동과 Tick은 아직 구현·검증하지 않았다. 커밋과 푸시는 하지 않았다.


### PC에서 이어하기 메모 2026-11-05

- 사용자 요청: 다른 접속 환경에서 돌아와 이어갈 수 있도록 현재 상태를 기록한다. 이후 작업을 한꺼번에 구현하지 않고 목적·변경 파일을 먼저 설명한 뒤 한 단계씩 진행한다. 사용자가 수정한 기존 코드·주석은 보존하고 새 코드는 쉬운 한국어 주석과 해당 날짜 표시를 사용한다.
- 현재 브랜치는 CJH, 원격은 origin/CJH다. 11/4까지는 f9d7ffd(패킷 추출)와 c700a41(GameClient Init/TryConnect) 두 커밋으로 푸시 완료다.
- 이 이어하기 메모를 처음 작성한 시점에는 11/5 작업이 미커밋·미푸시였고 추가 커밋 요청도 없었다. 이후 사용자가 학습 방식 기록과 11/5 작업의 커밋·푸시를 요청했다. 최신 범위는 아래 커밋 기록을 따른다.
- PollNetwork 구현: 연결 확인, 송신 버퍼 전송, 수신 누적, ExtractPacket 반복 호출 및 ProcessPacket 전달, 종료·오류 시 연결 정리. 송신 WOULD_BLOCK이어도 수신을 계속한다. 수신 CLOSED/ERROR를 처리하기 전에 이미 받은 완성 패킷부터 처리하며, 패킷 오류 이유를 종료 메시지로 덮어쓰지 않는다.
- ProcessPacket은 private void 함수다. 서버 패킷 13종의 type을 switch로 구분하고 sizeof(S2C_...)와 전체 size를 비교한다. 잘못된 헤더·종류·길이는 오류를 저장하고 연결을 정리한다. 정상 패킷은 현재 검사까지만 하고 소비하며 각 Handle 함수의 게임 상태 반영은 아직 구현하지 않았다.
- 검증 완료: 기존 프로젝트 Debug x64 GameClient.cpp 선택 컴파일, 임시 ProcessPacket 테스트. 13종의 정상·잘못된 크기, 분할·연속 수신 순서, 마지막 패킷 처리 순서, 오류 메시지 보존을 확인했다. 전체 게임 빌드는 기존 GLM/freeglut 누락으로 미확인이며 팀 서버·UI 통합도 아직 하지 않았다.
- 바로 다음 후보는 11/5 Tick(const KeyManager& keys)다. 아직 선언·정의하지 않았고 InputHandler, UpdateVisuals 등 의존 기능도 없다. 사용자가 돌아오면 목적과 이번 최소 구현 범위를 먼저 설명하고 요청에 맞춰 진행한다. Main 연결, 로그인·방·이동 처리, GameClient 종료 흐름을 임의로 추가하지 않는다.
- 사용자는 Init, TryConnect, ExtractPacket, PollNetwork와 ProcessPacket의 기본 목적을 설명받았다. 합성(has-a)과 상속(is-a), 클래스와 인스턴스 차이도 이해했다. 설명과 커밋 본문은 쉬운 말로 간결하게 작성한다.
- 커밋 양식은 feat(client): 한글 기능 설명이며 함수명을 괄호에 적는다. 제목과 본문은 구분하고 본문은 함수별 역할 한 줄 정도로 짧게 적는다. 커밋·푸시는 해당 단계에 대한 요청을 받으면 CJH에 진행한다.


### 11/5 학습 방식 저장 및 커밋·푸시 요청

- 사용자는 목적과 용도를 먼저 이해하고, 함수 하나씩 구현한 뒤 변수 의미와 코드를 쉬운 말로 설명받으며 진행하는 학습 방식을 선호한다. 이해했다고 확인하고 다음 작업을 요청하기 전에는 임의로 단계를 확장하지 않는다.
- 기존 사용자 주석을 보존하고 새 주석은 짧고 쉽게 적는다. 낯선 개념은 현재 프로젝트 코드와 간단한 예시로 설명한다. 커밋 제목은 feat(client): 한글 기능 설명(함수명), 본문은 함수별 역할 한 줄 정도로 간결하게 적는다.
- 위 선호와 최신 구현 범위를 저장소 루트 AGENTS.md에도 반영해 다른 PC의 다음 작업에서도 확인할 수 있게 했다. 이 문서에는 이어하기용 상세 진행 상태와 테스트 결과가 있다.
- 사용자 요청으로 이번 11/5 작업과 학습·이어하기 기록을 하나의 커밋에 포함해 CJH에서 origin/CJH로 푸시한다. 포함 파일은 client/GameClient.h, client/GameClient.cpp, docs/CLIENT_PROGRESS.md, AGENTS.md다.
- 커밋 제목: feat(client): 통신 처리(PollNetwork) 및 패킷 검사(ProcessPacket) 기능 구현
- 구현 범위는 PollNetwork의 송수신·패킷 전달·오류 처리와 ProcessPacket의 서버 패킷 13종 검사·분기 틀까지다. Tick과 실제 Handle 함수, Main 연동은 남아 있다.
- 검증은 앞서 통과한 선택 컴파일과 패킷 검사·loopback 테스트 결과를 따른다. 이후 소스 변경은 사용자 주석뿐이므로 같은 테스트를 반복하지 않았다. 전체 게임 빌드의 GLM/freeglut 의존성 누락과 팀 서버·UI 미검증 상태는 유지한다.
