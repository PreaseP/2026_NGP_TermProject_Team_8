# 현재 상태: 기존 게임 이식 단계

현재 진행: 기존 일정의 11/4 접속·송수신·패킷 추출 및 GameClient Init/TryConnect까지 완료하고, 기존 11/5 PollNetwork와 ProcessPacket 검사·분기 틀을 구현한 상태다. 11/6 ReceivePackets 통합 시도는 사용자 요청으로 롤백했다. ReceiveData와 ExtractPacket 분리 구조 및 기존 11/4 완료 코드를 유지한다. 최신 문서는 남은 작업의 계획에 참고하며 Tick·로그인·실제 게임 상태 반영은 아직 구현하지 않았다.

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


### 2026-11-06 교수님 피드백 반영 설계·일정 문서 검토

- 사용자 요청으로 docs/8팀_조정현_클라이언트_리팩토링_개발일정.docx와 docs/Minecraft_Snowball_Fight_LLD_함수_리팩토링_v01.docx를 읽고 현재 코드와 대조했다. 이후 구현 계획은 이 최신 문서들을 기준으로 잡는다. 현재 소스는 기존 설계로 구현한 상태이며 이번 요청에서는 코드나 사용자 문서를 수정하지 않았다.
- 주요 통합: NetworkClient::ReceiveData + ExtractPacket -> NET_RESULT ReceivePackets(std::vector<PacketBytes>& out). PacketBytes는 std::vector<unsigned char> 별칭이다. 완성 패킷을 순서대로 반환하고 미완성 바이트를 보관하며, 종료되어도 이미 추출한 패킷은 결과로 전달한다.
- GameClient::PollNetwork의 수신 호출·반복 분배 -> Tick(const KeyManager& keys). 새 프레임 흐름은 경과 시간 계산 -> ProcessInput -> FlushSendBuffer -> ReceivePackets -> ProcessPacket 반복 -> UpdateVisuals -> Render 요청이다. 송신 종료는 즉시 정리하고 수신 종료는 받은 패킷을 반영한 뒤 정리한다. 새 LLD의 GameClient 함수 목록에는 PollNetwork가 없다.
- 로그인 외 요청 8종은 template<class Packet> bool QueueRequest(Packet packet)로 통합한다. RequestLogin은 닉네임 검사·재접속을 담당하므로 별도로 유지한다.
- 단순 응답 처리 함수들은 ProcessPacket의 분기로, 오브젝트·피격·아이템 등 경기 중 응답 처리 함수들은 ApplyWorldPacket으로 합친다. HandleGameStartResult는 별도 유지한다. 이동 입력 보조 함수는 ProcessInput으로 합친다.
- 새 동작 규칙 차이: ConnectToServer는 기존 연결을 닫고 새로 접속한다(현재 코드는 이미 연결 중이면 거절). GameClient::Init은 입력 연결과 초기 접속 시도까지 담당한다(현재는 준비만). ProcessPacket의 알 수 없는 종류는 무시한다(현재는 연결 종료). 알려진 종류의 길이 검증을 삭제하라는 내용은 새 문서에 없다.
- 공통 선언은 common.h에 NET_RESULT, INVALID_ID(-1), NetUtil, Inventory를 두는 설계다. 현재 common에는 protocol.h만 있으므로 공통 담당 영역과의 연결 범위를 후속 구현 전에 확인해야 한다. protocol.h의 패킷 이름과 구조는 그대로 사용한다.
- 새 일정: 11/2 접속·종료, 11/3 QueuePacket/FlushSendBuffer/ReceivePackets, 11/4 Init/TryConnect/Tick/ProcessPacket 프레임 루프, 11/5 OnKeyDown/Reset/RequestLogin, 11/6 ProcessPacket 로그인 결과·HandleDisconnect·로그인 렌더·OnExit 정리 연결, 11/7 분할 수신·연결 종료·로그인 오류 점검.
- 이후 일정: 11/9 방 목록, 11/10~12 QueueRequest와 입장·퇴장·캐릭터·시작 요청, 11/13 경기 시작 응답, 11/16~28 이동·공격·월드 표시, 11/30~12/3 아이템·결과·복귀, 12/4 실제 서버 통합, 12/9 최종 검증·제출. 통합 전까지 테스트 응답을 사용한다.
- 코드 정리의 첫 후보는 ReceiveData/ExtractPacket을 ReceivePackets로 통합하는 단계다. 그다음 PollNetwork 흐름을 새 Tick으로 옮기고 ProcessPacket 규칙을 맞춘 뒤 로그인 기능으로 이어간다. 사용자 이해 확인·요청 없이 구현을 시작하지 않는다.
- 문서에서 추가로 명확히 할 부분: 새 GameClient 함수 목록에는 기존 설계의 Shutdown이 없으나 OnExit은 네트워크 자원 정리 함수를 호출하도록 되어 있다. private network의 Shutdown에 어떤 공개 경로로 연결할지는 종료 단계에서 정해야 한다.
- 이번에는 검토 결과만 기록했다. 새 설계의 코드 구현·테스트와 문서 커밋·푸시는 하지 않았다. 새 DOCX 두 개는 현재 미추적 파일 상태다.


### 2026-11-06 기존 구현 점검 및 최신 설계 반영 1단계 수신 통합

- 사용자 요청은 기존 일정의 11/4 완료 및 11/5 진행 상태를 최신 일정에 맞춰 정리하는 것이다. 한 단계씩 이해하는 작업 방식에 따라 이번에는 ReceiveData/ExtractPacket 통합과 그 호출부만 수정했다. 새 로그인 기능은 추가하지 않았다.
- NetworkClient.h/.cpp의 기존 두 공개 함수를 NET_RESULT ReceivePackets(std::vector<PacketBytes>& outPackets)로 합쳤다. PacketBytes는 클라이언트 헤더에 둔 std::vector<unsigned char> 별칭이며 공용 common.h가 준비되면 공통 선언과 맞춘다.
- ReceivePackets는 호출 시작에 이전 출력 목록을 비우고, 수신 버퍼에서 완성된 패킷을 순서대로 목록에 복사한다. 덜 받은 바이트는 남겨 이어 받고, 0/1처럼 헤더보다 짧은 길이는 NET_ERROR다. 네트워크 계층은 type별 구조체 검사를 하지 않는다.
- 이번 호출에서 바이트를 받았거나 패킷을 꺼냈으면 NET_OK, 아무것도 처리하지 못한 상태에서 WSAEWOULDBLOCK이면 NET_WOULD_BLOCK이다. 수신 종료/오류는 NET_CLOSED/NET_ERROR로 반환하되 그 전에 꺼낸 패킷 목록을 유지한다. 소켓 정리 판단은 호출자가 맡는다.
- PollNetwork는 ReceivePackets 결과 목록을 순서대로 ProcessPacket에 전달하고 그 뒤 수신 종료·오류를 처리하도록 바꿨다. 잘못된 패킷으로 ProcessPacket에서 연결을 끊으면 그 오류 이유를 보존한다. ProcessPacket의 실제 게임 상태 반영은 여전히 미구현이다.
- 기존 주석 문구를 모두 보존했다. 통합 전 인터페이스 설명은 과거 기록임을 표시했고 바뀐 호출에는 보충 주석을 붙였다. NetworkClient/GameClient 네 파일의 기존 주석이 빠지지 않았음을 비교 검사했다. protocol.h, server와 원본 게임은 수정하지 않았다.
- 검증: 기존 프로젝트 Debug x64의 NetworkClient.cpp 및 GameClient.cpp 선택 컴파일 성공, 경고 없음. .vs/ReceivePacketsRefactor의 독립 수신 테스트와 GameClient 연계 테스트 컴파일·실행 PASS.
- 수신 테스트는 길이 2/7/255의 모든 분할 위치, 여러 패킷과 미완성 꼬리, 이전 출력 초기화, 종료·오류 전 패킷 보존, 잘못된 길이, 기존 버퍼의 완성 패킷, 실제 TCP로 4096바이트 경계를 넘는 700패킷의 순서·내용을 검증했다. 연계 테스트는 서버 패킷 13종 검사, 분할·연속 전달, 송신 backpressure, 마지막 패킷 선처리 및 오류 메시지 보존을 재검증했다.
- 남은 설계 차이: ConnectToServer에서 기존 연결을 닫고 새 접속, Init에서 입력 대상 연결과 첫 접속 시도, ProcessPacket의 알 수 없는 종류 무시, PollNetwork 흐름의 Tick 통합이다. QueuePacket/FlushSendBuffer/Disconnect/Shutdown은 기존 핵심 구현을 재사용할 수 있다.
- 다음 단계 후보는 ConnectToServer 재접속 동작 조정이며, 목적과 코드 차이를 설명하고 사용자 이해·요청에 맞춰 진행한다. Tick을 새 일정의 11/4에 넣었다고 해서 입력·렌더·로그인 전체를 이번 단계에 한꺼번에 추가하지 않는다.
- 전체 게임 빌드는 기존 GLM/freeglut 의존성 누락으로 미확인이다. 사용자 수정 DOCX 두 개는 보존했고 이번 코드·기록 변경 및 새 문서의 커밋·푸시는 하지 않았다.


### 2026-11-06 ReceivePackets 통합 롤백 및 진행 기준 재확인

- 사용자 결정: 방금 적용한 수신·추출 통합 방식은 취소하고 기존 일정의 11/4까지 완료한 코드를 그대로 유지한다. 새 문서의 일정과 클라이언트 설계를 다시 읽어 남은 작업을 파악한다.
- .vs/ReceivePacketsRefactor의 수정 직전 백업과 현재 차이를 확인한 뒤 NetworkClient.h/.cpp, GameClient.h/.cpp 네 파일을 정확히 복원했다. 복원 전 통합 버전은 .vs/ReceivePacketsRollback에 임시 보관했다. 전체 Git reset이나 기존 커밋 변경은 하지 않았다.
- 복원 결과: ReceiveData와 ExtractPacket이 각각 선언·정의되고 PollNetwork가 두 함수를 호출하는 기존 반복 흐름으로 돌아왔다. PacketBytes 별칭과 ReceivePackets 함수, 해당 호출과 보충 주석은 이번 롤백으로 제거됐다. 통합 전에 이미 작성한 PollNetwork/ProcessPacket 검사 틀과 사용자 주석은 그대로 보존됐다.
- 검증: 네 파일이 통합 직전 백업과 바이트 단위로 동일하고 client 소스는 현재 HEAD 대비 차이가 없음을 확인했다. NetworkClient.cpp와 GameClient.cpp 선택 컴파일 성공, 경고 없음. 기존 패킷 검사·loopback·분할 수신·마지막 패킷 선처리 회귀 테스트 PASS.
- 수정 일정 문서와 리팩토링 LLD의 클라이언트 전체(구성·함수·요청 조건·응답 처리·콜백·프레임 흐름)를 다시 읽었다. 문서에는 여전히 ReceivePackets가 있으나, 이번 사용자 결정이 우선이며 그 통합안을 다시 자동 적용하지 않는다.
- 또한 기존 일정의 11/4 코드 유지 결정에 따라 ConnectToServer를 기존 연결 종료 후 재접속으로 바꾸거나 Init에서 최초 접속을 시도하도록 하는 이전 제안은 현재 진행 대상으로 삼지 않는다. 해당 동작을 바꾸려면 사용자와 별도 범위를 정한다.
- 최신 문서의 남은 흐름: Tick의 입력·통신·패킷 반영·화면 갱신 연결, 11/5 닉네임 입력·RequestLogin, 11/6 로그인 응답·오류·HandleDisconnect·로그인 화면·OnExit 연결, 11/7 접속·로그인 점검이다. 기존 진행 날짜와 새 일정의 날짜를 혼동해 완료 판정을 하지 않는다.
- 향후 미구현 요청 함수는 RequestLogin을 별도로 두고 그 외 8종은 QueueRequest로, 기본 응답은 ProcessPacket으로, 경기 중 변경은 ApplyWorldPacket으로, 이동 입력은 ProcessInput으로 모으는 문서 방향을 참고한다. 실제 추가·통합은 한 단계씩 설명하고 사용자 요청에 따라 진행한다.
- 기존 11/4 구현을 유지하면서 새 프레임 흐름에 연결할 때는 ReceiveData 후 ExtractPacket 반복 호출로 완성 패킷을 전달할 수 있다. Tick과 PollNetwork의 역할 정리는 별도 단계이며 이번에 구현하지 않았다.
- 기존 11/6 수신 통합 기록과 다음 ConnectToServer 수정 제안은 이 롤백 결정으로 대체된다. 새 DOCX 두 개는 수정하지 않았고 현재 미추적 상태다. 이번 변경은 진행·학습 기준 기록만 남으며 커밋과 푸시는 요청받지 않아 수행하지 않았다.


### 2026-11-06 클라이언트 최종 개발 일정 작성

- 사용자 요청으로 원래 추진계획서 PDF, 수정 달력 일정 DOCX, 수정 LLD의 클라이언트 부분을 합쳐 docs/8팀_조정현_클라이언트_최종_개발일정.docx를 새로 만들었다. 원본 두 DOCX와 추진계획서 PDF는 수정하지 않았다.
- 확정 조건: 기존 기록의 11/4까지 날짜와 함수 배치는 그대로 고정한다. 10/30 InitOpenGL, 10/31 입력·타이머·종료 준비, 11/2 접속·종료 다섯 함수, 11/3 QueuePacket/FlushSendBuffer/ReceiveData, 11/4 ExtractPacket 및 GameClient Init/TryConnect다.
- 사용자는 11/5 이후의 기존 일정 배치를 전부 재구성해도 된다고 했다. 이는 이번 일정 문서 작성 범위이며 실제 11/5 소스를 삭제하거나 되돌리지는 않았다. ReceivePackets 통합은 재도입하지 않고 기존 ReceiveData/ExtractPacket 방식으로 계획했다.
- 새 배치: 11/5 Tick/ProcessPacket과 main·타이머 흐름 연결, 11/6 닉네임 입력·RequestLogin, 11/7 로그인 결과·화면, 11/9 HandleDisconnect·OnExit·접속 점검, 11/10 로비 목록, 11/11 공통 QueueRequest와 방 입장, 11/12 퇴장·대기실, 11/13 캐릭터·시작 요청, 11/14 경기 시작 응답·방 흐름 점검이다.
- 11/16 이후는 수정 LLD의 ProcessInput, QueueRequest, ApplyWorldPacket, UpdateVisuals를 중심으로 이동·공격·월드·아이템을 배치했다. 12/2 결과, 12/3 복귀·재접속, 12/4 실제 서버 통합, 12/5 한 판·재시작, 12/7~8 통합 디버깅, 12/9 최종 검증·제출이다. 일요일 휴식은 유지했다.
- 형식은 기존 달력의 가로 용지, 일~토 7열, 표 색상·테두리·글꼴과 제목·역할 범례를 유지했다. 원본이 Word에서 세 쪽으로 밀리는 문제는 글자 크기를 유지한 채 제목 간격과 미래 주의 최소 행 높이를 조정해 두 쪽으로 정리했다.
- 검증: 41개 날짜의 2026년 요일 정렬, 11/4까지 함수 목록 일치, 필요한 클라이언트 함수 포함, 허용하지 않은 ReceivePackets 및 개별 요청 함수 미등장, 원본 DOCX 해시 보존을 확인했다. ZIP의 document.xml 외 모든 부분은 바이트 단위로 보존했다.
- Word의 읽기 전용 PDF 출력과 Windows PDF 페이지 렌더링으로 두 페이지를 확인했다. 제공된 render_docx.py는 pdf2image 의존성과 작업 런타임 로더가 없어 사용하지 못해 native Word 렌더링으로 검증했다. 최종 Word 문서만 전달하며 QA 이미지와 PDF는 .vs/FinalSchedule1106 안에 보관한다.
- 앞으로 일정 기준은 새 최종 DOCX다. 코드와 학습 진행 상태는 이 문서 및 AGENTS.md를 함께 확인한다. 이번에는 코드 변경이나 커밋·푸시를 하지 않았다.

