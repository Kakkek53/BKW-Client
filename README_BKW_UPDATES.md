# BKW test updates

Update repository: https://github.com/Kakkek53/test

The **release title**, not its tag, must be exactly a channel prefix and numeric
version: `b0.3`, `B0.4`, `r0.1`, `R0.2`. Beta and Release are separate channels.
Only one channel can be selected in BKW → Основное → Обновления.
Within a channel, equal and older versions are ignored. Switching channels
explicitly allows a different numbering sequence, including b0.3 → r0.1.

The client first requests `/releases/latest`, then `/releases?per_page=100`.
GitHub excludes prereleases from latest, so the list is needed for Beta and for
finding the selected channel when latest belongs to the other channel.
Drafts, malformed titles and releases without a compatible download are skipped.
The highest matching version among the latest 100 releases is selected.

## Testing the two builds

1. Extract and run the complete **b0.3** Windows ZIP (not just DDNet.exe).
2. In the test repository, publish a release titled **r0.1**.
3. Attach the **inner** `bkw-r0.1-win64.zip` package from the r0.1 Actions artifact.
   Do not upload the outer Actions artifact ZIP or GitHub's source-code archive.
4. In b0.3 select **Только Release**, then **Проверить**, **Скачать**, **Установить**.
5. The restarted client should show Release 0.1 and the redesigned Main page.
   A second check must not offer the same release again.

For Linux attach `bkw-r0.1-linux_x86_64.tar.xz`. The full installed package must
contain `bestclient-updater` (or `bestclient-updater.exe` on Windows) and be writable.
Downloads are staged in the installation's absolute `update/` directory.
Only HTTPS release-asset links belonging to Kakkek53/test are accepted.
Checking is automatic at startup. Automatic downloading is optional;
installation/restart always needs a user click.

macOS uses manual installation. Android installation requires both APKs to use
the **same signing key**; the inherited Fast Build workflow generates a fresh
CI signing key on each run, so those APKs cannot test an in-place upgrade.
Do not uninstall the old Android app to work around signing: that can lose data.

The test repository's existing placeholder release is not modified or removed.
