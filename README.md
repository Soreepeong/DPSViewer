# DPSViewer

![DOT](https://raw.githubusercontent.com/Soreepeong/DPSViewer/master/readme-images/en-dot.png)
![DPS](https://raw.githubusercontent.com/Soreepeong/DPSViewer/master/readme-images/en-dps.png)
![Config](https://raw.githubusercontent.com/Soreepeong/DPSViewer/master/readme-images/en-config.png)
![Chat](https://raw.githubusercontent.com/Soreepeong/DPSViewer/master/readme-images/en-chat-translate.png)


DPSViewer is a lightweight DPS meter for FFXIV (v4.05 for Global version and v3.41 for Korean version).
It supports both DX9 and DX11 versions.

  - See the DPS of everyone around you
  - Take multiple screenshots without delay
  - See your DoT status even when there are too many DoTs (in 24-people raids)
  - Parse only yourself, your party, your alliance, or everyone
  - Translate chat in game
  - Use 2 oGCD skills in 1 GCD without delay even with bad latency

Installation
----
1. If you don't have Visual C++ 2017 Redist or .NET 4.5.2, then install the following:
Redist https://go.microsoft.com/fwlink/?LinkId=746572
.NET https://www.microsoft.com/en-us/download/details.aspx?id=42643
2. If you launched FFXIV as administrator, you will have to run the program in administrator mode.

Note
----
It will NOT work after any FFXIV update. In that case, please wait for me to update the program.

About using 2 oGCD skills: The program will make the client think that the skill has been cancelled, hence making it not wait for server to acknowledge the skill. Server will still process the skill (including cast); if you move during cast, the server will cancel the skill. All the casts will be immediately "interrupted", but you cannot use any skill during the cast anyway. This trick will hide some of your damages dealt from showing in the client, so there is an option to make the client try to display the damage twice. When using with ACT, it still works well when you use Parse from Network option of FFXIV ACT Plugin.


# DPSViewer (한국어 설명)

DPSViewer는 파이널 판타지 14를 위한 가벼운 DPS 미터 프로그램입니다.
글로벌 서버는 v4.05, 한국 서버는 v3.41 까지 지원합니다. 파이널 판타지 업데이트 이후에는 사용 불가능해지니 그 때는 이 프로그램의 업데이트를 기다리셔야 합니다.
현재 DX9 및 DX11 버전을 지원합니다.

  - 주변 모두의 DPS 보기.
  - 대기시간 없이 스크린샷 연사.
  - DoT가 안 떠도 예측해서 시간 재어 보여줌. (24인 레이드)
  - 연합 및 파티만 보기.
  - 인터넷 상태가 좋지 않아도 글쿨스킬 사이에 2비글쿨스킬 끼울 수 있도록 하기.

설치
----
1. Visual C++ 2017 Redist x86 또는 .NET 4.5.2가 없다면 다음을 설치합니다.
Redist https://go.microsoft.com/fwlink/?LinkId=746572
.NET https://www.microsoft.com/en-us/download/details.aspx?id=42643
2. 관리자 권한으로 (한국어 버전일 경우 런처로) 파이널 판타지를 켰을 경우, 이 프로그램도 관리자 권한으로 실행하셔야 합니다.

글쿨 스킬에 대하여
----
스킬 시전 시 게임 클라이언트가 스킬 시전을 취소한 것으로 취급하도록 만들어, 클라이언트가 서버로부터 스킬 이용 확인을 받을 필요를 없게 만듭니다. 서버는 여전히 스킬 및 캐스팅을 처리합니다. 캐스팅 시에는 즉시 취소됨 움직이면 서버 측에서 캐스팅이 취소되며, 쿨타임이 클라이언트 측에서 꼬여도 서버 측에서 잘못된 쿨타임에 스킬을 시전했을 시 취소시켜 줍니다. 이 기능을 사용하면 종종 직접 딜한 양이 클라이언트 내에서 표시가 안 되어서, 클라이언트가 대미지를 두 번 보이게 시도시키는 옵션이 있습니다. 이 기능을 쓰든 안 쓰든 이 프로그램의 DPS 미터 및 ACT에서의 집계에는 변동이 없습니다.

License / 라이선스
----
GPLv3

Referenced projects / 참조 프로젝트
----
- FFXIV.Memory and FFXIV_ACT_Plugin for signatures
- nlohmann/json
- MinHook
- zlib
- imgui
- https://github.com/nlohmann/json
