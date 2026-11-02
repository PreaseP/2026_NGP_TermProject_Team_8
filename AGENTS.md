# 작업 방식

- 사용자는 구현 과정을 이해하며 한 단계씩 함께 진행하기를 원한다. 완성본을 한꺼번에 구현하거나 다음 단계로 임의 확장하지 않는다.
- 현재 승인된 단계는 11/2 일정의 NetworkClient::InitNetwork/ConnectToServer/IsConnected/Disconnect/Shutdown 구현까지다. 이후 송수신·GameClient·Main 연결은 일정과 사용자 요청에 맞춰 진행한다.
- 사용자가 수정한 기존 코드 주석은 보존한다. 이번 단계는 새 파일에 [11.2] 표시를 사용한다.
- 기존 솔루션과 client 프로젝트를 유지한다. 원본 게임 저장소, server, 사용자 문서와 common/protocol.h는 보존한다.
- 이후 작업 전에 현재 단계의 목적과 변경 파일을 짧게 설명하고 사용자 지시에 맞춰 진행한다.
- 진행 상태는 docs/CLIENT_PROGRESS.md에 기록한다. 한국어로 설명한다.