### 2026-11-06 후속 일정 조정 및 최종 확정 — 기존 기록 보존

이 절은 앞선 '클라이언트 최종 개발 일정 작성' 이후 사용자와 확정한 변경 사항이다. 앞선 기록은 당시의 이력으로 유지하며, 향후 일정은 아래 내용을 우선한다. 실제 함수 구현 상태는 코드 확인 결과와 구분한다.

#### 최신 문서 위치와 변경 이유

- 최종 전달본: `C:/Users/USER/Documents/ChatGPT/넷겜플/output/documents/8팀_조정현_클라이언트_개발일정_상단정리.docx`.
- 저장소의 `docs/8팀_조정현_클라이언트_최종_개발일정.docx`는 이번 후속 수정 이전 문서다. 최신 문서를 저장소에 복사하거나 기존 DOCX를 덮어쓰지는 않았다.
- 교수님 피드백에 맞춰 리팩토링 함수명과 구현 날짜를 연결하되, 팀원 달력처럼 간결하게 줄였다. 반복되는 통합 함수에는 로그인·방 목록·이동 등 짧은 기능 구분만 붙인다.
- 사용자 시험기간 때문에 12월 기능 개발은 1·3·5일로 조정했다. 테스트는 격일 제한에서 제외한다. 작업 없는 날은 '휴식/시험 준비' 문구 없이 날짜만 남겼다.
- 상단 범례는 N: NetworkClient / G: GameClient / I: InputHandler만 남긴다. 전역 함수 접두어 F는 삭제했으며 Inventory와 drawScene의 상단 설명도 삭제했다. 달력 본문의 Inventory 및 drawScene 함수는 유지한다.

#### 코드·커밋 확인 결과와 계획 날짜의 차이

- `65d639f`(10/31)는 솔루션과 프로젝트 파일 추가다. 사용자 본인의 키보드·타이머 콜백 신규 구현 기록으로 해석하면 안 된다.
- `07481ee`(11/2)에 기존 게임 이식, InitOpenGL 분리, OnExit 추가·종료 정리가 포함된다. 키보드·타이머 콜백은 기존 게임 코드다.
- 처음에는 이 커밋 날짜에 맞춰 InitOpenGL/OnExit를 11/2로 옮겼으나, 사용자가 최종적으로 두 함수를 10/31에 배치하도록 지정했다. 최종 달력은 10/30 빈칸, 10/31 InitOpenGL/OnExit다. 이는 계획 날짜 조정이며 커밋 날짜를 변경한 것이 아니다.
- 기존 일정의 11/4까지 함수 구현은 코드에서 확인했다: InitOpenGL/OnExit, NetworkClient의 InitNetwork/ConnectToServer/IsConnected/Disconnect/Shutdown/QueuePacket/FlushSendBuffer/ReceiveData/ExtractPacket, GameClient의 Init/TryConnect.
- 추가로 11/5 PollNetwork와 ProcessPacket의 패킷 검사·분기 틀이 구현돼 있다. 관련 최근 커밋은 `962ef27`이다. ProcessPacket은 아직 로그인·월드 상태를 실제로 갱신하지 않는다.
- Tick, InputHandler의 새 네트워크 입력 처리, 로그인·방·인게임 반영 및 Main과의 네트워크 통합은 남은 작업이다. 함수 구현 완료와 게임 전체 연동 완료를 혼동하지 않는다. 이번 확인에서 새 빌드나 실행 테스트를 수행한 것은 아니다.

#### 유지할 구조와 리팩토링 범위

- ReceiveData/ExtractPacket 분리 및 PollNetwork를 유지한다. 이전 ReceivePackets 통합은 롤백 상태이며 재도입하지 않는다. Tick을 만들 때 기존 PollNetwork를 활용하는 범위를 먼저 설명하고, 두 곳에서 송수신을 중복 호출하지 않도록 한다.
- 로그인은 RequestLogin, 나머지 요청 8종은 QueueRequest, 로그인·방·경기 종료 응답은 ProcessPacket, 객체·상태·피격·아이템 응답은 ApplyWorldPacket으로 구현한다. HandleGameStartResult와 HandleDisconnect는 별도 유지한다. 이동 입력 보조 처리는 ProcessInput으로 묶는다.
- 변경내역 문서의 클라이언트+GLUT 함수 수는 54→32지만, 현재의 ReceiveData/ExtractPacket 분리 및 PollNetwork 유지 예외가 있으므로 이 수치를 현재 코드의 최종 함수 수로 단정하지 않는다. 이전 대화의 33개 계산은 수신 분리만 고려한 가정이었다. 두 예외를 모두 유지하고 나머지 명세가 동일하다면 설계상 34개가 되며 실제 구현 시 별도 집계가 필요하다.
- Inventory는 공통으로 사용할 아이템 보관·추가·제거·보유 확인·초기화 자료형이다. 현재 common에는 protocol.h만 있고 Inventory/common.h 구현은 확인되지 않았다. protocol.h에는 ITEM_TYPE, MAX_ITEMS=2 및 아이템 패킷이 있다. LLD의 공통 자료형 계획을 이미 구현된 코드로 설명하지 않는다.
- 공통이라는 뜻은 관리 규칙과 코드를 재사용한다는 의미다. 서버와 클라이언트는 각각 데이터를 가진다. 서버가 지급·사용 가능 여부를 판정하고 클라이언트는 응답을 반영한다.
- drawScene은 Main.cpp의 기존 전역 렌더 콜백이며 glutDisplayFunc(drawScene)에 등록된다. LLD의 Render에 해당하는 실제 함수명으로 달력에 표시했다. 새로운 렌더 함수를 임의로 추가하는 계획이 아니다.

#### 최종 일정 요약

N=NetworkClient, G=GameClient, I=InputHandler. 아래 날짜는 계획 기준이며 완료 판정은 위 코드 확인 결과를 따른다. 전역 함수는 접두어가 없다.

| 날짜 | 작업 |
| --- | --- |
| 10/30 | 빈칸 |
| 10/31 | InitOpenGL, OnExit — 그래픽 초기화·종료 정리 |
| 11/1 | 빈칸 |
| 11/2 | N: InitNetwork, ConnectToServer, IsConnected, Disconnect, Shutdown |
| 11/3 | N: QueuePacket, FlushSendBuffer, ReceiveData — 송수신 버퍼 |
| 11/4 | N: ExtractPacket / G: Init, TryConnect |
| 11/5 | G: PollNetwork, ProcessPacket — 송수신·패킷 검사 |
| 11/6 | G: Tick, RequestLogin / I: OnKeyDown, Reset / main, TimerFunction, Keyboard, KeyboardUp — 프레임 연결·로그인 입력 |
| 11/7 | G: ProcessPacket / drawScene — 로그인 결과 |
| 11/8 | 빈칸 |
| 11/9 | G: HandleDisconnect / OnExit — 접속·로그인 점검 |
| 11/10 | G: ProcessPacket / drawScene — 방 목록 |
| 11/11 | G: QueueRequest, ProcessPacket / I: OnKeyDown — 방 입장 |
| 11/12 | G: QueueRequest, ProcessPacket / I: OnKeyDown / drawScene — 퇴장·대기실 |
| 11/13 | G: QueueRequest / I: OnKeyDown — 캐릭터 선택·시작 |
| 11/14 | G: HandleGameStartResult, ClearWorld / I: Reset — 시작 응답·방 점검 |
| 11/15 | 빈칸 |
| 11/16 | I: ProcessInput / G: QueueRequest / SpecialKeyboard, SpecialKeyboardUp — 이동 |
| 11/17 | G: ApplyWorldPacket — 이동 좌표 |
| 11/18 | G: UpdateVisuals, Tick / drawScene — 보간·화면 연결 |
| 11/19 | G: ApplyWorldPacket, UpdateVisuals — 팔 상태 |
| 11/20 | Mouse / I: OnMouse / G: QueueRequest — 차징·발사 |
| 11/21 | 이동·공격 입력 테스트 |
| 11/22 | 빈칸 |
| 11/23 | G: ApplyWorldPacket — 눈덩이 생성 |
| 11/24 | G: ApplyWorldPacket, UpdateVisuals — 눈덩이 이동·제거, 로컬 물리·충돌 정리 |
| 11/25 | G: ApplyWorldPacket / drawScene — 피격·체력 |
| 11/26 | G: ApplyWorldPacket, UpdateVisuals / drawScene — 눈 벽 |
| 11/27 | I: OnKeyDown / G: QueueRequest / Inventory: Has — 아이템 요청 |
| 11/28 | 인게임 표시·아이템 테스트 |
| 11/29 | 빈칸 |
| 11/30 | G: ApplyWorldPacket / Inventory: Add, Remove / drawScene — 아이템 획득·사용 |
| 12/1 | G: ApplyWorldPacket / drawScene — 방어막·공격 강화 |
| 12/2 | 빈칸 |
| 12/3 | G: ProcessPacket / I: Reset / drawScene — 경기 결과 |
| 12/4 | 서버 연동 테스트 |
| 12/5 | G: ProcessPacket, ClearWorld, HandleDisconnect, TryConnect — 복귀·재접속 |
| 12/6 | 빈칸 |
| 12/7 | 1차 디버깅 및 테스트 |
| 12/8 | 2차 디버깅 및 테스트 |
| 12/9 | 최종 디버깅 및 테스트 진행 후 제출 |

#### 이어서 작업할 때

- 코드 구현은 사용자가 요청한 함수·단계만 진행한다. 일정에 여러 함수가 같은 날 적혀 있어도 한꺼번에 구현하지 않는다. 기존 주석과 커밋·디스코드 기록은 보존한다.
- 이 채팅에서는 일정 DOCX를 수정하고 코드·커밋을 읽었을 뿐, 실제 게임 코드는 수정하지 않았다. 이번 인계 기록 갱신으로 구현 완료 상태가 달라지지 않는다.
- 최신 DOCX는 내용·함수 소속·XML 수정 여부를 확인했지만 렌더러 부재로 실제 페이지 잘림은 미검증이다. 앞선 저장소 DOCX의 Word 렌더 결과를 이 최신 파일의 검증으로 대체하지 않는다.
- MD 최신화는 기존 내용을 보존한 추가 기록 방식으로 수행한다. 코드·DOCX 원본 변경, 커밋·푸시는 이번 요청에 포함하지 않는다.

### 2026-11-08 구현 — 11/6 첫 단계: Tick 추가

- 사용자 요청에 따라 한 단계만 구현했다. GameClient.h/.cpp에 void Tick() 선언과 정의를 추가했다.
- Tick은 주기적인 클라이언트 처리의 진입점이며 현재는 기존 PollNetwork()를 한 번 호출한다. 연결 여부 확인과 송수신·패킷 검사는 기존 PollNetwork가 담당한다.
- ReceiveData/ExtractPacket 분리 구조와 기존 주석을 유지했다. Tick에서 송수신을 중복 호출하지 않는다.
- Main.cpp 및 TimerFunction에는 아직 연결하지 않았다. 따라서 게임 실행 중 Tick이 자동 호출되는 상태는 아니다. 로그인 입력·RequestLogin·상태 반영도 아직 미구현이다.
- 검증: 변경 차이와 선언·정의 일치를 확인했다. 이번 단계에서 빌드·실행 테스트는 수행하지 않았다.
- 다음 단계는 기존 main의 초기화·접속 및 TimerFunction의 Tick 호출 연결 범위를 설명하고 사용자 요청에 따라 진행하는 것이다. 커밋·푸시는 하지 않았다.

### 2026-11-08 구현 — 11/6 두 번째 단계: 시작·타이머 연결

- 사용자 요청에 따라 Main.cpp에 GameClient 객체를 추가하고, 그래픽 초기화 성공 후 Init("127.0.0.1") → TryConnect()를 호출하도록 연결했다. 같은 PC 서버를 기본 대상으로 사용하며 포트는 기존 SERVER_PORT(9000)를 따른다.
- 초기화 실패 시 접속은 시도하지 않는다. 초기화·접속 실패는 콘솔에 표시하고 기존 화면은 계속 실행한다. 자동 재접속은 추가하지 않았다. 최초 접속은 기존 blocking connect 방식이다.
- TimerFunction 맨 앞에서 Tick()을 한 번 호출한다. 타이틀 화면의 조기 반환보다 앞에 두어 화면에 관계없이 통신을 처리한다. Tick → PollNetwork 구조로 송수신 중복 호출을 피했다.
- GameClient.h는 OpenGL 헤더보다 먼저 포함해 Winsock 헤더 순서를 지켰다. 기존 게임 로직·주석과 ReceiveData/ExtractPacket 분리 구조는 보존했다.
- 검증: git diff --check 통과. Debug x64 전체 빌드를 시도했지만 기존 GLM(glm/glm.hpp) 및 freeglut(gl/freeglut.h) 의존성 누락으로 실패했다. GameClient.cpp 컴파일에는 오류가 보고되지 않았으나 Main.cpp 컴파일·링크·실제 접속과 화면 실행은 검증하지 못했다.
- 로그인 입력·RequestLogin·로그인 응답 반영은 아직 미구현이다. OnExit의 네트워크 Shutdown 연결도 11/9 작업으로 남아 있으며, 현재 시작·통신 연결을 전체 수명 관리 완료로 해석하지 않는다.
- 다음 단계는 RequestLogin의 패킷 생성·전송 대기열 등록을 설명하고 한 함수씩 진행하는 것이다. 커밋·푸시는 하지 않았다.

### 2026-11-08 OpenGL 헤더·라이브러리 경로 복구

- 사용자 요청으로 GL 헤더 오류를 확인했다. 프로젝트가 참조하는 external/vcpkg_installed/x64-windows가 이 PC에 없어 GLM 및 freeglut 헤더를 찾지 못했다.
- 원본 게임의 external에서 GLM·GLEW·freeglut 헤더, x64 DLL·import lib를 현재 저장소의 external/local_opengl로 복사했다. 원본 저장소는 수정하지 않았다. 복사한 SDK는 .gitignore에 등록해 로컬 의존성으로 유지한다.
- client.vcxproj는 기존 vcpkg SDK가 없고 로컬 SDK가 있을 때만 해당 경로를 사용한다. 로컬 SDK는 Release DLL/import lib를 Debug에서도 사용하며, freeglut의 Debug 라이브러리 자동 지정은 FREEGLUT_LIB_PRAGMAS=0으로 끄고 프로젝트에서 직접 링크한다. DLL은 기존 빌드 후 복사 작업을 통해 실행 폴더에 배치된다.
- 검증: client Debug x64 전체 컴파일·링크 성공. 실행 폴더에 glew32.dll/freeglut.dll 존재 확인. 기존 형변환 경고는 남아 있다. 실제 게임 실행 및 Visual Studio 편집기의 빨간 줄 해소 여부는 직접 확인하지 않았다.
- 게임 소스·서버·공용 프로토콜은 이번 경로 수정에서 변경하지 않았다. 이전 Tick·main 연결 변경은 유지했다. 커밋·푸시는 하지 않았다.

### 2026-11-09 구현 — 11/6 세 번째 단계: RequestLogin

