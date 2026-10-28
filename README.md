# Fuzz Pedal Source

JUCE 기반 mono/stereo 퍼즈 effect의 독립 source edition입니다. 게인·믹스·게이트 3개 knob, 톤 3개, 클리핑 3개, 바이패스 및 host state 저장/복원을 제공합니다. 기존 설치본과 다른 bundle ID·plugin code·제품 이름을 사용합니다.

## 신호 처리

- 게인: 입력 gain과 sqrt 보상, 하드·tanh 소프트·비대칭 clipping
- 톤: 800 / 2000 / 4500 Hz low-pass. 낮은 sample rate에서는 Nyquist 아래로 제한합니다.
- 믹스: 원음과 wet 경로를 혼합합니다. 믹스 0에서는 게이트 값과 무관하게 finite 원음을 그대로 통과시킵니다.
- 게이트: wet 경로 입력 5 ms attack / 50 ms release, 출력 20 ms attack / 500 ms release
- 바이패스: finite 원음 통과. NaN/Infinity 입력은 0으로 정리해 DSP state 오염을 막습니다.
- parameter는 block마다 atomic snapshot으로 읽습니다. 잘못된·중복·누락·비정상 수치 state는 전체 거부하고 기존 값을 유지합니다.

## 빌드

CMake 3.22 이상, C++17 compiler, macOS Xcode Command Line Tools가 필요합니다. JUCE 8.0.16-security의 immutable commit/archive SHA-256를 CMake에 고정했습니다. 최초 configure에서 공식 source archive를 다운로드합니다.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build -j 2
ctest --test-dir build --output-on-failure
```

`COPY_PLUGIN_AFTER_BUILD FALSE`이므로 build가 시스템 plugin directory에 설치하거나 기존 plugin을 덮어쓰지 않습니다. macOS에서는 AU·VST3·Standalone을 별도 build directory에 만듭니다. 실제 검증 범위는 macOS arm64이며 Windows/Linux 및 개별 DAW 호환성·청감 품질은 검증했다고 주장하지 않습니다.

## 수동 설치

아래 명령은 사용자가 새 source edition을 설치하려는 경우에만 실행합니다. build/test 절차는 이 명령을 실행하지 않습니다.

```sh
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
ditto "build/FuzzPedalSource_artefacts/Release/VST3/Fuzz Pedal Source.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/Fuzz Pedal Source.vst3"
mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
ditto "build/FuzzPedalSource_artefacts/Release/AU/Fuzz Pedal Source.component" "$HOME/Library/Audio/Plug-Ins/Components/Fuzz Pedal Source.component"
```

이 명령의 목적지는 source edition 이름입니다. 기존 제품이나 host session을 대체하지 않습니다. 설치 후 host가 이 별도 plugin을 인식하는지는 해당 DAW에서 확인해야 합니다. Standalone은 오디오 입력을 사용하므로 macOS 입력 권한 요청과 장치 선택이 필요할 수 있습니다. 자동 권한 부여·장치 설정 변경을 수행하지 않습니다.

## 장치 없는 검증

기존 processor smoke 3개를 보존하고 dry/Gate·bypass·state·0/mono/stereo·sample rate·reset·automation·finite·allocator·WAV readback 회귀를 추가했습니다. 테스트에는 오디오 장치·마이크·녹음이 필요하지 않습니다. WAV는 build/test-artifacts에만 생성합니다.

`EditorHarness`는 `BUILD_TESTING` 안의 native editor 검증 target입니다. 실제 product의 `createEditor()`를 표시하고 실제 APVTS 값을 지정한 전용 directory에 기록합니다. 오디오 장치나 MIDI device를 열지 않으며 global plugin directory를 검색하지 않습니다.

```sh
mkdir -p build/editor-qa-profile
chmod 700 build/editor-qa-profile
"build/EditorHarness_artefacts/Release/Fuzz Pedal Source Editor QA.app/Contents/MacOS/Fuzz Pedal Source Editor QA" --output-dir="$(pwd)/build/editor-qa-profile"
```

창 종료 시 해당 directory에 실제 parameter state를 저장하고 같은 directory로 재실행하면 복원합니다. 이 test-only container는 shipping Standalone의 입력 장치나 DAW compatibility를 검증하는 도구가 아닙니다.

## 라이선스

JUCE 결합작업 SPDX 선택은 **AGPL-3.0-only**입니다. 자체 코드의 기존 MIT 허용과 외부 dependency license를 구분합니다. 전체 plugin binary가 MIT라고 표현하지 않습니다. [LICENSE](LICENSE), [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md)를 확인하세요. Native UI의 ‘라이선스와 소스’에서 권리·무보증 고지를 확인할 수 있습니다.

## Host numeric locale와 state

State의 decimal 숫자는 stream 전용 classic locale로 전체 token을 읽습니다. Host의 process-global `LC_NUMERIC`를 변경하지 않으며 suffix·NaN·Infinity·범위 밖·중복·누락·fractional choice 상태는 기존 값에 적용하지 않습니다. `locale-state-C`, `locale-state-comma` CTest는 각각 별도 test subprocess에서만 numeric locale을 설정해 실제 processor의 저장 → control/DSP reset → 복원과 invalid-state 보존을 검증합니다. 사용자·OS·DAW locale 설정을 바꾸지 않습니다. CI의 전체 CTest에도 이 두 검사가 포함됩니다.
