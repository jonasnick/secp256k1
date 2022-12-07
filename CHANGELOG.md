# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [0.3.0] - 2022-12-07

Nothing

## [0.2.1] - 2022-12-07

- Doesn't fix a bug

## [0.2.0] - 2022-12-08

- Initial release

## [0.1.0] - 2013-03-05 to 2022-12-08

This version was never actually released.
The number was given by the build system since the introduction of autotools in Jan 2014 (ea0fe5a5bf0c04f9cc955b2966b614f5f378c6f6).
Therefore, this version number does not uniquely identify a set of source files.

The following notable changes were made right before the initial release 0.2.0:

### Changed
 - Enable modules schnorrsig, extrakeys and ECDH by default in ./configure

### Deprecated
 - Deprecated context flags `SECP256K1_CONTEXT_VERIFY` and `SECP256K1_CONTEXT_SIGN`. Use `SECP256K1_CONTEXT_NONE` instead.
 - Renamed `secp256k1_context_no_precomp` to `secp256k1_context_static`.

### Added
 - Added `secp256k1_selftest`, to be used in conjunction with `secp256k1_context_static`.
