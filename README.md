# DPSViewer

![DPS Viewer](https://raw.githubusercontent.com/soreepeong/DPSViewer/master/readme-images/window.png)
![DPS Meter](https://raw.githubusercontent.com/soreepeong/DPSViewer/master/readme-images/meter-dps.png)
![DoT Meter](https://raw.githubusercontent.com/soreepeong/DPSViewer/master/readme-images/meter-dot.png)


DPSViewer is a lightweight DPS meter for FFXIV (v3.57 for Global version and v3.3 for Korean version).

  - See the DPS of everyone around you
  - Take multiple screenshots without delay
  - See your DoT status even when there are too many DoTs

Installation
----
1. Copy zlib1.dll to FFXIV installation directory or C:\Windows.
2. If you don't have Visual C++ 2015 Redist or .NET 4.5.2, then install the following:
Redist https://www.microsoft.com/en-us/download/details.aspx?id=481453
.NET https://www.microsoft.com/en-us/download/details.aspx?id=42643
3. If you launched FFXIV as administrator, you will have to run the program in administrator mode.

Note
----
It will NOT work after any FFXIV update. In that case, please wait for me to update the program.

# DPSViewer (한국어 설명)

DPSViewer는 파이널 판타지 14를 위한 가벼운 DPS 미터 프로그램입니다.
글로벌 서버는 v3.57, 한국 서버는 v3.3 까지 지원합니다. 파이널 판타지 업데이트 이후에는 사용 불가능해지니 그 때는 이 프로그램의 업데이트를 기다리셔야 합니다.

  - 주변 모두의 DPS 보기.
  - 대기시간 없이 스크린샷 연사.
  - DoT가 안 떠도 예측해서 시간재어 보여줌.

설치
----
1. Visual C++ 2017 Redist x86 또는 .NET 4.5.2가 없다면 다음을 설치합니다.
Redist https://go.microsoft.com/fwlink/?LinkId=746572
.NET https://www.microsoft.com/en-us/download/details.aspx?id=42643
2. 관리자 권한으로 (한국어 버전일 경우 런처로) 파이널 판타지를 켰을 경우, 이 프로그램도 관리자 권한으로 실행하셔야 합니다.

License / 라이선스
----
GPLv3

Referenced projects / 참조 프로젝트
----
- FFXIV.Memory (NOTE: Some internal functions are edited to public)
- MinHook
- zlib
- imgui
