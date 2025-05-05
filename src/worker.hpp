#pragma once
#include <QObject>
#include <QString>
#include <nlohmann/json.hpp>

class ApiWorker : public QObject {
    Q_OBJECT
public:
    explicit ApiWorker(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void checkLiveStatus(const QString &webRid);

signals:
    void liveStatusChecked(bool isLive, const QString message);

private:
    bool isLiveRoomStarted(const QString &webRid);
    QString getTtwid();
    QString randomUserAgent();
};
