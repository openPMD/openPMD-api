.. _maintenance-release-github:

New Version on GitHub
=====================

These are the steps we perform to create a new openPMD-api release.

Regular Release
---------------

As a first step, we merge in all pull requests and resolve all issued that we have assigned to the release's "Milestone" on GitHub.
For example, here are all `issues and pull requests <https://github.com/openPMD/openPMD-api/milestone/8>`__ on the version ``0.16.0`` release.

Then, we prepare a pull request that updates the ``CHANGELOG.rst`` and upgrade guide in ``NEWS.rst``, example PR: `Release notes: 0.16 <https://github.com/openPMD/openPMD-api/pull/1648>`__
In the same PR, we bump up the version in CMake files and documentation using our ``new_version.py`` script.

Once the PR is merged, we:

#. Wait for the CI to finish: there will be one more auto-commit added to the ``dev`` branch, updating out ``.pyi`` stub files
#. Pull the latest ``dev`` branch to our local machine
#. Add a GPG-signed tag, e.g., ``git tag -s 0.16.0``: see old releases for the format of this tag, e.g., use ``git show 0.15.0``. The text is from the top of ``CHANGELOG.rst``.
#. Upload the GPG-signed tag to mainline, e.g., ``git push -u mainline 0.16.0``
#. Click `Draft a new release <https://github.com/openPMD/openPMD-api/releases>`__ on GitHub, select the newly created tag.
   Fill the text fields using the same format that you see for `earlier releases <https://github.com/openPMD/openPMD-api/releases>`__, again, based on the top of the text in ``CHANGELOG.rst``.
   Skip the DOI badge for this release step.
#. If you don't have GPG properly set up for your git, then you can just do the last step, which then also creates a tag.
   Be sure to use the same version scheme for the tag, i.e., we do *not* prefix our tags with ``v`` or something of that kind!
#. Go to `Rodare <https://doi.org/10.14278/rodare.27>`__ and wait for the release to arrive.
   If there are issues, contact Rodare/HZDR IT support and check under `Settings - Webhooks <https://github.com/openPMD/openPMD-api/settings/hooks>`__ if the release was delivered.
#. Once the Rodare DOI is auto-created, click on the badge on the right hand side of the page, copy the Markdown code, and edit your `newly created release <https://github.com/openPMD/openPMD-api/releases>`__ text on GitHub to add the badge as you see in earlier releases.

That's it!


Backport (Bugfix) Release
-------------------------

For bugfix releases, we generally follow the same workflow as for regular releases.

The main difference is that we:

#. Start a new branch from the release we want to backport to.
   We name the branch ``release-<version>``, e.g., backports on the ``0.15.0`` release for a ``0.15.1`` release are named ``release-0.15.1``.
#. We add pull requests (usually we ``git cherry-pick`` commits) to that branch until we have all fixes collected.
   Backport releases try to not add features and to not break APIs!
#. Then, we follow the same workflow as above, but we tag on the ``release-<version>`` branch instead of the ``dev`` branch.
#. Once we uploaded the new tag to mainline and created the GitHub release, we remove the ``release-<version>`` branch from our mainline repo.
   The new tag we added contains all history we need if we wanted to do a follow-up bugfix, e.g., a ``release-0.15.2`` branch based on the ``0.15.1`` release.

As general guidance, we usually only fix bugs on the *latest* regular release.
Don't sweat it - if it is too hard to backport a fix, consider doing a timely new regular version release.

That's it!
