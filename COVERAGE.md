# 코드 커버리지 측정 가이드

커버리지 측정은 **개발/디버그 환경(MSVC)과 완전히 분리**되어 있습니다.  
`measure_coverage.ps1` 스크립트 하나로 clang-cl 빌드 → 테스트 실행 → 리포트 출력까지 자동으로 진행됩니다.  
`.vcxproj`와 기존 빌드 설정은 변경되지 않습니다.

---

## 측정 도구

| 도구 | 역할 |
|------|------|
| `clang-cl` | MSVC 호환 Clang 프론트엔드 — 커버리지 계측 컴파일 |
| `llvm-profdata` | 실행 후 생성된 `.profraw` 파일을 `.profdata`로 병합 |
| `llvm-cov` | `.profdata` 기반 라인 / 함수 / 브랜치 커버리지 리포트 출력 |

> MSVC(`cl.exe`)는 커버리지 빌드에 사용하지 않습니다.  
> MSVC PDB 기반 도구(OpenCppCoverage 등)는 라인 커버리지만 지원하며 브랜치/함수는 측정 불가합니다.

---

## 사전 준비 — LLVM 설치

터미널에서 한 번만 실행합니다.

```powershell
winget install LLVM.LLVM
```

설치 후 `C:\Program Files\LLVM\bin\clang-cl.exe` 가 존재하면 준비 완료입니다.

---

## 커버리지 측정 실행

프로젝트 루트에서 실행합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\measure_coverage.ps1
```

스크립트가 아래 4단계를 자동으로 수행합니다.

```
[1/4] clang-cl 빌드  →  coverage_clang\test_coverage.exe
[2/4] 테스트 실행    →  coverage_clang\coverage.profraw
[3/4] profdata 병합  →  coverage_clang\coverage.profdata
[4/4] 리포트 출력   →  콘솔 + coverage_clang\html\index.html
```

---

## 출력 예시

```
Filename              Regions  Missed  Cover   Functions  Missed  Executed   Lines  Missed  Cover   Branches  Missed  Cover
-----------------------------------------------------------------------------------------------------------------------------
Checker.cpp               217      14  93.55%           7       1    85.71%    294      40  86.39%       190      25  86.84%
DebugController.cpp       116      84  27.59%          16       4    75.00%    205     114  44.39%        72      57  20.83%
Environment.cpp            37       5  86.49%           5       0   100.00%     51       8  84.31%        20       5  75.00%
Executor.cpp              209      16  92.34%          22       1    95.45%    301      27  91.03%       146      18  87.67%
Parser.cpp                144       1  99.31%          30       0   100.00%    342       3  99.12%        90       4  95.56%
Shell.cpp                 181      30  83.43%          23       2    91.30%    263      56  78.71%       155      38  75.48%
Tokenizer.cpp              68       0 100.00%           7       0   100.00%     68       0 100.00%        52       3  94.23%
-----------------------------------------------------------------------------------------------------------------------------
TOTAL                    1000     154  84.60%         137      12    91.24%   1556     252  83.80%       725     150  79.31%
```

| 지표 | 설명 |
|------|------|
| **Lines** | 실행된 소스 줄 비율 |
| **Functions** | 한 번 이상 호출된 함수 비율 |
| **Branches** | `if` / `switch` / 삼항 등 분기 경로 비율 |
| **Regions** | llvm-cov 내부 분기 단위 (Branches보다 세분화) |

HTML 리포트(`coverage_clang\html\index.html`)를 브라우저로 열면 미커버 줄을 소스 코드 상에서 직접 확인할 수 있습니다.

---

## 출력 파일 구조

```
coverage_clang/
├── test_coverage.exe   # 커버리지 계측 빌드 바이너리
├── coverage.profraw    # 실행 후 원시 프로파일 데이터
├── coverage.profdata   # 병합된 프로파일 데이터
└── html/
    └── index.html      # 소스 레벨 HTML 리포트
```

> `coverage_clang/` 디렉터리는 `.gitignore`에 등록되어 있습니다.

---

## 주의 사항

- 커버리지 빌드는 계측 코드가 삽입되므로 실행 속도가 다소 느릴 수 있습니다.
- `measure_coverage.ps1`은 매 실행 시 전체를 재빌드합니다.
- 테스트 코드(`tests/`)와 GTest/GMock 소스는 리포트에서 제외됩니다.
