# 현재 상태: 기존 게임 이식 단계

현재 진행: 기존 게임 이식과 초기화·종료 준비, 11/2 NetworkClient 연결 수명 관리에 이어 **11/3 QueuePacket / FlushSendBuffer / ReceiveData**까지 구현했다. 패킷 추출과 GameClient·Main 연결은 아직 하지 않았다. 이후 기능은 사용자 요청에 따라 단계별로 진행한다.

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
