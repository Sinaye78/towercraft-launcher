// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QPixmap>
#include <QWidget>

#include "QObjectPtr.h"

class QLabel;
class QProgressBar;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class BaseInstance;
class ServerPingTask;
class TowerCraftEnsureInstanceTask;
class TowerCraftManifestSyncTask;

/** The TowerCraft single-instance home screen: replaces PrismLauncher's instance grid as the
 *  central widget. Shows a big background, server status/player count, the current version, a
 *  news panel, an inline progress bar + download log for the running launch task, and one big
 *  Play/Kill button - there is no instance picker, since this fork only ever manages one instance
 *  (see AGENTS.md-equivalent decision in DECISIONS.md: multi-instance UI removed in Phase 1). */
class TowerCraftDashboardWidget : public QWidget {
    Q_OBJECT

   public:
    explicit TowerCraftDashboardWidget(QWidget* parent = nullptr);
    ~TowerCraftDashboardWidget() override;

    /** Re-reads instance count/state (call after instances()->count() or isRunning() may have
     *  changed - instance creation, launch, kill, exit). */
    void refreshInstanceState();

   protected:
    void paintEvent(QPaintEvent* event) override;

   private slots:
    void onPlayClicked();
    void onKillClicked();
    void refreshServerStatus();

   private:
    BaseInstance* currentInstance() const;
    void appendLog(const QString& line);
    void hookLaunchSignals();
    void setPlayingState(bool running);
    void startPreparation();
    void runManifestSyncThenLaunch();

    QPixmap m_background;

    QLabel* m_versionLabel = nullptr;
    QLabel* m_statusDot = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_newsLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPlainTextEdit* m_logView = nullptr;
    QPushButton* m_playButton = nullptr;

    QTimer* m_statusTimer = nullptr;
    shared_qobject_ptr<ServerPingTask> m_pingTask;

    bool m_preparingLaunch = false;
    shared_qobject_ptr<TowerCraftEnsureInstanceTask> m_ensureInstanceTask;
    shared_qobject_ptr<TowerCraftManifestSyncTask> m_manifestSyncTask;
};