- 사용자 요청에 따라 GameClient.h/.cpp에 bool RequestLogin(const char* name) 한 함수를 추가했다. 공용 프로토콜 및 리팩토링 LLD의 로그인 조건을 확인했다.
- 로그인 상태 확인 → 빈 닉네임 및 영문·숫자 1~15자 검사 → 필요 시 기존 TryConnect로 재접속 → C2S_Login 생성 → QueuePacket 등록 순서다. MAX_NAME_LEN=16 중 마지막 칸은 널 문자용으로 남긴다.
- 패킷을 0으로 초기화하고 size/type/name을 채운다. true는 송신 버퍼 등록 성공이며 로그인 성공이나 실제 전송 완료를 뜻하지 않는다. 실제 송신은 기존 Tick → PollNetwork → FlushSendBuffer 경로에서 수행한다.
- 입력 검사는 접속 시도 전에 진행한다. 재접속 실패 시 TryConnect의 오류 메시지를 유지한다. 로그인 응답에 의한 상태 전환과 중복 요청 대기 제어는 이번에 추가하지 않았다.
- 검증: Debug x64 전체 빌드·링크 성공. 기존 게임의 형변환 경고만 남는다. git diff --check 통과. 실제 서버 로그인 및 입력 연동 테스트는 아직 수행하지 않았다.
- InputHandler와 Keyboard에는 아직 연결하지 않아 키 입력으로 요청이 발생하지 않는다. 다음 단계는 닉네임 입력을 관리할 InputHandler를 한 단계씩 구현하는 것이다. 기존 주석·Main·NetworkClient·protocol.h는 이번 작업에서 수정하지 않았다. 커밋·푸시는 하지 않았다.

### 2026-11-09 구현 — 11/6 네 번째 단계: InputHandler 기본 틀과 Reset

- 사용자와 합의한 한 단계 범위로 InputHandler.h/.cpp를 새로 만들고 기존 client.vcxproj 및 filters에 등록했다.
- 리팩토링 LLD에 맞춰 game 포인터, previousKeys, attackPressed, nameBuffer[MAX_NAME_LEN], nameLength를 초기값과 함께 선언했다. game은 아직 nullptr이며 GameClient 소유·입력 전달 연결은 다음 단계다.
- Reset은 설계대로 이동 기록(previousKeys=0)과 공격 버튼 상태(attackPressed=false)만 초기화한다. 닉네임과 game 포인터는 유지한다. 앞선 일반적인 입력 초기화 설명을 닉네임 삭제로 해석하지 않는다.
- nameBuffer는 객체 생성 시 0으로 초기화되고 nameLength는 0이다. 아직 닉네임 추가·삭제·Enter 처리나 OnKeyDown, 실제 이동·공격 처리는 구현하지 않았다. Main.cpp, GameClient 및 기존 KeyManager도 이번 단계에서는 수정하지 않았다.
- 검증: 처음 컴파일에서 새 파일의 UTF-8 무BOM을 CP949로 해석하는 오류를 확인해 새 두 파일만 UTF-8 BOM 및 CRLF로 저장했다. 이후 Debug x64 컴파일·링크 성공, 이번 증분 빌드 경고 없음. git diff --check 통과. 아직 호출 연결이 없어 실제 입력 동작 검증은 하지 않았다.
- 다음 단계는 OnKeyDown의 로그인 입력 처리와 GameClient에 요청을 전달할 연결을 설명하고 사용자 요청에 맞춰 진행한다. 커밋·푸시는 하지 않았다.

### 2026-11-09 구현 — 11/6 다섯 번째 단계: OnKeyDown 로그인 입력

- InputHandler::OnKeyDown(unsigned char key)를 추가했다. 요청 대상이 없거나 로그인 상태가 아니면 반환한다. 영문·숫자 최대 15자 입력, Backspace 삭제, Enter의 RequestLogin 호출을 구현했다. 빈 문자열 삭제와 최대 길이 입력에서도 널 종료를 유지한다.
- RequestLogin이 false이면 닉네임을 그대로 두고 반환한다. 자동 반복·종료는 없으며 다음 Enter 호출로 다시 요청할 수 있다. true여도 서버 응답 전 상태를 변경하거나 닉네임을 지우지 않는다. 오류 문자열은 기존 GameClient에 저장되지만 화면 표시는 아직 없다. 응답 대기 중 중복 Enter 억제는 아직 구현하지 않았다.
- GameClient가 InputHandler inputHandler{this}를 소유하도록 연결했다. InputHandler 생성자는 대상 주소를 기억하고, GameClient::GetState() const는 입력 처리에 필요한 현재 상태만 읽도록 추가했다. 기존 Init/TryConnect 구현 및 사용자 수정 주석은 보존했다.
- 검증: Debug x64 전체 빌드·링크 성공. 기존 게임 코드의 형변환 경고만 남는다. git diff --check 통과. 실제 키보드·서버 실행 테스트는 하지 않았다.
- Main.cpp의 Keyboard는 아직 연결하지 않았다. 따라서 실행 중 키를 눌러도 새 입력 처리기가 호출되는 상태는 아니다. 다음 단계는 기존 콜백의 종료 키·게임 시작 키와 로그인 입력이 충돌하지 않도록 설명한 뒤 Keyboard 연결을 진행하는 것이다. 커밋·푸시는 하지 않았다.

### 2026-11-09 구현 — 11/6 여섯 번째 단계: Keyboard 연결

- 사용자 요청으로 Main.cpp의 Keyboard/KeyboardUp만 로그인 입력에 맞춰 연결했다. GameClient.h에 입력 처리기로 전달하는 공개 OnKeyDown 래퍼를 추가했다.
- 수정 직후 위치: Main.cpp 508~509줄에서 ESC는 항상 종료, Q/q는 로그인 상태가 아닐 때만 종료하도록 조건을 변경했다. 기존 종료 주석은 보존하고 새 예외 설명을 붙였다.
- Main.cpp 515~520줄에서 CLIENT_LOGIN이면 gameClient.OnKeyDown(key)를 호출하고 재출력을 요청한 뒤 반환한다. 따라서 1/3은 기존 로컬 게임 시작 대신 닉네임으로 입력되며 게임 조작에 중복 전달되지 않는다.
- Main.cpp 589~590줄에서 로그인 중 KeyboardUp은 즉시 반환하여 기존 이동 키 처리·눈덩이 발사를 실행하지 않는다. GameClient.h 18줄의 OnKeyDown은 InputHandler::OnKeyDown으로 전달만 한다.
- 현재 로그인 결과에 따른 상태 전환은 미구현이므로 CLIENT_LOGIN에 머문다. 기존 타이틀 화면 및 1/3 조작 안내는 아직 남지만 해당 키로 로컬 게임을 시작하지 않는다. 닉네임·오류 메시지 화면 표시도 미구현이며 화면 표시·서버 응답은 다음 단계다.
- 검증: Debug x64 전체 빌드·링크 성공, 기존 형변환 경고만 유지. git diff --check 통과. 실제 창의 키 입력·실제 서버 로그인은 이번에 실행 검증하지 않았다. 커밋·푸시는 하지 않았다.

### 2026-11-09 구현·검증 — 11/6 초기화 연결 및 입력·송신 점검

- 사용자 요청으로 GameClient::Init의 상태 초기화 부분에 inputHandler.Reset() 한 줄을 연결했다. 기존 주석은 보존했다. 닉네임과 입력 대상은 Reset에서 유지한다.
- client Debug x64 빌드·링크 성공. .vs/Login1106에만 임시 테스트를 작성하고 실제 GameClient/InputHandler/NetworkClient 소스를 컴파일해 로컬 TCP 서버로 검증했다. 테스트용 private 접근은 임시 파일에만 있으며 게임 프로젝트에는 추가하지 않았다.
- PASS: Init에서 이동·공격 기록 초기화, 빈 이름 Enter 거절, player의 각 글자 입력 후 Tick에서도 미전송, Backspace 및 금지 문자 무시, Enter 직후 버퍼 등록만 수행, 다음 Tick에서 정확한 C2S_Login 크기·종류·player 문자열 수신, 중복 전송 없음, 서버 응답 전 CLIENT_LOGIN 유지.
- PASS: 빈 이름에서 Backspace 반복 안전, q/Q/1/3 입력 허용, 15자 제한·널 종료, Reset에서 닉네임·대상 유지, 로그인 외 상태에서 글자 편집 무시. 테스트 서버·소켓·Winsock은 테스트 종료 시 정리했다.
- 11/6의 예정 함수 구현·연결 및 입력 처리기부터 TCP 전송까지 점검 완료. 실제 GLUT 창의 물리 키 입력·ESC 종료와 팀 서버 로그인은 이번 테스트 범위가 아니다. 11/7 로그인 결과 처리·닉네임/오류 화면 표시, 11/9 OnExit 네트워크 정리는 여전히 미구현이다.
- 다음 단계는 11/7 ProcessPacket의 S2C_LOGIN_RESULT 실제 상태 반영이다. 커밋·푸시는 하지 않았다.

### 2026-11-09 사용자 요청 — 로그인 화면 이미지·시작 버튼·응답 처리

- 사용자가 기존 두 메뉴 버튼을 지운 눈 배경, 제목 스타일의 별도 GAME START PNG, 크기 애니메이션과 클릭으로 로비 진입을 요청했다. 현재 설계에 맞춰 첫 화면에서 닉네임을 입력하고 로그인 성공 응답 후 로비로 이동하는 흐름으로 구현했다.
- 내장 image_gen으로 TitleBackground.png 및 투명 GameStart.png를 생성해 client 폴더에 저장했다. 원본 Title.png는 보존했다. 최종 프롬프트와 파일 역할은 docs/LOGIN_SCREEN_ASSETS.md에 기록했다.
- title.cpp는 새 배경·버튼 로드 및 버튼 텍스처 정리만 연결했다. 기존 CP949 파일의 나머지 바이트와 주석은 보존했다. 새 titleLogin.cpp를 프로젝트에 등록해 2D 입력창·상태·로비 안내를 그린다. 기존 게임의 호환 OpenGL 컨텍스트에서 메뉴를 그린 뒤 GL 상태·행렬·프로그램을 복원한다.
- 버튼은 시간에 따라 ±3.5% 크기가 변하며, 투명 여백을 제외한 같은 크기와 창 비율로 클릭 판정을 한다. Main.cpp Mouse는 클릭을 기존 Enter 요청 경로로 전달하고 drawScene은 로그인 또는 로비 UI를 그린다. 기존 콘솔의 1/3 시작 안내도 로그인 안내로 변경했다.
- GameClient에 응답 대기 플래그를 추가했다. 중복 Enter·클릭과 대기 중 입력 변경을 막으며, 응답 실패 시 닉네임 유지·오류 표시·재시도 가능, 10초 무응답은 연결 종료 후 재시도 가능하게 했다. 로그인 상태에서 유효한 성공 응답을 받아야 세션 번호 저장 및 CLIENT_LOBBY 전환한다.
- 로비는 로그인 성공 및 닉네임 안내까지만 표시한다. 방 목록·선택·입장 기능은 이후 일정으로 남긴다. 기본 접속 대상은 127.0.0.1:9000이고 실제 서버가 없으면 실패가 표시되는 것이 정상이다.
- 검증: Debug x64 빌드 성공. .vs/Login1106 테스트로 TCP 로그인 패킷, 중복 요청 차단, 실패 응답 후 입력 유지, 성공 응답 세션 저장·로비 전환, 연결 종료 복귀, 10초 타임아웃을 확인했다.
- 실제 창에서 투명 PNG·닉네임 입력·버튼 크기 변화·로비 화면을 확인했다. 처음 실행은 숨김 창이라 화면이 보이지 않았고, 작업 디렉터리를 지정하고 창을 표시한 뒤 검증했다. 임시 UI 서버가 로그인 요청을 받아 성공 응답을 보냈으며 로비 표시를 확인한 후 테스트 서버를 종료했다. 실제 팀 서버 연동 결과는 아니다.
- 전체 HandleDisconnect 및 OnExit의 네트워크 Shutdown 연결은 후속 단계다. 커밋·푸시는 하지 않았다.

### 2026-11-09 화면 파일 구조 정리 및 pending 설명

- 사용자는 새 titleLogin.cpp를 두지 않고 기존 title.cpp 안에 기능을 추가하는 구조를 요청했다. 메뉴 함수들을 기존 title.cpp 끝에 옮기고 titleLogin.cpp 및 프로젝트·필터 등록을 제거했다. 기존 title.cpp의 원래 바이트·주석은 보존했고 추가 부분은 같은 CP949 인코딩으로 저장했다.
- 현재 위치: loadTitleTexture 53줄, loadStartTexture 214줄, startScale 245줄, hitsStart 249줄, renderLoginUI 256줄. 함수들은 같은 파일에서 각각 정의하며 함수 안에 함수를 정의하지 않는다.
- Debug x64 빌드·링크 성공. 화면·네트워크 동작은 이번 구조 정리에서 변경하지 않았다. 기존 형변환 경고 유지. 커밋·푸시는 하지 않았다.
- 사용자는 pending부터의 흐름을 아직 학습 중이다. loginPending은 로그인 요청 등록 후 서버 결과를 기다리는 동안 중복 요청을 막고 대기 UI를 표시하는 추가 보호 장치다. 필수 프로토콜 필드는 아니며 10초 타임아웃도 별도 보완 기능이다. 사용자 이해 확인 없이 다음 기능을 진행하지 않는다.

### 2026-11-09 사용자 요청 — 기존 셰이더 출력 구조로 통일

- 사용자는 loadTitleTexture와 같은 기존 구조를 유지하고 별도 고정 기능 렌더링 및 익명 namespace를 사용하지 않길 요청했다.
- title.cpp의 메뉴 출력에서 glUseProgram(0), glBegin, glMatrixMode, glRasterPos 및 GLUT 글자 그리기를 제거했다. panel/centeredText는 TitleScreen 멤버 함수로 옮기고 익명 namespace를 제거했다.
- 배경·버튼·패널·글자는 기존 shaderProgramID와 vao 및 model/view/projection을 사용하고 glDrawArrays로 그린다. drawUIQuad는 기존 사각형을 위치·크기만 바꿔 재사용한다. Main.cpp는 renderLoginUI에 shaderProgramID를 전달한다.
- 기존 vertex.glsl/fragment.glsl에 uiMode, uiUVRect, uiColor를 추가했다. uiMode일 때만 이미지 영역·색·알파를 적용하고 출력 후 false로 복원한다. 기존 게임 조명 경로는 보존했다.
- 글자는 Windows GDI로 시작 시 영문 글자 표를 메모리에 한 번 생성한 후 GPU 텍스처로 올린다. 실제 화면 출력은 버튼과 같은 셰이더·VAO를 사용한다. GDI는 화면 렌더러로 쓰지 않는다. 글꼴 표시가 기존 GLUT 비례 글꼴에서 Consolas로 바뀌었다.
- 현재 title.cpp 주요 위치: drawUIQuad 199줄, panel 213줄, loadFontTexture 219줄, centeredText 267줄, loadStartTexture 283줄, renderLoginUI 325줄. 새 코드 파일은 만들지 않았다.
- 검증: Debug x64 빌드·링크 성공. 실제 창에서 셰이더 컴파일 성공 및 배경·투명 시작 버튼·닉네임 안내·오류 문구 표시를 확인했다. 이번 변경은 표시 경로만이며 pending·로그인 패킷 처리는 변경하지 않았다. 커밋·푸시는 하지 않았다.

### 2026-11-09 사용자 요청 — 로그인 부가 기능 제거

