# Release Process

To create release `$MAJOR.$MINOR.$PATCH` proceed as follows:

1. Open PR to master that
   * adds release notes to [CHANGELOG.md](../CHANGELOG.md), and
   * if this is **not** a patch release, updates `_PKG_VERSION_{MAJOR,MINOR}` and `_LIB_VERSION_*` in `configure.ac` (with commit message `"release: prepare for $MAJOR.$MINOR.$PATCH"`, for example).
2. After the PR is merged,
   * if this is **not** a patch release, create a release branch with name `$MAJOR.$MINOR`.
     ```
     git checkout master && git pull && git checkout -b $MAJOR.$MINOR
     ```
     * Make sure that the branch contains the right commits.
     * Create commit on the release branch that sets `_PKG_VERSION_PATCH` to `0` and `_PKG_VERSION_IS_RELEASE` in `configure.ac` to `true` (with commit message `"release: update PKG_VERSION for $MAJOR.$MINOR.$PATCH"`, for example).
   * if this **is** a patch release, open a pull request to the `$MAJOR.$MINOR` branch that
     * includes the bugfixes,
     * includes the release note commit from master,
     * bumps `_PKG_VERSION_PATCH` and updates `_LIB_VERSION_*` in `configure.ac` (with commit message `"release: update PKG_ and LIB_VERSION for $MAJOR.$MINOR.$PATCH"`, for example).
3. Update the release branch and tag the commit:
   ```
   git checkout $MAJOR.$MINOR && git pull # only required for patch releases
   git tag -s v$MAJOR.$MINOR.$PATCH -m "libsecp256k1 $MAJOR.$MINOR.$PATCH"
   ```
4. Push branch and tag:
   ```
   git push git@github.com:bitcoin-core/secp256k1.git v$MAJOR.$MINOR.$PATCH $MAJOR.$MINOR
   ```
5. Create a new GitHub release with a link to the corresponding entry in [CHANGELOG.md](../CHANGELOG.md).

Note that adding a release entry to [CHANGELOG.md](../CHANGELOG.md) in step 1 requires a release date that should match the date of steps 3 to 5.
