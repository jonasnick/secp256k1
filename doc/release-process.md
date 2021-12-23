# Release Process

1. Open PR to master that
   * adds release notes to `CHANGELOG.md`, and
   * if this is **not** a patch release, updates `_PKG_VERSION_{MAJOR,MINOR}` and `_LIB_VERSION_*` in `configure.ac`.
2. After the PR is merged,
   * if this is **not** a patch release, create a release branch with name `MAJOR.MINOR`.
     Make sure that the branch contains the right commits.
     Create commit on the release branch that sets `_PKG_VERSION_PATCH` to `0` and `_PKG_VERSION_IS_RELEASE` in `configure.ac` to `true` .
   * if this **is** a patch release, open a pull request to the `MAJOR.MINOR` branch that
     * includes the bugfixes,
     * includes the release note commit from master,
     * bumps `_PKG_VERSION_BUILD` and updates `_LIB_VERSION_*` in `configure.ac`.
3. Tag the commit with `git tag -s vMAJOR.MINOR.PATCH`.
4. Push branch and tag with `git push git@github.com:bitcoin-core/secp256k1.git vMAJOR.MINOR.PATCH`.
5. Create a new GitHub release with a link to the corresponding entry in `CHANGELOG.md`.

Note that adding a release entry to `CHANGELOG.md` in step 1 requires a release date that should match the date of steps 3 to 5.