- 사용자는 기본 요청·응답 흐름부터 이해하기 위해 부가 기능을 나중으로 미루도록 요청했다. loginPending/IsLoginPending, loginStarted/chrono, 중복 요청·입력 차단, 10초 타임아웃, 대기 문구·버튼 흐림을 제거했다.
- Tick은 기존처럼 PollNetwork() 호출만 남겼다. 앞서 추가한 연결 종료 시 로비→로그인 자동 복귀도 이번에 제거했으며 종료·재접속 흐름은 후속 HandleDisconnect 단계에서 다룬다.
- 닉네임 검사 → Enter/버튼 클릭 → C2S_Login 송신 → S2C_LoginResult 성공이면 세션 번호 저장 및 CLIENT_LOBBY 전환은 유지했다. 실패는 로그인 상태에서 오류를 표시한다. 로비는 임시 안내 화면이며 실제 방 목록 UI는 아직 없다.
- 닉네임은 로그인 패킷 본문으로 전송하고 클라이언트 IP/포트는 서버가 TCP 연결에서 확인한다. 별도의 주소 필드를 추가하지 않았다.
- 검증: Debug x64 빌드·링크 성공. .vs/Login1106 임시 테스트를 현재 기본 흐름에 맞춰 갱신해 TCP 로그인 송신, 실패 응답 후 로그인 유지, 성공 응답 후 세션 저장·로비 전환 PASS. 기존 중복 방지·타임아웃 테스트 결과는 제거 전 버전에 대한 기록이다. git diff --check 통과. 커밋·푸시는 하지 않았다.

### 2026-11-09 사용자 요청 — 닉네임 오류 시 GAME START 효과

- 사용자 요청으로 닉네임 검사 실패 시 GAME START 버튼에 약 0.45초 동안 붉은색과 빠른 좌우 흔들림을 적용했다. 효과 중에는 기존 크기 변화를 멈추고, 끝나면 멈춘 지점에서 이어서 재생한다. 새 기능은 로그인 화면 효과 한 단계로 한정했다.
- GameClient의 nameErrorEffect는 빈 닉네임·허용되지 않는 글자·길이 초과 또는 서버의 LOGIN_INVALID_NAME 응답에서 켜진다. ConsumeNameError가 이 값을 한 번 읽고 끄며, Main.cpp의 drawScene이 TitleScreen::PlayNameError를 호출한다. 접속 실패·서버 정원 초과는 닉네임 오류 효과를 발생시키지 않는다. 로그인 응답 대기 플래그·타임아웃은 다시 추가하지 않았다.
- title.h/.cpp에 효과 시작 시각과 누적 정지 시간, 남은 효과 세기와 좌우 이동 계산을 추가했다. 기존 uiColor에 붉은 색조를 전달하고 drawUIQuad의 x 위치를 움직인다. GLSL 변경 없이 기존 셰이더를 재사용했다. hitsStart도 같은 크기와 좌우 이동을 사용한다.
- 기존 InputHandler의 영문·숫자 필터와 15자 제한은 유지한다. 실행 화면에서 가장 간단한 확인 방법은 닉네임을 비우고 Enter 또는 GAME START를 누르는 것이다. 효과 중 다시 실패하면 처음부터 재생한다.
- 검증: client Debug x64 빌드·링크 성공. 기존 형변환 경고 외 새 컴파일 오류 없음. .vs/Login1106의 임시 검증에서 닉네임 오류의 1회 전달, 직접 호출한 잘못된 이름·길이 초과, 서버 이름 거절, 기존 TCP 로그인 성공 흐름 PASS. 실제 TitleScreen 코드의 시간 계산으로 색 복귀·좌우 이동·크기 정지/재개·연속 실패·클릭 영역 검증 PASS. 임시 검증 코드는 게임 프로젝트에 추가하지 않았다.
- 실제 게임 실행을 시도했으나 computer-use에서 조작 가능한 게임 창이 노출되지 않아 실제 화면의 시각 검증은 완료하지 못했다. 앞선 화면 검증 기록을 이번 효과의 검증으로 대체하지 않는다. 검증용으로 시작한 숨김 프로세스는 정리했다.
- 기존 사용자 주석·NetworkClient·공용 프로토콜·서버·이미지·셰이더는 이번 효과 작업에서 변경하지 않았다. 커밋·푸시는 하지 않았다. 다음 기능으로 진행하지 않고 사용자의 이해·요청에 맞춰 이어간다.

### 2026-11-09 후속 수정 — 빌드는 성공하지만 편집기에 표시된 오류

- 사용자가 Visual Studio 재시작 후에도 다수 오류가 남는데 빌드·실행은 된다고 알려 주었다. 실제 오류 목록을 확인한 결과, 빌드 오류가 아닌 IntelliSense의 E2429/E0067 및 연쇄적인 E0292/E0020이었다. 효과 시간의 static constexpr 선언 주변에서 클래스 분석이 어긋나 뒤의 vao/vbo/isInitialized 등을 없는 멤버로 표시했다. 정확한 분석기 내부 원인은 확정하지 않는다.
- title.h의 효과 시간 선언을 `static constexpr int START_ERROR_MS = 450`에서 `const int START_ERROR_MS = 450`으로 단순화했다. 효과 시간 값과 계산 동작은 같다. 캐시 삭제나 오류 표시 비활성화는 하지 않았다.
- 검증: 실제 Visual Studio의 전체 솔루션·빌드 + IntelliSense 오류 목록에서 검색 필터를 비운 상태로 오류 0개 확인. Debug x64 빌드·링크 성공 및 기존 효과 검증(색 복귀·좌우 이동·크기 정지/재개·연속 실패·클릭 영역) PASS. 기존 형변환 빌드 경고는 오류와 구분한다. 커밋·푸시는 하지 않았다.

### 2026-11-09 커밋 분리 — 일정 11/6 범위

- 사용자 요청에 따라 Tick, RequestLogin, InputHandler의 OnKeyDown/Reset 및 main·TimerFunction·Keyboard/KeyboardUp 연결과 InputHandler 프로젝트 등록만 하나의 커밋으로 분리한다.
- 로그인 응답의 상태 반영, 닉네임 화면·시작 버튼·오류 효과, 셰이더·이미지와 로컬 OpenGL SDK 경로 복구는 이번 커밋에서 제외하며 작업 폴더에 보존한다. 닉네임을 화면에 읽어 주는 GetName도 이후 화면 작업으로 남긴다.
- 이 커밋 단계는 서버 응답 전 CLIENT_LOGIN을 유지하는 입력·송신 단계다. 기존 타이틀의 1/3 안내는 남지만 로그인 입력에서 해당 키를 처리하므로 로컬 게임 시작으로 전달되지 않는다. 전체 네트워크 종료 연결은 이후 일정으로 남는다.
- 기존 작업 기록은 보존하며, 앞선 미구현·커밋 미실행 표기는 각 기록 작성 시점의 상태다. 이번에 선택한 11/6 코드만 별도 복사본에서 빌드·입력·TCP 송신 검증 후 CJH에서 origin/CJH로 커밋·푸시한다.

- 이번 커밋용 검증 완료: 선택한 11/6 코드 복사본의 Debug x64 전체 빌드·링크 성공(기존 형변환 경고 유지). 로컬 SDK는 임시 검증용 MSBuild 설정으로 연결했으며 그 설정은 커밋하지 않는다. 빈 이름·금지 문자·길이 검사, 입력 수정·Reset, Enter의 송신 버퍼 등록 및 다음 Tick의 정확한 TCP 송신, 11/7 로그인 응답 상태 반영이 제외되었음을 확인했다. 스테이징 내용과 검증 복사본의 일치 및 원래 작업 소스의 보존도 확인했다.

- 커밋·푸시 완료: 7b18292 (feat(client): 주기적 통신 및 닉네임 로그인 요청 연결(Tick, RequestLogin, OnKeyDown, Reset)). CJH에서 origin/CJH로 푸시했으며, 로그인 응답·화면·버튼 효과와 로컬 SDK 설정 등 이후 변경은 작업 폴더에 미커밋 상태로 유지했다.

### 2026-11-09 사용자 요청 — 닉네임 입력창 간소화

- title.cpp의 renderLoginUI에서 NICKNAME 제목, Type your nickname 기본 안내, Letters / numbers - 1 to 15 characters 문구를 제거했다. 이름이 비어 있을 때는 밑줄 커서만 0.5초 간격으로 깜빡이며, 입력하면 이름 뒤에 커서를 표시한다.
- 중앙 입력칸을 UI 좌표 기준 470×86에서 360×50으로 줄이고 중심을 유지했다. 입력 글자를 새 칸의 세로 중앙에 맞췄다. 이번 변경은 표시 부분만이며 기존 입력 검사·로그인·버튼 동작은 유지한다.
- 기존 title.cpp의 CP949 인코딩과 사용자 주석을 보존했다. 커밋·푸시는 하지 않는다.
- 검증: Debug x64 빌드·링크 성공, git diff --check 통과. 기존 형변환 경고는 유지되며 이번 변경 후 실제 게임 화면의 시각 확인은 아직 하지 않았다.

### 2026-11-09 커밋 분리 1/2 — 서버 로그인 결과와 화면 상태

- ProcessPacket에서 로그인 성공 시 서버 사용자 번호 저장·CLIENT_LOBBY 전환·입력 기록 초기화를 처리하고, 실패 시 닉네임을 유지하며 실패 이유를 저장한다. GetName/GetErrorMessage로 표시할 내용을 전달한다.
- drawScene은 로그인 상태 또는 임시 로비 안내를 출력한다. 이를 실제로 표시하는 공통 글자 출력 함수와 셰이더의 UI 분기를 함께 포함했다. Keyboard/Mouse는 로비에서 기존 로컬 게임 조작을 막는다. 방 목록·입장과 전체 연결 종료 처리는 이후 일정이다.
- 시작 버튼 이미지·클릭·크기 애니메이션·붉은색/흔들림과 입력창 패널 디자인은 두 번째 커밋으로 분리한다. 현재 작업 파일은 그대로 보존하고 첫 커밋만 별도 복사본에서 빌드·로그인 요청/응답을 검증한다.

### 2026-11-09 커밋 분리 2/2 — 로그인 화면과 시작 버튼 꾸미기

- 정리한 눈 배경과 투명 GAME START 이미지, 클릭으로 로그인 요청, 크기 애니메이션, 닉네임 오류 시 0.45초 붉은색/좌우 흔들림과 크기 정지·재개를 포함한다. 출력과 클릭 판정은 같은 버튼 위치·크기를 사용한다.
- 최종 닉네임 칸은 360×50이며 이름이 없으면 밑줄만 깜빡인다. NICKNAME 제목·기본 안내·글자 제한 문구는 제거한 현재 상태를 커밋한다. 앞선 기록의 더 큰 입력창·안내 문구 및 pending/타임아웃 설명은 해당 시점의 이력이다.
- 로컬 OpenGL SDK 경로 복구(client.vcxproj)와 SDK 제외 설정(.gitignore)은 두 기능 커밋에 섞지 않고 작업 폴더에 유지한다. 검증 복사본은 임시 MSBuild 설정으로 이 PC의 SDK를 연결하며, SDK 및 임시 테스트는 커밋하지 않는다.

- 두 커밋은 각각 별도 복사본으로 Debug x64 빌드·링크 및 로그인 요청/응답을 검증한 뒤 CJH에서 origin/CJH로 푸시한다. 두 번째는 붉은색 복귀·흔들림·크기 정지/재개·연속 실패·클릭 영역 검증도 수행한다. 원본 작업 소스·사용자 주석·서버·프로토콜은 보존한다.

- 검증 완료: 첫 번째와 두 번째 커밋용 복사본 모두 Debug x64 전체 빌드·링크 성공(기존 형변환 경고 유지). 각 복사본의 로컬 TCP 테스트에서 로그인 요청의 실제 송신, 실패 후 닉네임 유지, 성공 후 사용자 번호 저장·로비 전환을 확인했다. 두 번째 코드의 효과 검증도 PASS이며, 스테이징 내용과 검증 복사본이 일치하고 작업 소스가 보존되었음을 확인했다. 이번 커밋 분리 과정에서 실제 게임 창의 시각 검증을 새로 수행한 것은 아니다.
- 첫 번째 커밋: 302b78f — 서버 로그인 결과에 따라 화면 전환(ProcessPacket, drawScene). 두 번째는 위의 최종 입력창·버튼 디자인 상태를 담고, 두 커밋을 함께 origin/CJH로 푸시한다. 기존 기록의 미커밋·미푸시 표기는 각 기록 시점의 이력이다.

### 2026-11-09 최신 이어하기 기록 — 구현·커밋·푸시 상태 확정

- 사용자가 지금까지 기억할 내용을 저장해 달라고 요청해 AGENTS.md와 이 문서를 최신화했다. 이전의 Tick·로그인 미구현 또는 11/7 미커밋 표기는 과거 기록이다. 최신 상태는 아래 내용을 따른다.
- 11/6 완료 및 푸시: 7b18292 — Tick/PollNetwork의 주기적 통신, RequestLogin의 검사·패킷 생성·송신 버퍼 등록, InputHandler의 닉네임 입력·삭제·Enter/Reset 및 main·타이머·키보드 연결. 실제 송신은 다음 Tick → PollNetwork → FlushSendBuffer에서 한다. ReceiveData/ExtractPacket 분리 구조는 유지한다.
- 11/7 첫 커밋 완료 및 푸시: 302b78f — ProcessPacket 로그인 성공·실패 처리, 성공 시 서버 사용자 번호 저장·로비 상태 전환·입력 기록 초기화, 실패 시 이름 유지·오류 저장. GetName/GetErrorMessage와 drawScene의 상태별 화면 출력 및 로비 입력 제한을 포함한다. 공통 글자 출력과 셰이더 UI 분기도 독립 빌드에 필요한 공통 코드로 함께 포함했다.
- 11/7 두 번째 커밋 완료 및 푸시: 7ec6cc1 — 닉네임 입력 화면과 시작 버튼 꾸미기(renderLoginUI, PlayNameError). 배경·투명 버튼 이미지, 클릭 로그인, 크기 애니메이션, 오류 시 0.45초 붉은색·좌우 흔들림·크기 변화 정지/재개를 포함한다.
- 최종 화면: 360×50 입력칸에 닉네임과 깜빡이는 밑줄만 표시한다. NICKNAME 제목·Type your nickname·영문/숫자 길이 안내는 제거했다. 입력 검사는 영문·숫자 1~15자를 유지한다. 로그인 응답 대기 플래그·중복 요청 차단·10초 타임아웃은 제거한 상태다.
- 마지막 확인 당시 HEAD와 origin/CJH는 모두 7ec6cc1이며 미푸시 커밋은 없었다. 원격 푸시 성공도 확인했다. 게임 코드·이미지·셰이더·관련 개발 기록은 두 커밋에 포함되어 있다.
- 기록 갱신 전 남은 미커밋 변경은 .gitignore와 client/client.vcxproj의 로컬 OpenGL SDK 설정 두 파일이었다. SDK 폴더를 Git에서 제외하고 기존 vcpkg 경로가 없으면 이 PC에 복사한 SDK를 사용하게 한다. 이 설정을 사용자 요청 없이 커밋하거나 제거하지 않는다.
- 검증: 두 커밋을 각각 별도 복사본으로 Debug x64 빌드·링크하고 로컬 TCP로 로그인 성공·실패를 확인했다. 버튼의 색 복귀·흔들림·크기 정지/재개·반복 실패·클릭 영역 검증도 통과했다. 실제 팀 서버 연동은 미검증이다. 입력창 간소화 및 커밋 분리 후 실제 게임 창의 시각 검증을 새로 완료한 것으로 표시하지 않는다.
- 로비는 로그인 성공 안내까지만 있다. 실제 방 목록·방 입장·네트워크 게임 시작·월드 반영은 아직 없다. 최신 일정의 11/9 HandleDisconnect와 OnExit의 네트워크 Shutdown 연결도 미구현이다. 다음 단계는 사용자 요청을 받은 뒤 목적·수정 파일·함수 역할부터 짧게 설명한다.
- 사용자는 11/6 입력 → RequestLogin → QueuePacket → 다음 Tick의 실제 송신 흐름을 이해했다. C++에서 버튼 시간·위치·색을 계산하고 기존 GLSL uiMode/uiColor로 출력하는 관계도 설명했다. 이후 설명은 쉬운 말과 실제 함수명·파일·현재 줄 번호로 이어간다.
- 디스코드용 문구는 로그인 결과 처리와 화면 꾸미기를 나누어 작성했다. 1번에는 실제 함수명을 넣어 따라갈 수 있게 하고, 2번은 시작 버튼 애니메이션 등 화면 변화를 쉬운 말로 설명하는 방식을 선호한다. 디스코드에 직접 전송한 것은 아니다.
- 이번 요청은 로컬 기억 기록 저장만이다. 코드·이미지·문서는 추가 수정하지 않고 AGENTS.md와 CLIENT_PROGRESS.md에만 기록을 추가했다. 새 커밋·푸시는 하지 않았으므로 이 기록 두 파일도 미커밋 상태로 남는다.

