# Release Process

To create release `$MAJOR.$MINOR.$PATCH` proceed as follows:

1. Open PR to the master branch that includes the following commits:
   1. A commit (with commit message `"release: prepare for $MAJOR.$MINOR.$PATCH"`, for example) that
       * adds release notes to [CHANGELOG.md](../CHANGELOG.md) and
       * if this is **not** a patch release, updates `_PKG_VERSION_{MAJOR,MINOR}`, `_LIB_VERSION_*`, and sets `_PKG_VERSION_IS_RELEASE` to `true` in `configure.ac`.
   2. A commit that
       * if this is **not** a patch release, sets `_PKG_VERSION_IS_RELEASE` to `false`.
       * if this is **not** a patch release for a former major or minor version (i.e. if `_PKG_VERSION_MAJOR._PKG_VERSION_MINOR` is equal to `$MAJOR.$MINOR`), sets `_PKG_VERSION_PATCH` to `$PATCH + 1` and increases `_LIB_VERSION_REVISION` and,
     If none of the two conditions apply, the commit is ommitted.
2. After the PR is merged,
   * if this is **not** a patch release, create a release branch with name `$MAJOR.$MINOR`:
     ```
     BRANCH_COMMIT=<commit of step 1.1>
     git checkout -b $MAJOR.$MINOR $BRANCH_COMMIT
     ```
     * Make sure that the branch contains the right commits.
   * if this **is** a patch release, open a pull request to the `$MAJOR.$MINOR` branch that
     * includes the bugfixes,
     * includes the release note commit from master,
     * bumps `_PKG_VERSION_PATCH` and `_LIB_VERSION_REVISION` in `configure.ac` (with commit message `"release: update PKG_ and LIB_VERSION for $MAJOR.$MINOR.$PATCH"`, for example).
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
