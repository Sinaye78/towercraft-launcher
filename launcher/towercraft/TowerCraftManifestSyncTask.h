// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QList>
#include <QString>

#include "QObjectPtr.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

struct TowerCraftManifestEntry {
    QString name;  // path relative to the instance's .minecraft root, e.g. "mods/towercraft-0.2.0.jar"
    qint64 size = 0;
    QString sha256;
    QString url;
};

/** Downloads TowerCraft's own manifest.json (name/size/sha256/url per file, hosted on GitHub - see
 *  DECISIONS.md) and brings the instance's files in line with it: downloads anything missing or
 *  changed (verified by size+sha256, not just filename), then deletes any previously-synced file
 *  that's no longer listed. No-ops cleanly (emits success immediately) when
 *  BuildConfig.TOWERCRAFT_MANIFEST_URL is blank, since no real manifest repo exists yet.
 *
 *  Tracks "previously synced" via a local copy of the last manifest that was fully applied
 *  (<instance>/.towercraft/last_manifest.json) - diffing against that, not against whatever
 *  happens to be on disk, is what lets a file removed from the manifest actually get deleted
 *  instead of orphaned forever. */
class TowerCraftManifestSyncTask : public Task {
    Q_OBJECT

   public:
    explicit TowerCraftManifestSyncTask(QString instanceMinecraftRoot, QObject* parent = nullptr);

   protected:
    void executeTask() override;

   private:
    void onManifestDownloaded();
    void startFileDownloads();
    void onFilesDownloaded();
    void pruneRemovedFiles();

    static QList<TowerCraftManifestEntry> parseManifest(const QByteArray& data);
    bool isUpToDate(const TowerCraftManifestEntry& entry) const;
    QString localPath(const TowerCraftManifestEntry& entry) const;
    QString lastManifestPath() const;
    QList<TowerCraftManifestEntry> loadLastManifest() const;
    void saveLastManifest() const;

    QString m_instanceRoot;
    QByteArray m_manifestData;
    shared_qobject_ptr<NetJob> m_manifestJob;
    shared_qobject_ptr<NetJob> m_filesJob;
    QList<TowerCraftManifestEntry> m_entries;
    QList<TowerCraftManifestEntry> m_toDownload;
};