### 2026-11-09 후속 요청 — 인계 기록 커밋 및 PC 이어하기

- 사용자가 인계 기록을 커밋한 뒤 다른 PC에서 이어가겠다고 요청했다. 이 문서와 AGENTS.md만 문서 커밋으로 만들어 CJH에서 origin/CJH로 푸시한다. 게임 코드·이미지는 기존 두 기능 커밋의 상태를 유지하며, 로컬 SDK 설정 두 파일은 제외한다.
- 이어가기 순서: 다른 PC에서 저장소 열기 또는 복제 → CJH 브랜치 선택 → 최신 origin/CJH 가져오기 → AGENTS.md와 이 문서의 최신 이어하기 기록 확인. GitHub Desktop에서는 Fetch origin 후 Pull origin을 사용한다. 로컬 수정이 있으면 보존한 뒤 갱신한다.
- 새 대화에 전달할 문구: "AGENTS.md와 docs/CLIENT_PROGRESS.md의 최신 인계 기록을 읽고 이어가자. 11/6·11/7과 로그인 화면 디자인은 커밋·푸시 완료야. 다음 단계는 목적과 함수 역할부터 설명하고 한 단계씩 진행해 줘."
- OpenGL SDK는 Git에 포함되지 않는다. 다른 PC에서는 기존 설치 경로를 확인한다. 최신 일정 DOCX가 없으면 이 문서의 날짜별 확정표를 따른다. 기록의 '다음 작업'은 자동 구현 승인이 아니며 사용자 요청에 맞춰 진행한다.

### 2026-11-09 [11.3] 다른 PC에서 저장소 풀

- 사용자 요청으로 CJH를 origin/CJH의 c2e9649까지 fast-forward했다. 새 기능 구현·커밋·푸시는 하지 않았다.
- 기존 로컬 변경과 새 파일을 backup-before-pull-2026-11-09 stash에 백업한 뒤 복원했다. AGENTS.md는 로컬 지침과 원격의 추가 인계 기록을 함께 보존했다. Main.cpp는 로컬 주석과 원격의 로그인 중 Q 입력 처리를 보존했다.
- 로컬 추진계획서 변경은 원격 이동 경로인 docs/추진계획서 8팀.docx에 복원됐다. 커밋 양식 이미지는 원격본을 원래 경로에, 로컬본을 docs/커밋양식.local-before-pull.png에 보존했다. 백업 stash는 유지한다.
- 브랜치 동기화·충돌 표시 제거·백업 파일 일치를 확인한다. 빌드·실행은 이번 풀 작업에서 수행하지 않는다.

### 2026-11-10 [11.3] 단계 1 — Q/ESC 키보드 종료 제거

- 사용자는 연결 종료·송수신 오류·잘못된 패킷 발생 시 이전 접속 데이터를 비우고 첫 로그인 화면으로 돌아가는 방식을 선택했다. 닉네임도 비우며, 다시 START를 누르면 새 연결과 로그인 요청을 보내 서버 응답의 사용자 번호를 저장한다. 자동 재접속·기존 세션 복구는 구현하지 않는다.
- 단계별 진행의 첫 작업으로 Main.cpp의 Keyboard에서 Q/ESC 종료 분기를 제거했다. title.cpp의 기존 Q/ESC 분기와 타이틀·로그인 화면의 ESC 종료 안내도 제거하고, 콘솔 종료 안내를 창 X 버튼으로 변경했다. q/Q는 닉네임 문자로 계속 입력할 수 있다.
- 창 X의 glutCloseFunc(OnExit) 및 기존 자원 정리는 유지한다. 종료 버튼 신설, HandleDisconnect와 OnExit 네트워크 Shutdown 연결은 아직 구현하지 않았다. 다음 단계는 HandleDisconnect의 초기화 항목과 함수를 설명하고 사용자 요청에 따라 진행한다.
- 기존 로컬 변경을 보존하고 Main.cpp의 UTF-8 및 title.cpp의 기존 인코딩을 유지했다. 커밋·푸시는 하지 않는다.
- 검증: client Debug x64 빌드·링크 성공(기존 형변환 경고 유지). 이번 변경 파일의 git diff --check 통과. 실제 빌드 대상의 Q/ESC 종료 분기·ESC 종료 안내 제거와 창 X 콜백 유지를 확인했다. cgproj.cpp는 프로젝트의 None 항목인 원본 참고 파일이라 보존했다. 실제 창에서 키 입력·X 종료는 아직 실행 검증하지 않았다.
- 후속 주석 확인: 삭제된 Q/ESC 종료 동작을 설명하던 주석은 제거·수정했고, Keyboard에 추가했던 불필요한 Q/ESC 미종료 설명도 제거했다. 창 X의 콜백 설명과 실제 자원 정리 주석은 유지한다. 주석만 추가 정리하여 빌드는 반복하지 않았다.

### 2026-11-10 [11.9] 단계 2 — HandleDisconnect 함수 구현

- 사용자 요청에 따라 GameClient.h/.cpp에 private HandleDisconnect() 선언과 구현을 추가했다. 이번 단계는 함수 자체까지만 만들며 PollNetwork/ProcessPacket의 오류 경로에는 아직 연결하지 않는다.
- network.Disconnect()로 소켓과 송수신 버퍼를 정리하고 CLIENT_LOGIN으로 전환한다. 사용자 번호·방 번호·자리 번호를 -1로 초기화하고, InputHandler{this}를 대입해 닉네임·길이·이동·공격 기록을 초기화하면서 요청 대상은 현재 GameClient로 유지한다.
- 닉네임 오류 효과 요청을 지우고 연결 종료 및 닉네임 재입력·GAME START 안내를 저장한다. serverIp와 Winsock 초기화 상태는 유지하므로 이후 START 요청에서 재접속할 수 있다. 기존 InputHandler::Reset()의 닉네임 유지 동작은 변경하지 않았다.
- 현재 화면은 drawScene이 CLIENT_LOGIN을 확인해 로그인 화면을 그리는 구조다. 네트워크 월드 데이터는 아직 구현되지 않아 이번 초기화 대상에 없으며, 추가할 때 정리 대상에도 포함해야 한다. 기존 로컬 게임·그래픽 자원은 이번 함수에서 변경하지 않는다.
- 다음 단계는 송수신 종료·오류와 잘못된 패킷 검사 경로에서 HandleDisconnect()를 호출하도록 연결하는 것이다. OnExit 네트워크 Shutdown 연결, 새 기능·커밋·푸시는 이번 범위에 포함하지 않는다.
- 검증: client Debug x64 빌드·링크 성공(기존 형변환 경고 유지), 이번 변경 파일의 git diff --check 통과. HandleDisconnect는 아직 호출하지 않으므로 실제 연결 종료 시 화면 복귀 동작은 다음 연결 단계에서 검증한다.
- 일정 표기 정정: HandleDisconnect는 11/9 계획의 작업이다. 실제 작성일은 2026-11-10이며 함수 주석의 [11.3]을 [11.9]로 수정했다. 동작 변경은 없어 빌드는 반복하지 않았다.

### 2026-11-10 [11.9] 단계 3 — 오류 경로에 HandleDisconnect 연결

- 사용자 요청으로 GameClient.cpp의 기존 오류 경로 7곳을 HandleDisconnect() 호출로 변경했다. PollNetwork의 송신 종료·오류, 패킷 추출 실패, 수신 종료·오류 및 ProcessPacket의 잘못된 헤더, 알 수 없는 종류, 종류별 길이 불일치, 잘못된 로그인 응답 값에 적용했다.
- 각 경로는 소켓·버퍼·사용자/방/자리 번호·닉네임·입력 기록을 비우고 CLIENT_LOGIN으로 돌아간다. 개별 오류 문자열은 공통 재로그인 안내로 통일해 불필요한 기존 문자열 대입도 제거했다. 정상적인 닉네임 거절·서버 정원 초과 응답과 NET_WOULD_BLOCK은 기존 처리를 유지한다.
- 기존 수신 순서(종료 감지 전 수신된 완성 패킷 처리 후 연결 정리)는 유지한다. HandleDisconnect 내부에서만 network.Disconnect를 직접 호출한다. GameClient.h, Main.cpp, NetworkClient, InputHandler, 서버, 공통 프로토콜은 이번 단계에서 수정하지 않았다.
- 검증: client Debug x64 빌드·링크 성공(기존 형변환 경고 유지), 변경 파일의 git diff --check 통과. 무시되는 .vs/Disconnect1109 임시 실행 파일은 실제 GameClient/InputHandler/NetworkClient 소스를 빌드해 loopback의 임시 포트로 검증했다. 테스트용 private 접근은 임시 소스에만 있으며 게임 소스에 테스트 경로를 추가하지 않았다.
- PASS: 로비에서 서버 정상 종료 후 상태·ID·방 정보·닉네임·입력·버퍼·오류 효과 요청 초기화 및 서버 IP/Winsock/입력 대상 보존. 새 TCP 연결 후 새 닉네임의 로그인 패킷 전송과 새로운 서버 응답 ID 저장도 확인했다. 임시 포트 재연결은 테스트가 NetworkClient::ConnectToServer를 호출했으며, 기본 9000 포트의 실제 START 자동 재접속을 실행 검증한 것은 아니다.
- PASS: 송신 오류, 수신 오류, 잘못된 패킷 길이·알 수 없는 종류·종류별 길이 불일치·잘못된 로그인 응답의 초기화. 마지막 로그인 성공 패킷과 서버 종료가 이어져도 최종 상태는 CLIENT_LOGIN이었다. NET_WOULD_BLOCK에서 연결 유지 및 닉네임 거절·서버 정원 초과에서 연결/닉네임 유지도 확인했다.
- 실제 GLUT 화면·팀 서버 연동은 이번 검증에 포함하지 않았다. OnExit의 네트워크 Shutdown 연결은 다음 단계로 남긴다. 커밋·푸시는 하지 않았다.

### 2026-11-10 [11.9] 단계 4 — 창 종료 시 네트워크 Shutdown 연결

- 사용자 요청으로 GameClient.h에 public Shutdown()을 선언하고 GameClient.cpp에서 기존 network.Shutdown()을 호출하도록 구현했다. 로그인 화면으로 돌아가는 HandleDisconnect와 프로그램 종료용 Shutdown은 각각의 역할을 유지한다.
- Main.cpp의 OnExit 시작에서 gameClient.Shutdown()을 호출한다. 기존 glutCloseFunc(OnExit)에 따라 창 X로 닫을 때 네트워크를 먼저 정리하고 기존 입력·그래픽 자원 정리를 이어서 수행한다. 네트워크 종료 연결 전의 오래된 TODO 주석만 제거하고 다른 기존 주석·정리 동작은 보존했다.
- 기존 NetworkClient::Shutdown은 Disconnect로 소켓·송수신 버퍼를 정리한 뒤 initialized인 경우에만 WSACleanup을 호출하고 initialized를 false로 바꾼다. 연결 전·연결 실패 후·이미 끊긴 상태에도 호출할 수 있으며 이번 단계에서 NetworkClient나 서버·프로토콜은 수정하지 않았다. 새 종료 버튼과 Q/ESC 종료는 추가하지 않았다.
- 검증: client Debug x64 빌드·링크 성공(기존 형변환 경고 유지), 변경 파일의 git diff --check 통과. .vs/Disconnect1109의 임시 테스트에서 GameClient::Shutdown 공개 경로를 사용해 초기화 전·초기화 후 미접속·실제 TCP 접속 중·반복 호출을 검증했다. 상대 소켓의 EOF, 송수신 버퍼 정리, Winsock 참조 균형을 확인했고 기존 연결 종료·재로그인·패킷 오류·정상 거절 응답 테스트도 통과했다.
- 실제 게임 창의 X 클릭 및 팀 서버 연동은 이번에 실행 검증하지 않았다. 11/9의 HandleDisconnect 구현·오류 경로 연결·OnExit 네트워크 종료 연결은 구현 완료이며, 다음 일정 기능은 사용자 요청 후 진행한다. 커밋·푸시는 하지 않았다.

### 2026-11-10 커밋 분리·푸시 요청

- 사용자가 합의한 세 커밋으로 나누어 CJH에서 origin/CJH로 푸시하도록 요청했다. 1번은 키보드 종료 제거, 2번은 HandleDisconnect와 오류 경로 연결, 3번은 Shutdown과 OnExit 연결이다. 각 변경과 해당 진행 기록을 함께 담는다. 위의 미커밋·미푸시 설명은 이 요청 전 작업 이력이다.
- 기존 NetworkClient/Snow 수정, 관련 없는 Main.cpp 주석 수정, AGENTS.md, 사용자 DOCX와 로컬 이미지 백업은 작업 폴더에 보존한다. 오류 경로의 사용자 수정 주석은 기능과 함께 보존한다.
- 분리한 세 커밋 각각의 소스로 별도 폴더에서 Debug x64 전체 재빌드에 성공했다(기존 형변환 경고 유지). 최종 커밋 소스로도 TCP 종료·오류·새 로그인·Shutdown 테스트가 통과했다. 검증용 SDK 경로만 임시 지정했으며 실제 게임 창의 X 클릭·팀 서버 연동은 검증 완료로 표시하지 않는다.

### 2026-11-10 최종 이어하기 기록 — 11/9 완료 및 대화 종료 준비

- 11/9 예정 구현 완료 및 CJH → origin/CJH 푸시 완료: 8e46a4c(키보드 종료 제거), 1a6d163(연결 오류 초기화·로그인 복귀), 8253de2(창 종료 네트워크 정리). 세 커밋은 각각 생성 후 순서대로 푸시했으며 HEAD/origin/CJH가 일치하고 스테이징은 비어 있음을 확인했다.
- 완료 기능·처리 기준·검증 범위·남은 로컬 수정·사용자 설명/디스코드 문체 선호는 AGENTS.md의 '2026-11-10 최신 인계'에 함께 기록했다. 위의 미구현·미커밋·미푸시 표기는 과거 작업 이력이다.
- 푸시 후 사용자가 GameClient.h/.cpp 주석을 추가 수정한 상태도 보존했다. 그 외 기존 로컬 코드·문서·이미지 백업과 stash는 그대로 남아 있다. 세 기능 커밋에는 관련 변경만 담았다.
- 실제 창 X 종료와 팀 서버 연동은 미검증이며 다음 일정 11/10 방 목록 작업은 미착수다. 사용자 요청을 받고 목적·역할·파일부터 설명한 뒤 한 단계씩 진행한다.
- 다음 대화 시작 문구: "AGENTS.md와 docs/CLIENT_PROGRESS.md 최신 기록 읽고 이어가자. 11/9는 3개 커밋으로 푸시 완료했고, 다음은 11/10 작업의 목적과 역할부터 설명해 줘."
- 이번 인계 저장은 AGENTS.md와 이 문서의 로컬 수정뿐이며 추가 커밋·푸시는 하지 않았다. 새 대화에서 전체 대화가 자동 복원된다고 가정하지 말고 이 기록과 실제 저장소 상태를 기준으로 이어간다.

