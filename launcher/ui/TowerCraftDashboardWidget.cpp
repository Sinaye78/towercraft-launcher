// SPDX-License-Identifier: GPL-3.0-only
#include "TowerCraftDashboardWidget.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "Application.h"
#include "BaseInstance.h"
#include "InstanceList.h"
#include "LaunchController.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "towercraft/TowerCraftEnsureInstanceTask.h"
#include "towercraft/TowerCraftManifestSyncTask.h"
#include "ui/pages/instance/ServerPingTask.h"

// TowerCraft server address to query for online status/player count (Minecraft Server List Ping).
// TODO: move to a manifest-driven config once Phase 3 (manifest.json sync) lands, rather than a
// literal here.
static const QString TOWERCRAFT_SERVER_HOST = "91.197.6.249";
static const int TOWERCRAFT_SERVER_PORT = 23832;
static const int STATUS_REFRESH_INTERVAL_MS = 60000;

TowerCraftDashboardWidget::TowerCraftDashboardWidget(QWidget* parent) : QWidget(parent)
{
    m_background = QPixmap(":/icons/multimc/scalable/towercraft_dashboard_bg.png");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 16, 24, 24);
    root->setSpacing(12);

    // Top bar: version (left) - status (right)
    auto* topBar = new QHBoxLayout();
    m_versionLabel = new QLabel(tr("TowerCraft"), this);
    m_versionLabel->setStyleSheet("color: #e8e3c8; font-size: 16px; font-weight: bold; background: transparent;");
    topBar->addWidget(m_versionLabel);
    topBar->addStretch();

    m_statusDot = new QLabel(this);
    m_statusDot->setFixedSize(12, 12);
    m_statusDot->setStyleSheet("background-color: #888; border-radius: 6px;");
    m_statusLabel = new QLabel(tr("Statut inconnu"), this);
    m_statusLabel->setStyleSheet("color: #e8e3c8; font-size: 13px; background: transparent;");
    topBar->addWidget(m_statusDot);
    topBar->addWidget(m_statusLabel);
    root->addLayout(topBar);

    root->addStretch();

    // News panel: a semi-opaque box so text stays readable over the background image.
    m_newsLabel = new QLabel(tr("Aucune actualite pour le moment."), this);
    m_newsLabel->setWordWrap(true);
    m_newsLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_newsLabel->setStyleSheet(
        "color: #e8e3c8; font-size: 13px; background-color: rgba(12, 18, 32, 160); "
        "border-radius: 6px; padding: 10px;");
    m_newsLabel->setMaximumWidth(420);
    m_newsLabel->setMinimumHeight(90);
    root->addWidget(m_newsLabel, 0, Qt::AlignLeft);

    // Progress + download log - hidden until a launch is running.
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);
    root->addWidget(m_progressBar);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(500);
    m_logView->setFixedHeight(90);
    m_logView->setVisible(false);
    m_logView->setStyleSheet(
        "background-color: rgba(12, 18, 32, 190); color: #cfd8e6; font-family: monospace; font-size: 11px; "
        "border-radius: 6px;");
    root->addWidget(m_logView);

    // Big Play button.
    m_playButton = new QPushButton(tr("JOUER"), this);
    m_playButton->setMinimumHeight(56);
    m_playButton->setStyleSheet(
        "QPushButton { background-color: #4b6b32; color: #e8e3c8; font-size: 20px; font-weight: bold; "
        "border-radius: 8px; border: 2px solid #0c0a06; } "
        "QPushButton:hover { background-color: #5c803d; } "
        "QPushButton:disabled { background-color: #444; color: #999; }");
    connect(m_playButton, &QPushButton::clicked, this, &TowerCraftDashboardWidget::onPlayClicked);
    root->addWidget(m_playButton);

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &TowerCraftDashboardWidget::refreshServerStatus);
    m_statusTimer->start(STATUS_REFRESH_INTERVAL_MS);
    refreshServerStatus();

    refreshInstanceState();
}

TowerCraftDashboardWidget::~TowerCraftDashboardWidget() = default;

void TowerCraftDashboardWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    if (!m_background.isNull()) {
        painter.drawPixmap(rect(), m_background, m_background.rect());
    }
    QWidget::paintEvent(event);
}

BaseInstance* TowerCraftDashboardWidget::currentInstance() const
{
    auto* instances = APPLICATION->instances();
    if (!instances || instances->count() <= 0) {
        return nullptr;
    }
    return instances->at(0);
}

void TowerCraftDashboardWidget::refreshInstanceState()
{
    auto* instance = currentInstance();
    if (!instance) {
        m_versionLabel->setText(tr("TowerCraft - premier lancement (les fichiers seront telecharges)"));
        setPlayingState(false);
        return;
    }

    QString versionText = tr("TowerCraft");
    if (auto* mcInstance = dynamic_cast<MinecraftInstance*>(instance)) {
        auto version = mcInstance->getPackProfile()->getComponentVersion("net.minecraft");
        if (!version.isEmpty()) {
            versionText = tr("TowerCraft - Minecraft %1").arg(version);
        }
    }
    m_versionLabel->setText(versionText);

    setPlayingState(instance->isRunning());
    if (instance->isRunning()) {
        hookLaunchSignals();
    }
}

void TowerCraftDashboardWidget::setPlayingState(bool running)
{
    m_playButton->setText(running ? tr("EN JEU...") : tr("JOUER"));
    m_playButton->setEnabled(!running && !m_preparingLaunch);
    m_progressBar->setVisible(running);
    // The log stays visible once shown (even after this drops back to "not running" on failure or
    // exit) so the player can still read what happened - only appendLog() reveals it in the first
    // place, never this.
}

