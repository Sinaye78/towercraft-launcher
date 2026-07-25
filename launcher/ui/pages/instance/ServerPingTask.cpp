#include <QFutureWatcher>

#include <Json.h>
#include "Exception.h"
#include "McClient.h"
#include "McResolver.h"
#include "ServerPingTask.h"

unsigned getOnlinePlayers(QJsonObject data)
{
    try {
        return Json::requireInteger(Json::requireObject(data, "players"), "online");
    } catch (Exception& e) {
        qWarning() << "server ping failed to parse response" << e.what();
        return 0;
    }
}

int getMaxPlayers(QJsonObject data)
{
    try {
        return Json::requireInteger(Json::requireObject(data, "players"), "max");
    } catch (Exception& e) {
        qWarning() << "server ping failed to parse max players" << e.what();
        return -1;
    }
}

QString getVersionName(QJsonObject data)
{
    try {
        return Json::requireString(Json::requireObject(data, "version"), "name");
    } catch (Exception& e) {
        qWarning() << "server ping failed to parse version" << e.what();
        return {};
    }
}

// The status document's "description" (MOTD) is either a plain string or a chat-component object
// with a "text" field, depending on server implementation - handle both rather than assuming one.
QString getMotd(QJsonObject data)
{
    auto value = data.value("description");
    if (value.isString()) {
        return value.toString();
    }
    if (value.isObject()) {
        return value.toObject().value("text").toString();
    }
    return {};
}

void ServerPingTask::executeTask()
{
    qDebug() << "Querying status of" << QString("%1:%2").arg(m_domain).arg(m_port);

    // Resolve the actual IP and port for the server
    McResolver* resolver = new McResolver(nullptr, m_domain, m_port);
    connect(resolver, &McResolver::succeeded, this, [this](QString ip, int port) {
        qDebug().nospace().noquote() << "Resolved address for " << m_domain << ": " << ip << ":" << port;

        // Now that we have the IP and port, query the server
        McClient* client = new McClient(nullptr, m_domain, ip, port);

        connect(client, &McClient::succeeded, this, [this](QJsonObject data) {
            m_outputOnlinePlayers = getOnlinePlayers(data);
            m_outputMaxPlayers = getMaxPlayers(data);
            m_outputVersionName = getVersionName(data);
            m_outputMotd = getMotd(data);
            qDebug() << "Online players:" << m_outputOnlinePlayers << "/" << m_outputMaxPlayers << "version:" << m_outputVersionName;
            emitSucceeded();
        });
        connect(client, &McClient::failed, this, [this](QString error) { emitFailed(error); });

        // Delete McClient object when done
        connect(client, &McClient::finished, this, [client]() { client->deleteLater(); });
        client->getStatusData();
    });
    connect(resolver, &McResolver::failed, this, [this](QString error) { emitFailed(error); });

    // Delete McResolver object when done
    connect(resolver, &McResolver::finished, [resolver]() { resolver->deleteLater(); });
    resolver->ping();
}