### 2026-11-11 CJH 최신 변경 가져오기

- 사용자 요청으로 CJH를 origin/CJH의 b19d9dd까지 fast-forward했다. 8253de2 이후 23커밋을 받았다.
- 기존 로컬 변경과 미추적 파일은 업데이트 직후 SHA-256으로 비교해 원본과 일치함을 확인했다. 원격에서 삭제한 이 문서는 로컬 파일로 보존했고, 추진계획서의 로컬본은 작업 폴더에 복원했다. 원격 추진계획서는 b19d9dd에 있으며 두 로컬 문서의 백업은 backup-before-pull-2026-11-11-docs stash에 있다.

### 2026-11-11 [11.10] 단계 1 — 방 목록 저장 공간과 초기화

- 사용자 요청으로 첫 단계인 저장 공간과 초기화를 구현했다. GameClient.h에 공통 RoomSummary를 재사용하는 rooms[MAX_ROOMS] 배열과 실제 목록 개수 roomCount를 추가했다. 생성 시 빈 값으로 초기화하며 Init 성공 시와 HandleDisconnect에서 배열과 개수를 다시 비운다. 새 코드에는 [11.10] 표시를 붙였다.
- 검증은 기존 프로젝트의 Debug x64 빌드·링크 성공이다. 설치된 vcpkg 의존성을 사용하고 VcpkgManifestInstall=false로 실행했다. 기존 형변환 경고가 출력됐다. git diff --check는 기존 사용자 주석 '// [11.9] '의 끝 공백을 지적했으며 해당 주석을 보존했다.
- 다음 단계는 ProcessPacket의 S2C_ROOM_LIST 내용 검사·저장과 조회 함수 추가다. 현재는 방 목록 수신 반영과 로비 표시 전 단계다. 실행·서버 연동 검증은 수행하지 않았으며 커밋·푸시는 하지 않았다.

### 2026-11-11 [11.10] 단계 2 — 방 목록 수신 처리와 조회

- 사용자 요청으로 GameClient::ProcessPacket에 S2C_ROOM_LIST의 내용 검사·저장을 추가했다. 기존 종류·고정 길이 검사를 통과한 패킷을 CLIENT_LOBBY에서 처리한다. 방 개수는 0~MAX_ROOMS, 방 번호는 서버 GameServer::Init의 생성 범위인 0~MAX_ROOMS-1, 인원은 0~MAX_ROOM_PLAYERS, 상태는 ROOM_WAITING/ROOM_PLAYING/ROOM_RESULT인지 확인한다. 방 번호 중복도 검사하며 잘못된 값은 기존 HandleDisconnect로 처리한다.
- 정상 목록은 서버가 보낸 순서대로 임시 배열에 모은 뒤 rooms와 roomCount에 반영한다. 이전보다 방 수가 줄거나 0개가 오면 사용하지 않는 칸도 초기화된다. GameClient.h에 GetRoomCount()와 const RoomSummary* GetRoom(int index) const를 추가했다. GetRoom은 방 번호가 아닌 목록 순서로 조회하며 범위를 벗어나면 nullptr을 반환한다.
- 검증은 Debug x64 빌드·링크 성공 및 .vs/RoomList1110의 임시 로컬 TCP 테스트 통과다. 실제 Tick → PollNetwork → ReceiveData/ExtractPacket → ProcessPacket 경로로 정상 목록·세 상태·순서 유지·갱신·분할 수신·목록 감소·빈 목록·조회 범위·잘못된 방 개수/번호/중복/인원/상태·연결 종료와 재초기화를 확인했다. 테스트 접속에 기존 임시 fixture의 private 접근을 사용했으며 게임 코드에 테스트 경로를 추가하지 않았다.
- 팀 서버 연동과 화면 표시는 다음 검증 범위다. 다음 단계는 준비된 로비 이미지와 저장된 방 목록을 drawScene 및 기존 화면 코드에서 표시하는 것이다. 커밋·푸시는 하지 않았다.

### 2026-11-11 [11.10] 단계 3 첫 작업 — 로비 이미지 준비·정리

- 사용자 요청으로 로비 화면 작업 중 이미지 로딩과 자원 정리부터 구현했다. title.h에 로비 배경·방 버튼·제목의 텍스처 번호 3개와 loadLobbyTexture(const char*, GLuint&)를 추가했다. 기존 TitleScreen::initialize에서 Images/Lobby의 PNG 3종을 불러온다. RGBA로 읽어 투명도를 유지하고 기존 이미지와 같은 세로 뒤집기·선형 필터·가장자리 제한 설정을 사용한다. 파일을 읽지 못하면 경로를 로그로 남기고 해당 번호는 0으로 유지한다.
- TitleScreen::cleanup에서 로비 텍스처 3개를 해제하고 번호를 0으로 비운다. 기존 OnExit → titleScreen.cleanup 호출에 연결되며 Main.cpp의 추가 수정은 없다. title.cpp의 CP949 인코딩과 기존 내용을 바이트 단위로 보존했다. 추가 부분은 [11.10]으로 표시했다.
- 실행 폴더에서도 경로가 일치하도록 client.vcxproj의 CopyGameAssets에 로비 PNG의 Images/Lobby 하위 폴더 복사를 추가했다. 검증은 Debug x64 빌드·링크 성공, 복사된 이미지 3종의 SHA-256 원본 일치, 이번 변경 파일의 git diff --check 통과다. 기존 형변환 경고가 출력됐다.
- 다음 작업은 준비된 로비 이미지를 화면에 그리는 연결과 저장된 방 인원·상태 표시다. 현재 검증 범위는 빌드와 이미지 복사이며 실제 OpenGL 텍스처 로딩·표시 및 종료 동작의 실행 검증은 남아 있다. 커밋·푸시는 하지 않았다.

### 2026-11-12 [11.10] 단계 3 후속 — 로비 이미지 화면 연결

- 사용자 요청으로 title.h/.cpp에 TitleScreen::renderLobby를 추가했다. 기존 셰이더와 drawUIQuad를 사용해 로비 배경을 화면 전체에, 제목을 상단 중앙에, 방 버튼 이미지를 세 줄로 배치했다. 기존 로그인 UI와 같은 1200×675 기준 화면 좌표와 알파 블렌딩을 사용하며 그린 뒤 깊이·컬링·블렌딩 설정을 복원한다. 로비 이미지가 없으면 기존 panel/centeredText 방식으로 해당 영역을 표시한다.
- Main.cpp의 drawScene에서 CLIENT_LOBBY이면 renderLobby를 호출하도록 연결했다. 새 코드에는 [11.10] 표시를 붙였으며 title.cpp의 기존 CP949 내용은 바이트 단위로 보존했다. 배치 좌표는 기존 이미지의 원본 비율과 투명 여백을 기준으로 정했다.
- 검증은 Debug x64 빌드·링크 성공과 .vs/LobbyRender1110의 숨긴 OpenGL 창에서 실제 title.cpp 및 vertex.glsl/fragment.glsl을 사용한 렌더 확인이다. 1200×675·960×540 결과 이미지를 확인해 배경·제목·버튼 세 칸의 배치, 투명도, 방향을 확인했다. 초기화 반복·정리 반복·그리기 후 깊이/컬링/블렌딩 복원도 통과했다. 이는 로비 함수의 직접 렌더 검증이며 실제 팀 서버 로그인 후 화면 전환과 실제 창 X 종료 검증은 남아 있다.
- 현재 버튼 세 칸은 이미지 배치 단계다. 다음 작업은 GetRoomCount/GetRoom으로 받은 방 번호·인원·상태를 버튼 위에 표시하는 것이다. 방 클릭·입장 요청은 11/11 일정에서 진행한다. 커밋·푸시는 하지 않았다.

### 2026-11-12 [11.10] 로컬 로그인 응답으로 로비 미리보기 준비

- 사용자가 팀 서버 연결 없이 게임 창에서 로비를 보는 방법을 요청했다. .vs/LobbyPreview1110/PreviewLoginServer.ps1을 만들고 127.0.0.1:9000에서 임시 로그인 응답 서버를 숨겨 실행했다. 실행 직후 PID는 17032였으며 현재 상태는 같은 폴더의 status.json으로 확인한다. 실제 서비스 중인 포트가 없음을 먼저 확인했고 리스닝 준비를 확인했다.
- 사용자는 게임 실행 → 닉네임 입력 → START로 정상 클라이언트 로그인 흐름을 사용할 수 있다. 테스트 서버는 현재 protocol.h의 형식에 맞는 S2C_LoginResult(LOGIN_OK, session_id=1)를 반환하고 연결을 유지한다. 클라이언트가 연결을 닫으면 테스트 서버도 종료된다. 실제 사용자의 START 클릭과 그 이후 화면 전환은 아직 확인 전이다.
- 이 서버는 현재 로비 이미지 미리보기용으로 로그인 성공 응답만 보내며 실제 팀 서버의 로그인·방·게임 처리를 검증한 결과로 해석하지 않는다. 게임 소스 변경·커밋·푸시는 하지 않았다.

### 2026-11-12 [11.10] 로비 방 버튼 크기 조정

- 사용자가 임시 로그인 서버를 통해 실제 게임에서 로비로 넘어감을 확인했고, 세 방 버튼을 약 10% 줄여 달라고 요청했다. title.cpp의 renderLobby에서 버튼 이미지 크기를 750×250에서 675×225로 줄이고 x를 262.5, y를 기존 값+12.5로 조정해 각 버튼의 중심 위치를 유지했다. 이미지 누락 시 panel도 같은 비율로 줄였다.
- Debug x64 빌드·링크와 기존 로비 직접 렌더 검증을 통과했다. 수정된 1200×675 이미지를 확인했으며 OpenGL 설정 복원과 반복 정리도 통과했다. 사용자 재확인을 위해 종료된 임시 로그인 서버를 다시 실행했다. 실제 팀 서버 연동 및 버튼 클릭·입장 처리는 후속 단계다. 커밋·푸시는 하지 않았다.

### 2026-11-12 [11.10] 방 버튼 추가 축소 및 로비 제목 축소

- 사용자가 버튼을 조금 더 줄이고 게임 로비 제목 이미지도 줄이도록 요청했다. 버튼은 직전 크기의 90%인 607.5×202.5로 조정했다. 최초 750×250 대비 가로·세로 각각 81%다. 제목은 600×201.6에서 540×181.44로 10% 줄였다. 각 이미지의 기존 중심 위치를 유지했다. 이미지 누락 시 표시하는 panel과 LOBBY 글자도 같은 비율로 조정했다.
- Debug x64 빌드·링크 및 기존 로비 직접 렌더 검증을 통과했다. 변경 후 1200×675 이미지를 확인했고 임시 로그인 서버도 다시 준비했다. 실제 게임 확인은 다시 빌드된 client 실행 파일로 진행한다. 커밋·푸시는 하지 않았다.

### 2026-11-12 [11.10] 방 버튼 5% 추가 축소

- 사용자 요청으로 세 방 버튼을 직전 크기에서 5% 더 줄였다. 이미지 크기는 577.125×192.375이며 최초 크기의 76.95%다. 위치는 x=311.4375, y=기존 값+28.8125로 각 버튼 중심을 유지했다. 이미지 누락 시 panel도 같은 비율로 줄였다. 제목은 직전 조정값을 사용한다.
- Debug x64 빌드·링크를 확인했고 재확인용 임시 로그인 서버를 준비했다. 이번 추가 축소의 게임 화면 시각 확인은 사용자 재실행으로 진행한다. 커밋·푸시는 하지 않았다.

### 2026-11-12 [11.10] 캡처 확인 후 방 버튼 추가 축소

- 사용자가 실제 로비 캡처를 제공하고 세 방 버튼을 조금 더 줄이도록 요청했다. 직전 크기에서 약 5% 줄여 548.27×182.76으로 조정했고 중심 위치를 유지했다. 최초 크기 대비 약 73.1%이며 이미지 누락 시 panel도 같은 비율로 조정했다.
- Debug x64 빌드·링크와 변경 파일의 diff 공백 검사를 확인하고 임시 로그인 서버를 다시 준비했다. 이번 추가 축소의 게임 화면 시각 확인은 사용자 재실행으로 진행한다. 커밋·푸시는 하지 않았다.

### 2026-11-12 [11.10] 방 목록 표시 완료

- 사용자 요청으로 저장된 방 번호·현재 인원/정원·방 상태를 로비 버튼 안에 표시했다. Main.cpp의 drawScene에서 GameClient::GetRoom(0)과 GetRoomCount()를 TitleScreen::renderLobby에 전달한다. title.h는 RoomSummary를 전방 선언하고 title.cpp에서 공통 protocol.h의 정의를 읽는다. GameClient와 네트워크의 기존 수신·조회 흐름을 사용한다.
- renderLobby는 서버가 보낸 순서와 실제 방 개수에 맞춰 버튼을 최대 MAX_ROOMS까지 그린다. 각 버튼 안에 Room 번호, 인원/2, Waiting/Playing/Result를 표시한다. 기존 글꼴은 ASCII 문자만 지원하므로 상태는 영문을 사용했다. 목록이 줄면 남은 버튼과 글자도 함께 사라지고 0개이면 No rooms available을 표시한다. 사용자가 조정한 이미지 크기와 중심 배치를 사용한다.
- 검증은 Debug x64 빌드·링크, 변경 파일의 git diff --check, 임시 .vs/LobbyRender1110의 실제 TCP 수신 → Tick/PollNetwork/ProcessPacket → GetRoom/GetRoomCount → renderLobby 경로 테스트 통과다. 세 상태와 방 번호 순서, 목록이 3개→1개→0개로 바뀔 때의 갱신, 1200×675와 960×540 결과 이미지, OpenGL 설정 복원과 반복 정리를 확인했다. 기존 형변환 경고가 출력됐다.
- 임시 .vs/LobbyPreview1110/PreviewLoginServer.ps1에 로그인 성공 후 예시 방 목록 3개를 보내는 처리를 추가하고 실행했다. 예시 값은 방 0=1/2 Waiting, 방 1=2/2 Playing, 방 2=0/2 Waiting이다. 테스트 데이터이며 팀 서버에서 받은 실제 방 현황이 아니다. 준비 직후 PID는 39112였고 현재 상태는 status.json에서 확인한다. 게임 재실행 후 닉네임 입력·START로 확인할 수 있다.
- 11/10 일정의 클라이언트 방 목록 수신·저장·화면 표시 구현과 로컬 검증은 완료했다. 실제 팀 서버의 방 목록 연동 검증은 남아 있다. 다음 일정은 11/11 QueueRequest/ProcessPacket/OnKeyDown의 방 입장이며 사용자 요청에 맞춰 목적·범위를 설명한 뒤 진행한다. 커밋·푸시는 하지 않았다.

### 2026-11-12 [11.10] 세 커밋 분리 및 CJH 푸시 완료