void TowerCraftDashboardWidget::onPlayClicked()
{
    if (m_preparingLaunch) {
        return;
    }
    auto* instance = currentInstance();
    if (instance && instance->isRunning()) {
        return;
    }
    startPreparation();
}

void TowerCraftDashboardWidget::startPreparation()
{
    m_preparingLaunch = true;
    m_playButton->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);
    m_logView->setVisible(true);

    if (currentInstance()) {
        runManifestSyncThenLaunch();
        return;
    }

    appendLog(tr("Aucune instance TowerCraft trouvee, creation en cours..."));
    m_ensureInstanceTask = makeShared<TowerCraftEnsureInstanceTask>();
    connect(m_ensureInstanceTask.get(), &Task::status, this, &TowerCraftDashboardWidget::appendLog);
    connect(m_ensureInstanceTask.get(), &Task::succeeded, this, [this] {
        refreshInstanceState();
        runManifestSyncThenLaunch();
    });
    connect(m_ensureInstanceTask.get(), &Task::failed, this, [this](const QString& reason) {
        appendLog(tr("Echec de la creation de l'instance : %1").arg(reason));
        m_preparingLaunch = false;
        setPlayingState(false);
    });
    m_ensureInstanceTask->start();
}

void TowerCraftDashboardWidget::runManifestSyncThenLaunch()
{
    auto* instance = currentInstance();
    if (!instance) {
        appendLog(tr("Aucune instance disponible apres preparation."));
        m_preparingLaunch = false;
        setPlayingState(false);
        return;
    }

    appendLog(tr("Verification des fichiers TowerCraft..."));
    m_manifestSyncTask = makeShared<TowerCraftManifestSyncTask>(instance->gameRoot());
    connect(m_manifestSyncTask.get(), &Task::status, this, &TowerCraftDashboardWidget::appendLog);
    connect(m_manifestSyncTask.get(), &Task::progress, this, [this](qint64 current, qint64 total) {
        if (total > 0) {
            m_progressBar->setRange(0, static_cast<int>(total));
            m_progressBar->setValue(static_cast<int>(current));
        } else {
            m_progressBar->setRange(0, 0);
        }
    });
    connect(m_manifestSyncTask.get(), &Task::succeeded, this, [this] {
        m_preparingLaunch = false;
        auto* inst = currentInstance();
        if (!inst) {
            setPlayingState(false);
            return;
        }
        appendLog(tr("Lancement de %1...").arg(inst->name()));
        APPLICATION->launch(inst);
        setPlayingState(true);
        hookLaunchSignals();
    });
    connect(m_manifestSyncTask.get(), &Task::failed, this, [this](const QString& reason) {
        appendLog(tr("Echec de la synchronisation des fichiers TowerCraft : %1").arg(reason));
        m_preparingLaunch = false;
        setPlayingState(false);
    });
    m_manifestSyncTask->start();
}

void TowerCraftDashboardWidget::onKillClicked()
{
    auto* instance = currentInstance();
    if (instance && instance->isRunning()) {
        APPLICATION->kill(instance);
    }
}

void TowerCraftDashboardWidget::hookLaunchSignals()
{
    auto* instance = currentInstance();
    if (!instance) {
        return;
    }
    auto* controller = APPLICATION->launchController(instance);
    if (!controller) {
        return;
    }
    connect(controller, &Task::status, this, &TowerCraftDashboardWidget::appendLog, Qt::UniqueConnection);
    connect(controller, &Task::details, this, &TowerCraftDashboardWidget::appendLog, Qt::UniqueConnection);
    connect(controller, &Task::progress, this, [this](qint64 current, qint64 total) {
        if (total > 0) {
            m_progressBar->setRange(0, static_cast<int>(total));
            m_progressBar->setValue(static_cast<int>(current));
        } else {
            m_progressBar->setRange(0, 0);
        }
    });
    connect(controller, &Task::finished, this, [this]() {
        appendLog(tr("Session terminee."));
        setPlayingState(false);
    });
}

void TowerCraftDashboardWidget::appendLog(const QString& line)
{
    if (line.isEmpty()) {
        return;
    }
    m_logView->setVisible(true);
    m_logView->appendPlainText(QStringLiteral("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), line));
}

void TowerCraftDashboardWidget::refreshServerStatus()
{
    if (m_pingTask && m_pingTask->isRunning()) {
        return;
    }
    m_pingTask = makeShared<ServerPingTask>(TOWERCRAFT_SERVER_HOST, TOWERCRAFT_SERVER_PORT);
    connect(m_pingTask.get(), &Task::succeeded, this, [this]() {
        m_statusDot->setStyleSheet("background-color: #5c803d; border-radius: 6px;");
        QString text = tr("En ligne");
        if (m_pingTask->m_outputMaxPlayers >= 0) {
            text += tr(" - %1/%2 joueurs").arg(m_pingTask->m_outputOnlinePlayers).arg(m_pingTask->m_outputMaxPlayers);
        }
        m_statusLabel->setText(text);
        if (!m_pingTask->m_outputMotd.isEmpty()) {
            m_statusLabel->setToolTip(m_pingTask->m_outputMotd);
        }
    });
    connect(m_pingTask.get(), &Task::failed, this, [this](const QString&) {
        m_statusDot->setStyleSheet("background-color: #a94442; border-radius: 6px;");
        m_statusLabel->setText(tr("Hors ligne"));
        m_statusLabel->setToolTip({});
    });
    m_pingTask->start();
}
