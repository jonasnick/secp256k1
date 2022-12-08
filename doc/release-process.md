# Release Process

This document outlines the process for releasing versions of the form `$MAJOR.$MINOR.$PATCH`.

We distinguish between two types of releases: *regular* and *maintenance* releases.
Regular releases are releases of a new major or minor version as well as patches of the most recent release.
Maintenance releases, on the other hand, are required for patches of older releases.

Note that adding a release entry to CHANGELOG.md requires a release date. This date should match the dates of the remaining steps in the release process, including the date of the tag and the GitHub release.

This process also assumes that there will be no minor releases for old major releases.

## Regular release

1. Open PR to the master branch that includes the following commits:
   1. A commit (with commit message `"release: prepare for $MAJOR.$MINOR.$PATCH"`, for example) that
       * adds release notes to [CHANGELOG.md](../CHANGELOG.md) and
       * updates `_PKG_VERSION_*`, `_LIB_VERSION_*`, and sets `_PKG_VERSION_IS_RELEASE` to `true` in `configure.ac`.
   2. A commit (with commit message `"release: update master after $MAJOR.$MINOR.$PATCH"`, for example) that sets `_PKG_VERSION_IS_RELEASE` to `false` and `_PKG_VERSION_PATCH` to `$PATCH + 1` and increases `_LIB_VERSION_REVISION`.
2. After the PR is merged, tag the commit:
   ```
   BRANCH_COMMIT=<commit of step 1.1>
   git tag -s v$MAJOR.$MINOR.$PATCH -m "libsecp256k1 $MAJOR.$MINOR.$PATCH" $BRANCH_COMMIT
   ```
3. Push tag:
   ```
   git push git@github.com:bitcoin-core/secp256k1.git v$MAJOR.$MINOR.$PATCH
   ```
4. Create a new GitHub release with a link to the corresponding entry in [CHANGELOG.md](../CHANGELOG.md).

## Maintenance release

Note that some releases do not require maintenance, even if they include bugs.
This is the case when users of the library are able to update to a newer, compatible release that does not include the bug.

1. Open PR to the master branch that includes a commit (with commit message `"release: prepare for $MAJOR.$MINOR.$PATCH"`, for example) that adds release notes to [CHANGELOG.md](../CHANGELOG.md).
2. If `$PATCH = 1`, create maintenance branch `$MAJOR.$MINOR`:
     ```
     BRANCH_COMMIT=<id of commit that set _PKG_VERSION_* to $MAJOR.$MINOR>
     git checkout -b $MAJOR.$MINOR $BRANCH_COMMIT
     ```
3. Open a pull request to the `$MAJOR.$MINOR` branch that
    * includes the bugfixes,
    * includes the release note commit from master,
    * bumps `_PKG_VERSION_PATCH` and `_LIB_VERSION_REVISION` in `configure.ac` (with commit message `"release: update PKG_ and LIB_VERSION for $MAJOR.$MINOR.$PATCH"`, for example).
4. After the PRs are merged, update the release branch and tag the commit:
   ```
   git checkout $MAJOR.$MINOR && git pull
   git tag -s v$MAJOR.$MINOR.$PATCH -m "libsecp256k1 $MAJOR.$MINOR.$PATCH"
   ```
5. Push branch and tag:
   ```
   git push git@github.com:bitcoin-core/secp256k1.git v$MAJOR.$MINOR.$PATCH $MAJOR.$MINOR
   ```
6. Create a new GitHub release with a link to the corresponding entry in [CHANGELOG.md](../CHANGELOG.md).