- 사용자 요청으로 CJH에서 세 커밋을 만들고 origin/CJH로 푸시했다. 72424b1은 방 목록 검사·저장·조회, 9c2cccc는 로비 이미지 로딩·정리·복사, e3cbe6c는 로비 화면·방 정보 표시다. 위의 미커밋·미푸시 설명은 이번 요청 이전 이력이다.
- 세 커밋의 소스를 각각 기준 상태에서 구성한 별도 복사본으로 Debug x64 전체 재빌드해 통과했다. 복사본 준비 중 누락했던 UTF-8 표식을 원본과 일치시킨 뒤 전체 재검증했다. 기존 형변환 경고가 유지됐고 최종 커밋 차이의 공백 검사도 통과했다.
- 기존 로컬 파일 전체는 커밋 전후 SHA-256을 비교해 바이트 일치를 확인했다. GameClient/Main의 기존 사용자 주석 수정과 NetworkClient/Snow 수정, AGENTS.md, 사용자 DOCX와 미추적 이미지·LLD, 기존 stash를 보존했다. 테스트·미리보기 서버는 무시되는 .vs 폴더에 있다.
- 이 진행 기록은 원격에서 삭제된 뒤 로컬에 보존한 파일이므로 이번 세 기능 커밋은 코드 6개 파일에 한정했고, 진행 기록은 로컬 인계 파일로 추가했다. Git 작성자는 이전 CJH 커밋과 같은 Cho-Jeonghyeon <69703120+Cho-Jeonghyeon@users.noreply.github.com>을 명령 단위로 지정했다.
- 현재 11/10 클라이언트 구현·로컬 검증·코드 푸시가 완료됐으며 실제 팀 서버 연동은 미검증이다. 다음 일정은 11/11 방 입장 처리다. 목적과 수정 범위를 설명하고 사용자 요청에 맞춰 한 단계씩 진행한다.

### 2026-11-12 [11.11] 단계 1 — 공통 요청 등록 QueueRequest

- 사용자 요청으로 GameClient.h에 template<class Packet> bool QueueRequest(Packet packet)를 추가했다. 호출자는 type과 요청 내용을 채우며 함수가 복사본의 size를 sizeof(Packet)으로 설정해 기존 network.QueuePacket에 등록한다. 실제 전송은 기존 Tick → PollNetwork → FlushSendBuffer 흐름에서 수행한다. 템플릿 정의는 헤더에 두고 추가 부분을 [11.11]로 표시했다.
- 로그인 상태에서는 요청을 거절하고 C2S_JOIN_ROOM부터 C2S_USE_ITEM까지의 로그인 외 요청 8종을 받는다. type 범위를 벗어나면 오류 메시지를 남긴다. 바이트 복사 가능한 자료형과 2~255바이트 크기를 static_assert로 확인한다. QueuePacket 실패 시 기존 HandleDisconnect로 연결·로그인·방 데이터를 정리한다. 등록 성공 시 이전 오류를 지우며 반환값 true는 송신 버퍼 등록 성공을 의미한다.
- 검증은 Debug x64 빌드·링크와 .vs/QueueRequest1111의 임시 TCP 테스트 통과다. 실제 요청 8종을 모두 템플릿으로 컴파일해 크기 자동 설정·호출자 원본 유지·버퍼 등록·Tick 전 미송신·Tick 후 수신 바이트 일치, 로그인 전 거절, 로그인/서버 응답 종류 거절, 연결 종료 시 공통 초기화를 확인했다. 테스트용 private 접근은 임시 프로젝트의 강제 포함 헤더에만 있다. 기존 형변환 경고가 유지됐고 GameClient.h의 diff 공백 검사도 통과했다.
- 다음 단계는 로비의 방 선택 입력으로 C2S_JoinRoom을 만들어 QueueRequest에 전달하는 연결이다. 이후 S2C_JoinRoomResult의 성공·실패 처리를 ProcessPacket에 추가한다. 이번 단계의 게임 코드 수정 파일은 GameClient.h이며 기존 사용자 주석을 보존했다. 커밋·푸시는 하지 않았다.

### 2026-11-12 [11.11] QueueRequest를 일반 함수로 단순화

- 사용자가 template/static_assert/type_traits 문법이 학습 단계에 비해 복잡하다고 지적하고 쉬운 방식을 요청했다. QueueRequest를 bool QueueRequest(const void* packet, int packetSize) 일반 함수로 바꾸고 GameClient.h에 선언, GameClient.cpp에 구현했다. 앞선 템플릿 구현 기록보다 이 방식이 최신이다. 사용자는 기존 프로젝트에서 사용한 문법으로 목적과 코드를 이해하며 진행하기를 원한다.
- 호출자는 요청 구조체의 주소와 sizeof 크기를 전달한다. 함수는 로그인 상태·주소·2~255바이트 크기를 확인한 뒤 바이트 배열에 복사하고 요청 종류를 검사한다. 복사본의 첫 바이트에 크기를 채우고 기존 QueuePacket에 등록한다. 원본 보존, 등록 후 Tick/PollNetwork를 통한 실제 전송, 실패 시 HandleDisconnect의 기존 역할은 유지한다. 로그인 외 요청 8종을 같은 함수로 받는 설계를 유지했다.
- Debug x64 빌드·링크와 기존 임시 TCP 테스트를 통과했다. 요청 8종의 크기 설정·원본 유지·등록과 송신 시점·수신 바이트 일치, null/음수/짧은 길이/255 초과 거절, 요청 종류 검사와 연결 종료 초기화를 확인했다. 이번 단계의 게임 코드 변경 파일은 GameClient.h/.cpp이며 커밋·푸시는 하지 않았다.

### 2026-11-12 [11.11] 단계 2 첫 작업 — OnKeyDown에서 방 선택 요청 생성

- 사용자 요청에 맞춰 이번에는 InputHandler::OnKeyDown의 요청 생성 부분까지 구현했다. CLIENT_LOBBY에서 숫자 1·2·3을 목록의 첫째·둘째·셋째 항목으로 바꾸고 GetRoom으로 정보를 읽는다. C2S_JoinRoom의 type과 room_id를 채워 일반 함수 QueueRequest(&request, sizeof(request))에 등록한다. 실제 방 번호는 서버 목록의 room_id를 사용한다. InputHandler.h에 역할 설명을 추가했고 기존 로그인 입력 주석을 보존했다.
- 목록에 없는 항목이나 다른 키는 요청을 만들지 않는다. 클라이언트는 요청 등록 후 로비 상태를 유지한다. 방의 정원·상태에 따른 입장 판정과 성공 후 상태 전환은 서버 응답 처리 단계에서 진행한다. 입력 대상이 없을 때의 검사와 로그인 화면의 닉네임·Backspace·Enter 흐름을 유지했다.
- 검증은 Debug x64 빌드·링크와 .vs/RoomInput1111 임시 TCP 테스트 통과다. GameClient::OnKeyDown → InputHandler::OnKeyDown → QueueRequest → Tick 전송 경로로 방 목록 순서 2·0·1에 대해 키 1·2·3이 해당 실제 방 번호를 보내는지 확인했다. 빈 목록·줄어든 목록·다른 키·다른 상태·닉네임 유지·기존 로그인 요청·연결 종료 중 선택의 공통 초기화도 확인했다. 임시 테스트의 private 접근 설정은 .vs에만 있다.
- 다음 작업은 Main.cpp의 Keyboard에서 로비 입력을 전달하고 title.cpp/Main.cpp에서 방 버튼 클릭을 같은 요청 생성 흐름으로 연결하는 것이다. 현재 게임 창의 콜백 연결과 S2C_JoinRoomResult 성공·실패 반영은 후속 단계다. 커밋·푸시는 하지 않았다.

### 2026-11-13 [11.11] 방향키 선택·Enter 확정으로 변경 및 키 입력 연결

- 사용자가 숫자 1·2·3 대신 로비 기본 첫 방 선택 → 위·아래 이동 → Enter 입장 요청을 원한다고 확정했다. 숫자 선택 기록보다 이 조작 방식이 최신이다. 이번 단계는 입력·선택 상태 연결까지 진행하며 선택 버튼 확대·흰색 테두리는 다음 단계로 남긴다.
- GameClient.h/.cpp에 selectedRoomIndex, GetSelectedRoomIndex, MoveRoomSelection을 추가했다. -1은 위, 1은 아래로 이동하며 첫 방의 위·마지막 방의 아래에서는 선택을 유지한다. 초기화·로그인 성공·연결 종료 시 첫 번째 위치인 0으로 설정한다. 방 목록 갱신 시 기존 선택 방 번호를 찾아 유지하고 사라지면 첫 번째 위치로 돌아간다. 빈 목록에서는 입장 요청을 만들지 않는다.
- InputHandler::OnKeyDown의 로비 분기는 Enter일 때만 현재 선택 정보를 읽어 C2S_JoinRoom을 QueueRequest에 전달하도록 변경했다. Main.cpp의 Keyboard는 로그인·로비 키 입력을 전달하고, 기존 SpecialKeyboard는 로비에서 GLUT_KEY_UP/GLUT_KEY_DOWN을 MoveRoomSelection(-1/1)에 전달한다. 별도의 방향키 입력 클래스를 만들지 않았다. 기존 사용자 주석은 보존하고 추가 코드는 [11.11]로 표시했다.
- Debug x64 빌드·링크와 .vs/RoomInput1111의 기존 임시 TCP 검증을 통과했다. 기본 선택, 끝 경계, 숫자·다른 키 무시, Enter 전 미등록과 Enter 후 실제 선택 방 번호 전송, 로비 상태 유지, 목록 순서 변경·방 제거·빈 목록, 다른 상태 입력 무시, 기존 닉네임·로그인 입력과 연결 종료 초기화를 확인했다. 기존 형변환 경고가 출력됐다. diff 공백 검사는 기존 사용자 주석 '// [11.9] '의 끝 공백만 지적했다.
- 다음 단계는 선택된 방 위치를 renderLobby에 전달해 해당 버튼을 살짝 키우고 흰색 테두리로 강조하는 것이다. 이후 S2C_JoinRoomResult 성공·실패 처리로 방 번호·자리 번호 저장과 대기실 상태 전환을 구현한다. 실제 게임 창에서 방향키 조작을 눈으로 확인하는 검증은 강조 표시 연결 후 진행한다. 커밋·푸시는 하지 않았다.

### 2026-11-13 [11.11] 선택된 방 버튼 확대·흰색 테두리 연결

- 사용자 요청으로 Main.cpp에서 GetSelectedRoomIndex()를 renderLobby의 네 번째 인자로 전달하도록 연결했다. title.h/.cpp에서 해당 인자를 받아 선택된 방만 가로·세로 5% 확대하며 각 버튼의 중심 위치를 유지한다. 버튼 가장자리에 2픽셀 두께, 흰색 알파 0.9의 사각 테두리를 기존 panel/drawUIQuad로 그린다. 제목·서버 방 정보·일반 버튼은 기존 배치를 사용한다. 새 셰이더나 이미지를 추가하지 않았다.
- Debug x64 빌드·링크와 기존 .vs/LobbyRender1110의 실제 클라이언트 TCP 수신 후 렌더 검증을 통과했다. 첫째·둘째·셋째 선택 이미지와 1200×675·960×540 크기, 목록 감소·빈 목록, OpenGL 설정 복원·반복 정리를 확인했다. 해당 변경 파일의 diff 공백 검사도 통과했다. title.cpp는 기존 CP949 인코딩과 원본 바이트를 보존해 변경했다.
- computer-use 스킬의 sky API로 실제 게임 창을 확인했다. 직접 launch_app 실행은 대상 창이 잡히지 않아 기존 탐색기의 client/x64/Debug 폴더에서 실행했다. 게임의 상대 경로 셰이더·이미지는 이 실행 폴더를 기준으로 읽는다. 실제 창에서 방 정보와 선택 테두리를 확인했다. 자동 방향키 입력은 사용자 입력 감지로 실행되지 않았으므로 사용자 조작 중 표시 캡처와 임시 서버의 join_room_1 수신 로그를 근거로 확인 범위를 기록한다.
- 임시 로그인 서버에 C2S_JoinRoom 수신 시 방 번호를 status.json에 기록하는 테스트용 처리만 추가했다. 게임 창에서의 Enter 요청이 서버에 도착했음을 확인했으며 서버는 입장 성공·실패 응답을 아직 보내지 않는다. 테스트 서버는 .vs/LobbyPreview1110에 있고 확인 시 PID는 44412였다.
- 다음 단계는 ProcessPacket에서 S2C_JoinRoomResult를 처리해 성공 시 방·자리 번호 저장과 CLIENT_ROOM 전환, 정상 거절 시 로비 유지와 사유 안내를 연결하는 것이다. 대기실 상세 정보·디자인은 이후 일정에 따른다. 실제 팀 서버 연동 검증은 남아 있으며 이번 단계 커밋·푸시는 하지 않았다.

### 2026-11-13 [11.11] 방 입장 응답 처리와 결과 안내

- 사용자 요청으로 GameClient::ProcessPacket에 S2C_JoinRoomResult 처리를 추가했다. 기존 패킷 종류·길이 검사를 통과하고 CLIENT_LOBBY일 때 결과를 읽는다. JOIN_OK이면 방 번호와 자리 번호의 범위를 확인한 뒤 myRoomId/mySlot에 저장하고 CLIENT_ROOM으로 전환한다. 이동·공격 입력 기록과 이전 오류를 비운다. 정상 거절(JOIN_ROOM_FULL/JOIN_ROOM_PLAYING/JOIN_INVALID_ROOM)은 연결과 로비 상태를 유지하고 사유만 저장한다. 성공 응답의 잘못된 번호 또는 정의되지 않은 결과 값은 기존 HandleDisconnect로 처리한다.
- GameClient.h에 GetMyRoomId/GetMySlot을 추가했다. Main.cpp의 drawScene에서 로비 오류 메시지를 renderLobby에 전달하고, CLIENT_ROOM이면 renderRoomEntry로 입장한 방·자리 번호를 표시한다. Keyboard에서는 CLIENT_ROOM 입력이 기존 로컬 게임 시작 코드로 흘러가지 않도록 막았다. title.h/.cpp에 오류 안내와 간단한 ROOM ENTERED 화면을 추가했다. 대기실 참가자·준비 상태 등 상세 기능은 이번 단계에 구현하지 않았다. title.cpp의 기존 CP949 인코딩과 사용자 주석을 보존했다.
- 검증: Debug x64 빌드·링크 통과. .vs/JoinResult1111의 로컬 TCP 테스트로 세 종류 거절 후 재시도, 나뉘어 도착한 성공 응답, 모든 유효 방·자리 번호, 입력 초기화, 잘못된 성공 값·결과 값의 연결 초기화, 로비 이전·입장 이후 응답 무시, 성공 직후 연결 종료를 확인했다. .vs/LobbyRender1110의 실제 TCP 수신 및 OpenGL 렌더링 테스트를 통과했고 join-full/playing/invalid/success.bmp를 직접 확인했다. 렌더 테스트의 예시 목록 복원 시 생긴 중복 방 번호는 테스트 자료에서 수정했다. diff 공백 검사는 기존 사용자 주석 '// [11.9] '의 끝 공백만 지적했다.
- 테스트용 .vs/LobbyPreview1110/PreviewLoginServer.ps1에 입장 응답을 추가해 실행했다. Room 0은 성공·Slot 1, Room 1은 게임 진행 중, Room 2는 정원 초과로 응답한다. 방 목록도 Room 2=2/2 Waiting으로 맞췄다. 시작 확인 PID는 42820, 포트는 127.0.0.1:9000이다. 한 연결용 임시 서버이며 팀 서버 코드와 common/protocol.h는 변경하지 않았다. 실제 팀 서버 연동 및 이번 변경 이후 게임 창 키보드 조작은 별도로 검증하지 않았다. 커밋·푸시는 하지 않았다.
### 2026-11-13 [11.11] 세 커밋 분리 및 CJH 푸시 완료

