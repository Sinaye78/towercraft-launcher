// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "BaseVersion.h"
#include "QObjectPtr.h"
#include "tasks/Task.h"

/** Silently creates the single TowerCraft instance (pinned Minecraft + Fabric Loader version, see
 *  BuildConfig.TOWERCRAFT_MINECRAFT_VERSION/TOWERCRAFT_FABRIC_LOADER_VERSION) the first time the
 *  player clicks Play and no instance exists yet - no New Instance wizard, no version picker. This
 *  fork only ever manages one instance (see Phase 1 decisions in DECISIONS.md), so instance
 *  creation itself must be as invisible to the player as everything else here.
 *
 *  Succeeds immediately (no-op) if an instance already exists - this task is meant to be run
 *  unconditionally before every launch attempt, same spirit as TowerCraftManifestSyncTask. */
class TowerCraftEnsureInstanceTask : public Task {
    Q_OBJECT

   public:
    explicit TowerCraftEnsureInstanceTask(QObject* parent = nullptr);

   protected:
    void executeTask() override;

   private:
    void loadMinecraftVersion();
    void loadFabricVersion();
    void createInstance();

    Task::Ptr m_currentTask;
    unique_qobject_ptr<Task> m_stagingTask;
    BaseVersion::Ptr m_minecraftVersion;
    BaseVersion::Ptr m_fabricVersion;
};
