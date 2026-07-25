// SPDX-License-Identifier: GPL-3.0-only
#include "TowerCraftManifestSyncTask.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include "Application.h"
#include "BuildConfig.h"
#include "Exception.h"
#include "FileSystem.h"
#include "Json.h"
#include "net/ApiDownload.h"

TowerCraftManifestSyncTask::TowerCraftManifestSyncTask(QString instanceMinecraftRoot, QObject* parent)
    : Task(parent), m_instanceRoot(std::move(instanceMinecraftRoot))
{}

void TowerCraftManifestSyncTask::executeTask()
{
    if (BuildConfig.TOWERCRAFT_MANIFEST_URL.isEmpty()) {
        qDebug() << "[TowerCraft] No manifest URL configured, skipping mod file sync.";
        emitSucceeded();
        return;
    }

    setStatus(tr("Verification du manifest TowerCraft..."));
    auto [job, response] = Net::ApiDownload::makeByteArray(QUrl(BuildConfig.TOWERCRAFT_MANIFEST_URL));
    m_manifestJob = makeShared<NetJob>(tr("TowerCraft manifest"), APPLICATION->network());
    m_manifestJob->addNetAction(job);
    connect(m_manifestJob.get(), &NetJob::succeeded, this, [this, response] {
        m_manifestData = *response;
        onManifestDownloaded();
    });
    connect(m_manifestJob.get(), &NetJob::failed, this,
            [this](const QString& reason) { emitFailed(tr("Impossible de recuperer manifest.json : %1").arg(reason)); });
    connect(m_manifestJob.get(), &NetJob::progress, this, &TowerCraftManifestSyncTask::setProgress);
    m_manifestJob->start();
}

QList<TowerCraftManifestEntry> TowerCraftManifestSyncTask::parseManifest(const QByteArray& data)
{
    QList<TowerCraftManifestEntry> entries;
    auto doc = Json::requireDocument(data, "manifest.json");
    auto root = Json::requireObject(doc, "manifest.json");
    auto files = Json::requireArray(root, "files");
    for (const auto& fileValue : files) {
        auto fileObj = Json::requireIsType<QJsonObject>(fileValue, "manifest.json file entry");
        TowerCraftManifestEntry entry;
        entry.name = Json::requireString(fileObj, "name");
        entry.size = Json::requireInteger(fileObj, "size");
        entry.sha256 = Json::requireString(fileObj, "sha256").toLower();
        entry.url = Json::requireString(fileObj, "url");
        entries.append(entry);
    }
    return entries;
}

QString TowerCraftManifestSyncTask::localPath(const TowerCraftManifestEntry& entry) const
{
    return FS::PathCombine(m_instanceRoot, entry.name);
}

bool TowerCraftManifestSyncTask::isUpToDate(const TowerCraftManifestEntry& entry) const
{
    QFileInfo info(localPath(entry));
    if (!info.exists() || info.size() != entry.size) {
        return false;
    }
    QFile file(info.absoluteFilePath());
    if (!file.open(QFile::ReadOnly)) {
        return false;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        return false;
    }
    return QString::fromLatin1(hash.result().toHex()).compare(entry.sha256, Qt::CaseInsensitive) == 0;
}

void TowerCraftManifestSyncTask::onManifestDownloaded()
{
    try {
        m_entries = parseManifest(m_manifestData);
    } catch (const Exception& e) {
        emitFailed(tr("manifest.json invalide : %1").arg(e.what()));
        return;
    }

    m_toDownload.clear();
    for (const auto& entry : m_entries) {
        if (!isUpToDate(entry)) {
            m_toDownload.append(entry);
        }
    }

    if (m_toDownload.isEmpty()) {
        setStatus(tr("Fichiers TowerCraft a jour."));
        pruneRemovedFiles();
        saveLastManifest();
        emitSucceeded();
        return;
    }

    startFileDownloads();
}

void TowerCraftManifestSyncTask::startFileDownloads()
{
    setStatus(tr("Telechargement de %1 fichier(s) TowerCraft...").arg(m_toDownload.size()));
    m_filesJob = makeShared<NetJob>(tr("TowerCraft files"), APPLICATION->network());
    for (const auto& entry : m_toDownload) {
        QDir().mkpath(QFileInfo(localPath(entry)).absolutePath());
        m_filesJob->addNetAction(Net::ApiDownload::makeFile(QUrl(entry.url), localPath(entry)));
    }
    connect(m_filesJob.get(), &NetJob::succeeded, this, &TowerCraftManifestSyncTask::onFilesDownloaded);
    connect(m_filesJob.get(), &NetJob::failed, this,
            [this](const QString& reason) { emitFailed(tr("Echec du telechargement des fichiers TowerCraft : %1").arg(reason)); });
    connect(m_filesJob.get(), &NetJob::progress, this, &TowerCraftManifestSyncTask::setProgress);
    connect(m_filesJob.get(), &NetJob::status, this, &TowerCraftManifestSyncTask::setStatus);
    m_filesJob->start();
}

void TowerCraftManifestSyncTask::onFilesDownloaded()
{
    for (const auto& entry : m_toDownload) {
        if (!isUpToDate(entry)) {
            emitFailed(tr("Le fichier %1 telecharge ne correspond pas au manifest (SHA-256 different).").arg(entry.name));
            return;
        }
    }
    pruneRemovedFiles();
    saveLastManifest();
    emitSucceeded();
}

void TowerCraftManifestSyncTask::pruneRemovedFiles()
{
    auto previous = loadLastManifest();
    if (previous.isEmpty()) {
        return;
    }
    QSet<QString> currentNames;
    for (const auto& entry : m_entries) {
        currentNames.insert(entry.name);
    }
    for (const auto& oldEntry : previous) {
        if (!currentNames.contains(oldEntry.name)) {
            QFile::remove(localPath(oldEntry));
            qDebug() << "[TowerCraft] Removed file no longer in manifest:" << oldEntry.name;
        }
    }
}

QString TowerCraftManifestSyncTask::lastManifestPath() const
{
    return FS::PathCombine(m_instanceRoot, ".towercraft", "last_manifest.json");
}

QList<TowerCraftManifestEntry> TowerCraftManifestSyncTask::loadLastManifest() const
{
    QFile file(lastManifestPath());
    if (!file.open(QFile::ReadOnly)) {
        return {};
    }
    try {
        return parseManifest(file.readAll());
    } catch (const Exception&) {
        return {};
    }
}

void TowerCraftManifestSyncTask::saveLastManifest() const
{
    QDir().mkpath(QFileInfo(lastManifestPath()).absolutePath());
    QFile file(lastManifestPath());
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        file.write(m_manifestData);
    }
}
