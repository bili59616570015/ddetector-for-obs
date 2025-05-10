#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QString>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QThread>
#include <worker.hpp>
#include <QMutex>
#include <QAtomicInt>
#include <QDateTime>

class Detector : public QWidget {
	Q_OBJECT
public:
	explicit Detector(QWidget *parent = nullptr);
	~Detector();
signals:
	void requestLiveStatusCheck(const QString &webRid); // Changed signal

private slots:
	void StartButtonClicked();
	void StopButtonClicked();
	void HandleLiveStatusResult(bool isLive, const QString &user);

private:
	void FetchApi();
	void refreshBrowserSource(const char *source_name);
	QString getWebRid() const;
	QString getString(const QString &key);
	void saveString(const QString &key, const QString &value);

	void *vendor;
	QPushButton *startButton;
	QPushButton *stopButton;
	QLabel *resultLabel;
	QLineEdit *webRidInput;
	QTimer *timer;
	QThread *workerThread = nullptr;
	ApiWorker *worker = nullptr;
	int callCount = 0;
	std::atomic<bool> m_isStopping = false;

public:
	void PostLoad();
};
