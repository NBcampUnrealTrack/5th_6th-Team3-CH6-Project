![꾸미기 스크린샷 2026-03-30 155629](https://github.com/user-attachments/assets/4d5b8dcb-921d-436b-a8ca-5312db1cc561)
# Desecration

> 데몬즈소울과 다크소울 1에서 영감을 받은 소울라이크 3D 액션 RPG  
> 언리얼 엔진의 다양한 시스템을 활용하여 AAA급 게임 제작 과정을 축소판으로 경험하고, 최종 패키징과 스팀 출시를 목표로 개발 중인 프로젝트입니다.

---

## 🎮 프로젝트 소개

### 기획 의도
평소 즐겨하던 소울라이크 RPG를 직접 만들어보고 싶다는 목표에서 시작했습니다.  
언리얼 엔진의 다양한 기능을 학습하며 기술적·디자인적으로 충분히 구현 가능하다고 판단하여, 실제 AAA급 게임 제작 프로세스를 축소판으로 경험할 수 있도록 프로젝트를 기획했습니다.

게임의 난이도는 **캐릭터의 스펙 성장 40% : 플레이어의 컨트롤 성장 60%** 정도로 설계하여, 어려운 보스를 클리어했을 때 높은 성취감을 느낄 수 있도록 디자인했습니다.

### 게임 진행 구조
```text
튜토리얼 → 1스테이지 → 2스테이지 → 3스테이지 → 4스테이지(예정) → 최종 보스(예정)
```

- 각 스테이지의 보스를 처치하면 다음 스테이지가 해금됩니다.
- 최종 보스를 클리어하면 회차 계승(New Game+)이 가능하며, 전체 난이도가 상승합니다.
- 몬스터와 상자를 통해 룬 및 룬 파편을 획득할 수 있습니다.
- 획득한 자원으로 캐릭터를 성장시키고 장비를 강화할 수 있습니다.

---

## ✨ 주요 시스템

### 🔹 성장 시스템
- 몬스터 및 상자에서 획득한 룬으로 레벨 업
- 룬 파편 조합을 통한 룬 합성
- 중간 보스 및 보스 처치 시 획득하는 재료로 장비 강화
- 회차 계승(New Game+) 시스템 예정

### 🔹 전투 시스템
- 기본 공격 / 패링 / 회피 기반의 소울라이크 전투
- 다양한 플레이 스타일을 가진 클래스
- 보스별 고유 패턴 및 기믹
- 스킬 스위칭 및 포션 시스템

### 🔹 클래스
- 팔라딘
- 도사
- 발키리

---

## 🎮 조작법

| 입력 | 동작 | 설명 |
|------|------|------|
| `W` `A` `S` `D` | 이동 | 캐릭터 이동 |
| `F` | 상호작용 | 오브젝트 상호작용 |
| `LMB` | 공격 | 기본 공격 |
| `RMB` | 막기 / 패링 | 방어 및 패링 |
| `ESC` | 메뉴 | 메뉴 열기 / 닫기 |
| `I` | 인벤토리 | 인벤토리 열기 / 닫기 |
| `Q` | 스킬 사용 | 현재 스킬 사용 |
| `Shift` | 스킬 스위칭 | 스킬 변경 |
| `1` | 회복 포션 사용 | 체력 회복 |
| `2` | 회복 포션 스위칭 | 포션 종류 변경 |
| `E` | 버프 포션 사용 | 버프 포션 사용 |
| `R` | 버프 포션 스위칭 | 버프 포션 종류 변경 |

---

## 🔄 게임 흐름

```text
[Phase 1] 시작 / 튜토리얼
 └─ 플레이어가 게임을 시작하고 기본 조작을 익힙니다.

[Phase 2] 메인 게임플레이
 └─ 스테이지를 진행하며 몬스터와 보스를 처치합니다.
 └─ 마을에서 레벨 업, 장비 강화, 룬 조합을 통해 캐릭터를 성장시킵니다.

[Phase 3] 보스 / 클리어
 └─ 각 스테이지의 보스를 처치하면 다음 스테이지가 열립니다.
 └─ 최종 보스를 처치하면 회차를 계승하여 다시 플레이할 수 있습니다.
```

---

## 🛠 기술 스택

| 분류 | 기술 |
|------|------|
| 🎮 엔진 | Unreal Engine 5.7 |
| 💻 언어 | C++ |
| 📝 비주얼 스크립트 | Blueprint |
| 🔧 IDE | Visual Studio 2022 / Rider |
| 🐙 버전 관리 | Git / GitHub Desktop / Sourcetree |
| 🎨 디자인 | Miro |
| 💬 협업 | Notion / Slack / Discord |

---

## 🔌 사용 플러그인

| 플러그인 | 용도 |
|----------|------|
| Motion Warping | 공격 및 이동 애니메이션을 상황에 맞게 보정하여 자연스러운 전투 연출 구현 |
| Procedural Vegetation Editor | 레벨의 지형 및 자연 환경을 효율적으로 배치 |
| VARCO Sounds | 환경음 및 효과음 제작 |
| IconCreater | UI 아이콘 및 아이템 아이콘 제작 |

---

## 🛒 사용한 마켓플레이스 에셋


### 👤 캐릭터

| 구분 | 에셋 | 링크 |
|------|------|------|
| 팔라딘 - 캐릭터 | Dark Knight - Male and Female | https://fab.com/s/057b6c0390a0 |
| 팔라딘 - 애니메이션 | Sword and Shield Animation Pack | https://fab.com/s/87b75ea1c755 |
| 팔라딘 - 방패 | Eye Round Shield | https://fab.com/s/b13cf7192edd |
| 도사 - 캐릭터 | Samurai Ronin (Modular) | https://fab.com/s/fe59137958ea |
| 도사 - 공격 모션 | Throwing Animation Pack | https://fab.com/s/d63dcd9ac85f |
| 도사 - 무기 | Folding Fan | https://fab.com/s/3774b006efea |
| 도사 - 소환수 | Medhue Tiger | https://fab.com/s/acdf31bd9cfa |
| 발키리 - 캐릭터 | Angel Valkyrie | https://fab.com/s/503640f74a79 |
| 발키리 - 애니메이션 | Essential Great Sword Animation Pack | https://fab.com/s/04dd2001f4a3 |
| 공통 - 회복 모션 | Healing Animation Pack | https://fab.com/s/1497aaaf7192 |
| 공통 - 사다리 모션 | Climbing Ladder Pro - MoCap Animation Pack | https://fab.com/s/9ff3f1cf791b |


### 👹 몬스터

| 에셋 | 링크 |
|------|------|
| Demon Warrior | https://fab.com/s/40823748700d |
| Demon Woman | https://fab.com/s/877d7d84d62e |
| Skeleton Enemy | https://fab.com/s/289262fbdcb9 |
| Realistic Blood VFX (Niagara Blood Effects) | https://fab.com/s/5eaee75530cf |
| Death Knight - Armor Fantasy RPG Dungeon Warrior Monster Medieval Boss Character | https://fab.com/s/fd6f35d6fd6d |
| Bossy Enemy Animation Pack | https://fab.com/s/e96f0bb76c90 |


### 👑 보스 몬스터

| 에셋 | 링크 |
|------|------|
| Dragon | https://fab.com/s/653e0d985877 |
| Demon Executioner | https://fab.com/s/547be0d0bee7 |
| Demon God Raijin 01 | https://fab.com/s/b4a6ae52158a |
| Fallen Angel | https://fab.com/s/059936bbb76c |


### 🗺️ 레벨 / 환경 에셋

#### 메인 맵

| 에셋 | 링크 |
|------|------|
| Laketown | https://fab.com/s/5d5a5442f969 |
| Abandoned Cathedral | https://fab.com/s/2d49121c2c79 |
| Dark Castle | https://fab.com/s/7a6bd30ef52d |
| Traditional Chinese Style Park | https://fab.com/s/5762351d4e9b |

#### 지형 / 자연 환경

| 에셋 | 링크 |
|------|------|
| Lowpoly Realistic Rock [FREE] | https://fab.com/s/1a5fcb0ca8ad |
| Rock Environment Pack | https://fab.com/s/02c5961a5c17 |
| Desert Western Cliff Layered Large 03 | https://fab.com/s/38509138f484 |
| Quarry Cliff 01 | https://fab.com/s/495fd84fa5a1 |
| Quarry Cliff 02 | https://fab.com/s/b6072f0676a7 |
| Quarry Cliff 03 | https://fab.com/s/e70e6da62af1 |
| Quarry Cliff 04 | https://fab.com/s/77413986a06e |
| Quarry Cliff 05 | https://fab.com/s/201ca3fbfa11 |
| YF-Env: Cliff Pine Tree | https://fab.com/s/c90d1a3a229a |
| Water Environment Essentials | https://fab.com/s/be4af125f3a4 |

#### 오브젝트 / 구조물

| 구분 | 에셋 | 링크 |
|------|------|------|
| 포탈 | Stylized Portals VFX | https://fab.com/s/214c449296e2 |
| 엘리베이터 | Fantasy Elevators | https://fab.com/s/20528c39bbbe |
| 사다리 | Climbing Ladder Pro - MoCap Animation Pack | https://fab.com/s/9ff3f1cf791b |
| 발리스타 | Balista Medieval | https://skfb.ly/oSzVV |
| 바리케이드 | Wooden Barricade | https://skfb.ly/6WUBE |
| 투석기 | Trebuchet Fantasy | https://skfb.ly/XGTL |
| 장식용 무기 | Free Fantasy Weapon Sample Pack | https://fab.com/s/f96b04bf745e |
| 인테리어 가구 | Old West VOL.1 - Interior Furniture | https://fab.com/s/7f2f0af60dd7 |
| 문 | Door | https://skfb.ly/6ZWw8 |
| 로프 다리 | Rope Bridges - Dynamic Responsive & Efficient | https://fab.com/s/9ad854b78da5 |

## 📌 에셋 활용 방식

- 모든 캐릭터 및 몬스터 에셋은 프로젝트 콘셉트에 맞게 머티리얼, 애니메이션, 이펙트 등을 수정하여 사용했습니다.
- 레벨 에셋은 맵 구조에 맞춰 일부 메쉬를 재배치하고, 콜리전 및 라이팅 설정을 추가로 조정했습니다.
- 애니메이션 에셋은 Motion Warping 및 블렌드 스페이스를 적용하여 보다 자연스럽게 연결되도록 구성했습니다.

---

## 📁 프로젝트 구조

```text
Source/Desecration/
├── Public/
│   ├── Equipment/
│   ├── GameSystem/
│   ├── Interaction/
│   ├── Item/
│   │   ├── Component/
│   │   ├── Data/
│   │   └── Rune/
│   ├── Monster/
│   │   └── Interface/
│   ├── Player/
│   │   ├── Paladin/
│   │   ├── Taoist/
│   │   └── Valkyrie/
│   ├── Shop/
│   │   ├── Component/
│   │   └── Data/
│   └── UI/
│       └── Rune/
└── Private/
    ├── Equipment/
    ├── GameSystem/
    ├── Interaction/
    ├── Item/
    │   ├── Component/
    │   └── Rune/
    ├── Monster/
    ├── Player/
    │   ├── Paladin/
    │   ├── Taoist/
    │   └── Valkyrie/
    ├── Shop/
    │   └── Component/
    └── UI/
        └── Rune/
```

---

## 🧱 주요 클래스

| 클래스명 | 역할 | 주요 기능 |
|------|------|------|
| `T3GameMode` | 게임 모드 | 캐릭터 정보 로드, 사망 시 돈 드랍 등 핵심 게임 규칙 |
| `T3CharacterBase` | 플레이어 캐릭터 | 이동, 전투, 상호작용, 컴포넌트 관리 |
| `T3PlayerController` | 플레이어 컨트롤러 | 입력 처리 및 UI 관리 |
| `T3WorldSubsystem` | 월드 서브시스템 | 상호작용 액터 상태 저장 |
| `T3MonsterBase` / `T3BossBase` / `T3MidBossBase` | 적 베이스 | 몬스터 공통 로직 |
| `T3InventoryComponent` | 인벤토리 | 아이템 관리 |
| `T3CombatComponent` | 전투 컴포넌트 | 전투 로직 |
| `T3GameInstance` | 게임 인스턴스 | 세이브 / 로드, 레벨 이동, 로딩 UI 관리 |

---

## 📈 개발 진행률

| 항목 | 진행률 |
|------|------|
| 전체 완성도 | 80% |
| 코어 시스템 | 100% |
| 콘텐츠 | 80% |
| 폴리싱 | 70% |

---

## 👥 팀 소개

| 이름 | 역할 | 담당 |
|------|------|------|
| 노재욱 | 팀장 / 메인 프로그래머 | 보스 몬스터, 패키징, UI 디자인, 기획 |
| 이규진 | 부팀장 / 프로그래머 | 인벤토리, 아이템, 룬, 상점, 강화소 UI |
| 원유연 | 레벨 디자이너 | 레벨 디자인, 환경, 오브젝트 |
| 박진연 | 캐릭터 | 캐릭터 클래스, 애니메이션, 상호작용 오브젝트 |
| 문승재 | 캐릭터 | 캐릭터 클래스 |
| 신지용 | 일반 몬스터 | 일반 몬스터 제작 |
| 손우참 | 프로그래머 / 중간 보스 | 중간 보스 몬스터, 강화 시스템 |
| 유용선 | 레벨 디자이너 | 레벨 디자인 |
| 박영빈 | 게임 시스템 | 저장 시스템, 환경설정, 타이틀 UI |
| 김태은 | UI 그래픽 / 프롭스 | UI 그래픽 디자인, 프롭스 배치 |

---

## 📚 회고

### 잘한 점
- 기획 방향을 명확하게 정하여 프로젝트가 흔들리지 않도록 함
- 기능 구현과 연출 작업을 동시에 진행하여 완성도를 높임
- 매일 데일리 브리핑과 개별 회의를 통해 진행 상황을 공유

### 개선할 점
- 회의록과 기획서 문서화 부족
- 업무 분배와 PM 능력의 한계로 일부 팀원에게 업무가 편중됨
- 개인별 역량에 맞는 세부 마일스톤 설정 필요

### 프로젝트를 통해 배운 점
- 보스 AI, Motion Matching, StateTree, GameplayTag 등 UE5 심화 기능 활용
- 협업 시 인터페이스와 Delegate를 활용한 느슨한 결합의 중요성
- 레벨 디자인에서 충돌, 최적화, 패키징 환경까지 고려해야 함
- UI를 단순 시각 요소가 아닌 하나의 시스템으로 설계하는 방법 학습

---

## 📎 참고 자료

- Unreal Engine Documentation
- State Tree in Unreal Engine
- Motion Warping in Unreal Engine

© 2025 TriForce Team, NbcampUnreal.5th_6th-Team3-CH6-Project. All rights reserved.
