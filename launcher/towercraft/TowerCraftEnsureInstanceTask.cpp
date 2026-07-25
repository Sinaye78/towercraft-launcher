// SPDX-License-Identifier: GPL-3.0-only
#include "TowerCraftEnsureInstanceTask.h"

#include "Application.h"
#include "BuildConfig.h"
#include "InstanceList.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "minecraft/VanillaInstanceCreationTask.h"

TowerCraftEnsureInstanceTask::TowerCraftEnsureInstanceTask(QObject* parent) : Task(parent) {}

void TowerCraftEnsureInstanceTask::executeTask()
{
    if (APPLICATION->instances()->count() > 0) {
        emitSucceeded();
        return;
    }
    setStatus(tr("Preparation de l'instance TowerCraft..."));
    loadMinecraftVersion();
}

void TowerCraftEnsureInstanceTask::loadMinecraftVersion()
{
    setStatus(tr("Recuperation des versions Minecraft disponibles..."));
    auto vlist = APPLICATION->metadataIndex()->get("net.minecraft");
    m_currentTask = vlist->getLoadTask();
    connect(m_currentTask.get(), &Task::succeeded, this, [this, vlist] {
        m_minecraftVersion = vlist->getVersion(BuildConfig.TOWERCRAFT_MINECRAFT_VERSION);
        if (!m_minecraftVersion) {
            emitFailed(tr("Version Minecraft %1 introuvable.").arg(BuildConfig.TOWERCRAFT_MINECRAFT_VERSION));
            return;
        }
        loadFabricVersion();
    });
    connect(m_currentTask.get(), &Task::failed, this, &TowerCraftEnsureInstanceTask::emitFailed);
    connect(m_currentTask.get(), &Task::progress, this, &TowerCraftEnsureInstanceTask::setProgress);
    connect(m_currentTask.get(), &Task::status, this, &TowerCraftEnsureInstanceTask::setStatus);
    if (!m_currentTask->isRunning()) {
        m_currentTask->start();
    }
}

void TowerCraftEnsureInstanceTask::loadFabricVersion()
{
    setStatus(tr("Recuperation des versions Fabric Loader disponibles..."));
    auto vlist = APPLICATION->metadataIndex()->get("net.fabricmc.fabric-loader");
    m_currentTask = vlist->getLoadTask();
    connect(m_currentTask.get(), &Task::succeeded, this, [this, vlist] {
        m_fabricVersion = vlist->getVersion(BuildConfig.TOWERCRAFT_FABRIC_LOADER_VERSION);
        if (!m_fabricVersion) {
            emitFailed(tr("Fabric Loader %1 introuvable.").arg(BuildConfig.TOWERCRAFT_FABRIC_LOADER_VERSION));
            return;
        }
        createInstance();
    });
    connect(m_currentTask.get(), &Task::failed, this, &TowerCraftEnsureInstanceTask::emitFailed);
    connect(m_currentTask.get(), &Task::progress, this, &TowerCraftEnsureInstanceTask::setProgress);
    connect(m_currentTask.get(), &Task::status, this, &TowerCraftEnsureInstanceTask::setStatus);
    if (!m_currentTask->isRunning()) {
        m_currentTask->start();
    }
}

void TowerCraftEnsureInstanceTask::createInstance()
{
    setStatus(tr("Creation de l'instance TowerCraft..."));
    auto* creationTask = new VanillaCreationTask(m_minecraftVersion, "net.fabricmc.fabric-loader", m_fabricVersion);
    creationTask->setName(QStringLiteral("TowerCraft"));
    creationTask->setIcon(QStringLiteral("default"));

    m_stagingTask.reset(APPLICATION->instances()->wrapInstanceTask(creationTask));
    connect(m_stagingTask.get(), &Task::succeeded, this, &TowerCraftEnsureInstanceTask::emitSucceeded);
    connect(m_stagingTask.get(), &Task::failed, this, &TowerCraftEnsureInstanceTask::emitFailed);
    connect(m_stagingTask.get(), &Task::progress, this, &TowerCraftEnsureInstanceTask::setProgress);
    connect(m_stagingTask.get(), &Task::status, this, &TowerCraftEnsureInstanceTask::setStatus);
    m_stagingTask->start();
}
