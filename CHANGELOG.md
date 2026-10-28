# 변경 기록

## 1.0.0 — 2026-10-28

- 별도 source edition 제품명·bundle/plugin identity, 자동 설치 차단
- JUCE 8.0.16-security immutable pin과 AGPLv3/third-party 고지
- Mix 0/Gate 원음 보존, finite·sample rate·reset·state 검증과 cached atomic parameter
- audio-thread UI callback 대신 message-thread polling, 실제 control이 모두 보이는 native vector editor
- 기존 smoke 3개 및 실제 processor/WAV/allocator 회귀, 장치 없는 editor test container

- Host numeric locale에 의존하던 state parser를 stream 전용 classic locale로 변경하고, C/comma subprocess 실제 processor 회귀와 invalid-state 보존 검사를 추가
