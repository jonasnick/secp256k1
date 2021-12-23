# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [0.2.0] - 2022-12-08

### Changed
 - Enable modules schnorrsig, extrakeys and ECDH by default in ./configure

### Deprecated
 - Deprecated context flags `SECP256K1_CONTEXT_VERIFY` and `SECP256K1_CONTEXT_SIGN`. Use `SECP256K1_CONTEXT_NONE` instead.
 - Renamed `secp256k1_context_no_precomp` to `secp256k1_context_static`.

### Added
 - Added `secp256k1_selftest`, to be used in conjunction with `secp256k1_context_static`.

## [0.1.0] - 2013-03-05 to 2021-12-25

This version was never actually released.
The number was given by the build system since the introduction of autotools in Jan 2014 (ea0fe5a5bf0c04f9cc955b2966b614f5f378c6f6).
Therefore, this version number does not uniquely identify a set of source files.

