# 라이선스 고지

이 source edition의 JUCE 결합작업에 선택한 SPDX identifier는 **AGPL-3.0-only**입니다. GNU AGPL version 3 전문은 [LICENSE](LICENSE)에 있습니다. 보증 없이 제공하며 사용·복사·수정·배포 권리와 조건을 그대로 유지합니다. 수정일: 2026-10-28, Sungwoon Jeon.

자체 effect 구현의 기존 MIT 허용은 [MIT-OWN-COMPONENTS](LICENSES/MIT-OWN-COMPONENTS.txt)에 보존합니다. 자체 코드 파일의 SPDX `MIT OR AGPL-3.0-only`는 그 코드의 선택지를 나타냅니다. JUCE와 결합한 plugin 전체가 MIT라는 의미가 아닙니다. 원본 자료와 그 권리 표시는 변경하지 않았습니다.

## JUCE

- 공식 tag: `8.0.16-security`
- immutable commit: `ea77444750de45e036e0279bb0291144bf8e3688`
- [공식 LICENSE.md](https://raw.githubusercontent.com/juce-framework/JUCE/ea77444750de45e036e0279bb0291144bf8e3688/LICENSE.md), [그대로 보존한 전문](LICENSES/JUCE-LICENSE.md)
- JUCE modules는 AGPLv3/commercial dual license입니다. 이 edition은 AGPLv3 경로를 선택하며 commercial JUCE license를 취득했다고 주장하지 않습니다.
- [공식 변경 기록](https://github.com/juce-framework/JUCE/blob/ea77444750de45e036e0279bb0291144bf8e3688/CHANGE_LIST.md)은 8.0.16에서 malformed String input 보호를 명시합니다. 이 pin은 모든 취약점 부재를 보증하지 않습니다.
- FetchContent URL archive SHA-256: `79181e00e8511c837983478f561db082ed08028d60c53a0af8ac85ea220299da`

JUCE가 포함하는 dependency는 각각의 license를 따릅니다. AudioUnitSDK, VST3 SDK, FLAC, Ogg Vorbis, JPEG, PNG, zlib, HarfBuzz, SheenBidi 전문을 LICENSES에 보존했습니다. 전체 JUCE dependency 목록은 JUCE-LICENSE.md와 pinned source의 원래 고지를 참조하세요. 외부 framework의 copyright·고지를 이 프로젝트 license로 대체하지 않습니다.

이 공개 source에는 기존 icon·screenshot·사용자 녹음·음악 파일이 포함되어 있지 않습니다. UI는 native text/vector로 그립니다. 테스트 WAV는 별도 build directory의 수학적 test tone이며 제품 음악이나 청감 품질의 증거가 아닙니다.

후속 binary 배포 시 AGPL §6에 맞는 Corresponding Source와 build/install scripts 제공을 별도로 확인해야 합니다. 이 repository 준비는 signed/notarized binary release를 뜻하지 않습니다.
