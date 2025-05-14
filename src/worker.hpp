#pragma once
#include <QObject>
#include <QString>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

class ApiWorker : public QObject {
	Q_OBJECT
public:
	explicit ApiWorker(QObject *parent = nullptr);
	~ApiWorker(); // Add destructor

public slots:
	void initialize();
	void checkLiveStatus(const QString &webRid);

signals:
	void liveStatusChecked(std::optional<bool> isLive, const QString message);

private:
	QString getTtwid();
	QString randomUserAgent();
	QString m_ttwid;
	bool m_initialized = false;
};