- 사용자 요청으로 합의한 세 커밋을 CJH에서 생성하고 origin/CJH로 푸시했다. 9cebd1f는 QueueRequest 공통 요청 등록, fd8ddc8은 방향키 방 선택·Enter 입장 요청·선택 버튼 강조, 07a90a8은 입장 결과 반영·거절 이유·입장 확인 화면이다. 앞의 미커밋·미푸시 기록은 이번 요청 이전 상태다.
- 세 커밋 각각에 들어갈 소스만 별도 복사본(.vs/Commit1111/verify)으로 구성해 Debug x64 전체 재빌드와 단계별 TCP 테스트를 통과했다. 1단계는 요청 8종과 입력 검사·연결 종료, 2단계는 방향키 선택 범위·Enter 요청·목록 갱신, 3단계는 성공·거절·재시도·분할 수신·잘못된 응답 처리를 검증했다. 기존 형변환 경고는 유지된다. 실제 팀 서버 연동 검증은 남아 있다.
- 커밋에는 GameClient.h/.cpp, InputHandler.h/.cpp, Main.cpp, title.h/.cpp의 이번 기능만 포함했다. 기존 사용자 주석과 NetworkClient/Snow 수정, AGENTS.md, 사용자 DOCX·LLD·이미지, stash 및 로컬 진행 기록은 보존했다. 커밋 준비 전후 모든 기존 로컬 파일의 SHA-256이 일치함을 확인했다. title.cpp의 CP949와 나머지 소스의 UTF-8 BOM을 유지했다. 작성자 정보는 기존 CJH 커밋과 같은 값을 명령 단위로 지정했고 설정은 변경하지 않았다.
- 푸시 후 HEAD와 origin/CJH는 07a90a8로 동기화됐다. 진행 기록은 로컬 인계 파일로 유지하며 이번 기능 커밋에 추가하지 않았다. 다음 일정은 11/12 퇴장·대기실이며 사용자 요청에 맞춰 목적과 범위를 설명한 뒤 한 단계씩 진행한다.
### 2026-11-13 [11.12] 단계 1 — 대기실 참가자 저장 공간과 초기화

- 사용자는 11/12 브리핑 후 진행을 요청했고, 이번에는 첫 단계인 저장 공간 준비만 구현했다. GameClient.h에 RoomPlayerInfo roomPlayers[MAX_ROOM_PLAYERS]{} 배열을 추가했다. 기존 common/protocol.h의 RoomPlayerInfo(connected/name/character)를 그대로 사용하며 배열의 0·1 인덱스가 방의 자리 번호를 뜻한다.
- GameClient.cpp의 Init, HandleDisconnect, ProcessPacket의 JOIN_OK 경로에서 배열을 비우고 각 character를 CHAR_NONE으로 설정한다. {}만 적용하면 character=0인 CHAR_STEVE가 되므로 '미선택' 값은 별도로 넣는다. 성공 응답 번호 검사 후 새 방으로 들어갈 때 이전 참가자 정보를 제거한다. 수신 전이므로 내 자리도 아직 빈 값이며, 다음 단계의 S2C_RoomInfo로 채울 예정이다.
- Debug x64 전체 빌드·링크를 별도 출력 폴더 .vs/RoomStorage1112/bin에서 통과했다. 기존 형변환 경고가 유지됐다. 작업 직전 백업과 비교해 이번 배열 및 초기화 추가만 변경됐고, 추가 부분의 공백 검사와 기존 UTF-8 BOM 유지도 확인했다. 이번 단계는 저장 공간 준비이므로 새 테스트 코드는 작성하지 않았다.
- S2C_RoomInfo 수신·검사·저장, 조회 함수, 대기실 화면 확장, 퇴장 요청·로비 복귀는 아직 구현하지 않았다. 다음 단계는 ProcessPacket의 S2C_RoomInfo 처리다. 기존 사용자 수정(입장 결과 주석 및 정원 초과 한국어 문구 포함), 서버, common/protocol.h는 보존했다. 커밋·푸시는 하지 않았다.
### 2026-11-13 [11.12] 단계 2 — 대기실 정보 수신·검사·저장

- 사용자 요청으로 GameClient.cpp의 ProcessPacket에 S2C_ROOM_INFO 처리만 추가했다. 기존 종류·크기 검사를 유지하고 CLIENT_ROOM일 때 S2C_RoomInfo로 복사한다. 방 번호가 유효하고 myRoomId와 같은지 확인한다. 참가자 접속 여부 바이트가 0/1인지 먼저 검사해 잘못된 bool 값을 그대로 사용하지 않는다.
- 접속한 자리의 캐릭터는 CHAR_STEVE/CHAR_ALEX/CHAR_NONE만 허용하며 닉네임은 기존 로그인 규칙과 같은 영문·숫자 1~15자 및 범위 안의 널 종료를 검사한다. 빈 자리는 이름과 캐릭터 필드를 초기화하고 CHAR_NONE으로 만든다. 두 자리 모두 검사 후 roomInfo.players를 roomPlayers에 한 번에 복사한다. 잘못된 값은 기존 HandleDisconnect로 처리하고 참가자 정보도 비운다.
- Debug x64 빌드·링크와 .vs/RoomInfo1112 로컬 TCP 테스트를 통과했다. 두 참가자 저장, 정보 갱신, 15자 닉네임과 미선택 캐릭터, 빈 자리 정리와 상대 재입장, 분할 수신, 로그인·입장·대기실 패킷이 함께 도착하는 경우, 방 번호·닉네임·캐릭터·접속 여부 오류, 대기실 외 상태 무시, 연결 종료 후 전체 초기화를 확인했다. 작업 직전 백업과 비교한 diff 공백 검사도 통과했다.
- 게임 코드 변경은 GameClient.cpp의 이번 분기 추가뿐이다. 서버·common/protocol.h·기존 사용자 주석과 한국어 오류 문구는 보존했다. 공개 조회 함수·대기실 화면·퇴장은 이번 단계에 추가하지 않았다. 실제 팀 서버 연동은 미검증이며 미리보기 서버도 이번에 변경하지 않았다. 다음 단계는 저장된 정보를 조회하여 대기실에 표시하는 것이다. 커밋·푸시는 하지 않았다.
### 2026-11-13 [11.12] 단계 3 — 저장된 참가자 정보의 대기실 화면 표시

- 사용자가 이전 수신 코드의 위치·목적 설명과 다음 단계 진행을 요청했다. ProcessPacket의 S2C_ROOM_INFO 분기는 GameClient.cpp 356~403줄이며 임시 복사·현재 방 확인·참가자 값 검사·최종 저장의 역할을 설명했다. 검사가 과도하게 느껴진다는 논의는 있었으나 이번 요청에서 삭제·단순화를 확정하지 않았으므로 기존 수신 검사는 유지했다.
- GameClient.h에 범위를 검사하는 읽기 전용 GetRoomPlayer(slot)를 추가했다. Main.cpp의 drawScene에서 GetRoomPlayer(0)을 기존 renderRoomEntry에 전달한다. title.h에 RoomPlayerInfo 전방 선언과 추가 인자를 연결하고, title.cpp의 renderRoomEntry를 두 자리 대기실 화면으로 확장했다. 각 자리 번호·서버 닉네임·Steve/Alex/Not selected를 표시하고 내 자리는 (You)와 배경색으로 구분한다. 접속하지 않은 자리는 Waiting for player...로 표시한다. 아직 정보가 도착하지 않은 동안에는 두 자리가 빈 상태로 보인다. 현재 ASCII 글꼴에 맞춰 안내는 영문을 사용했다.
- Debug x64 빌드·링크 통과(기존 형변환 경고 유지). .vs/RoomUi1112에서 실제 TCP 수신 → ProcessPacket → GetRoomPlayer → renderRoomEntry 경로로 검증했다. 정보 수신 전·두 참가자·15자 이름·Steve/Alex·상대 퇴장 및 캐릭터 미선택, 1200×675와 960×540 결과 BMP를 직접 확인했다. OpenGL 설정 복원·조회 범위·기존 로비 렌더링도 확인했다. 변경 부분의 공백 검사가 통과했고 title.cpp의 기존 CP949 바이트는 의도한 변경을 역으로 치환해 보존 여부를 확인했다.
- 미리보기 서버 스크립트(.vs/LobbyPreview1110)도 입장 성공 후 S2C_RoomInfo를 보내도록 준비했다. 0번 자리는 PreviewBot/Steve, 1번 자리는 로그인한 닉네임/CHAR_NONE을 담는다. 스크립트 구문 검사는 통과했으며 이 수정 뒤 실제 미리보기 서버를 실행하거나 게임 창으로 접속하지는 않았다. 마지막 status.json은 closed였다. 팀 서버 및 common/protocol.h는 보존했다.
- 게임 코드 변경 파일은 GameClient.h, Main.cpp, title.h/.cpp이며 서버 수신·검사 로직은 이번 단계에서 변경하지 않았다. 퇴장 입력·로비 복귀와 캐릭터 선택·게임 시작은 아직 연결하지 않았다. 다음 작업은 11/12 단계 4인 OnKeyDown의 퇴장 요청이다. 커밋·푸시는 하지 않았다.
### 2026-11-13 [11.12] 대기실 수신의 이름 재검사 제거

- 사용자 요청으로 ProcessPacket의 S2C_ROOM_INFO 분기에서 닉네임 영문·숫자 검사, 빈 이름 검사, 널 종료를 찾는 반복문과 해당 연결 종료 처리를 제거했다. 로그인 요청 시의 이름 검사는 유지한다. 서버가 관리하는 이름 규칙을 수신 쪽에서 다시 판정하지 않는다.
- 대신 player.name[MAX_NAME_LEN - 1] = '\0' 한 줄로 고정 배열의 마지막 칸에 문자열 끝 표시를 넣는다. 정상 1~15자 이름에는 영향이 없으며, 널 문자가 없는 16바이트가 와도 화면이 배열 밖을 문자열로 읽지 않도록 최대 15자에서 끝낸다. 방 번호·접속 여부·캐릭터 검사 및 빈 자리 정리, 최종 저장은 유지했다.
- Debug x64 빌드·링크와 기존 .vs/RoomInfo1112 TCP 테스트를 변경된 동작에 맞춰 실행해 통과했다. 빈 이름·기호 포함 이름을 받아도 연결을 유지하고, 종료 문자가 없는 이름은 마지막 칸에서 끝나는 것을 확인했다. 기존 수신·갱신·빈 자리·연결 종료 테스트와 다른 항목의 오류 처리도 통과했다. 게임 코드 변경은 GameClient.cpp의 해당 이름 처리 부분뿐이며 커밋·푸시는 하지 않았다.
### 2026-11-13 최신 인계 — 11/12 대기실 정보·화면까지, 퇴장은 다음 작업

- 이 기록은 위의 과거 미구현 상태보다 우선한다. 사용자는 여기서 기능 작업을 멈추고 노트북에서 이어가려고 기록 및 커밋을 요청했다. 현재 브랜치는 CJH다. 11/11은 9cebd1f(QueueRequest), fd8ddc8(방향키·Enter·선택 표시), 07a90a8(입장 응답·결과 화면)까지 원격에 반영됐다.
- 11/12 완료: GameClient의 roomPlayers[2] 저장 공간 및 Init/HandleDisconnect/입장 성공 시 초기화, ProcessPacket의 S2C_ROOM_INFO 수신·현재 방/접속 여부/캐릭터 확인·빈 자리 정리·두 자리 일괄 저장, GetRoomPlayer의 읽기 전용 조회, drawScene → renderRoomEntry의 두 자리 닉네임·캐릭터·내 자리(You)·빈 자리 표시다. 이름은 사용자 요청으로 수신 시 영문·숫자/빈 이름을 재검사하지 않고 마지막 칸에 '\0'만 넣는다. 로그인 이름 검사는 유지한다.
- 11/12 남은 작업은 (4) 대기실 퇴장 입력과 C2S_LeaveRoom → QueueRequest 연결, (5) 요청 등록 성공 후 방·자리·참가자 정보를 비우고 CLIENT_LOBBY로 복귀하며 닉네임·세션·연결 유지, (6) 테스트 서버의 퇴장/목록 갱신 및 입장 → 대기실 → 퇴장 → 로비 → 재입장 전체 테스트다. Backspace는 나가기 키로 제안한 상태이며 아직 코드에 없다. 별도의 S2C_LeaveRoomResult는 현 프로토콜에 없으므로 임의 추가하지 않는다. 실제 팀 서버의 퇴장 후 방 목록 전송은 연동 시 확인한다. 캐릭터 선택·게임 시작 요청은 11/13 일정이며 아직 구현하지 않았다.
- 학습 방식: 사용자는 함수별 설명 전에 '닉네임 입력 → START → 패킷 등록 → send → 서버 응답 → recv → 저장 → drawScene → 조회 → 화면'의 구체적인 예시 흐름을 자세히 설명받으면 이해가 쉽다. 한 단계 구현 후 코드/현재 줄 번호/문법/목적을 설명하고 다음 구현은 요청받는다. 화면 구현을 설명하는 동안 수신 코드를 학습 중이면 다음 변경으로 임의 확장하지 않는다. 과도한 중복 검사를 피하고 필요한 경계 처리와 게임 규칙 검사를 구분해 설명한다.
- 개념 구분: S2C_JoinRoomResult는 입장 성공 및 내 방/자리 번호, S2C_RoomInfo는 두 참가자의 이름/캐릭터다. rooms/GetRoom은 로비의 방 요약 목록, roomPlayers/GetRoomPlayer는 입장한 방의 참가자 정보다. GetRoomPlayer는 서버 통신이 아니라 이미 저장한 배열 주소 조회이며 renderRoomEntry는 이를 그린다.
- 검증: Debug x64 빌드, 로컬 TCP의 분할/연속 수신·참가자 갱신·빈 자리 정리·이름 종료 보장·잘못된 값·연결 종료 테스트, 실제 OpenGL 렌더링의 두 사람/캐릭터 미선택/빈 자리 및 1200×675·960×540 이미지 확인을 통과했다. 실제 팀 서버 연동은 미검증이다. 이름 검사 단순화 이후 빌드·TCP 테스트도 통과했다.
- 노트북에서는 CJH 최신 원격을 가져오고 이 기록과 docs/CLIENT_PROGRESS.md의 최신 항목부터 읽는다. 진행 문서는 이전 원격 삭제 이후 로컬에 보존했던 전체 이력이며, 이번 이어하기 기록 요청에 따라 다시 원격에 포함한다. 최신 달력 DOCX가 없으면 진행 문서의 날짜별 확정표를 따른다.
- title.cpp는 기존 CP949 인코딩을 유지한다. GameClient.h/.cpp, Main.cpp, title.h는 UTF-8 BOM이다. 인코딩을 무심코 바꾸지 않는다. 로컬 SDK·OpenGL/vcpkg 환경은 PC마다 확인한다. .vs의 테스트 실행 파일·미리보기 서버·스크립트는 무시되는 로컬 자료여서 노트북에 자동으로 전달되지 않는다. 필요 시 테스트 서버를 다시 준비한다. 마지막 미리보기 서버 상태는 closed였고, 스크립트에는 입장 성공 후 PreviewBot/Steve와 로그인 이름/CHAR_NONE의 대기실 정보 전송까지 준비했다.
- 기능 커밋에 기존 사용자 주석·정원 초과 한국어 문구·NetworkClient(TCP_NODELAY 제거 포함)/Snow 수정·사용자 DOCX/LLD/이미지를 섞지 않고 로컬에 보존한다. 정원 초과 한국어 문구는 현재 ASCII 글꼴에서 정상 표시되지 않을 수 있으며 한글 글꼴 처리는 구현하지 않았다. 기존 stash도 유지한다. 이번 요청 이후 새 기능은 구현하지 않고, 현재 범위 코드와 이어하기 문서를 별도 커밋으로 전달한다.
- 이번 대기실 정보 수신·표시 기능 커밋은 544614c이며, 기존 사용자 수정을 제외한 커밋 소스로 Debug x64 전체 재빌드를 통과했다. 이 인계 문서는 별도 문서 커밋으로 전달한다. 노트북에서 시작할 기능 단계는 11/12의 퇴장 요청이다.
