#include "detector.hpp"
#include <QVBoxLayout>
// #include <curl/curl.h>
// #include <nlohmann/json.hpp>
// #include <random>
#include "obs-websocket-api.h"
#include "worker.hpp"
#include <QThread>
#include <obs-module.h>
#include <QSettings>

Detector::Detector(QWidget *parent) : QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	startButton = new QPushButton("Start", this);
	stopButton = new QPushButton("Stop", this);
	resultLabel = new QLabel("", this);
	webRidInput = new QLineEdit(this);
	webRidInput->setPlaceholderText("123456789012");
	QString savedText = Detector::getString("webRid");
	webRidInput->setText(savedText);
	QLabel *webRidLabel =
		new QLabel("https://live.douyin.com/123456789012\n电脑网页打开的后11,12位左右的数字", this);

	timer = new QTimer(this);

	layout->addWidget(webRidInput);
	layout->addWidget(webRidLabel);
	layout->addWidget(startButton);
	layout->addWidget(stopButton);
	layout->addWidget(resultLabel);

	stopButton->hide(); // initially hide stop

	// When start is clicked
	connect(startButton, &QPushButton::clicked, this, &Detector::StartButtonClicked);

	// When stop is clicked
	connect(stopButton, &QPushButton::clicked, this, &Detector::StopButtonClicked);

	connect(webRidInput, &QLineEdit::editingFinished, this, [=]() {
		QString text = webRidInput->text();
		Detector::saveString("webRid", text);
	});
}

Detector::~Detector()
{
	StopButtonClicked();
}

void Detector::PostLoad()
{
	vendor = obs_websocket_register_vendor("detector");
	if (!vendor)
		return;
}

void Detector::StartButtonClicked()
{
	callCount = 0;
	webRidInput->setReadOnly(true);
	startButton->hide();
	resultLabel->setText("正在检测...");
	if (!workerThread) {
		workerThread = new QThread(this);
		worker = new ApiWorker();
		worker->moveToThread(workerThread);

		connect(workerThread, &QThread::started, worker, &ApiWorker::initialize);
		connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
		connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
		connect(this, &Detector::requestLiveStatusCheck, worker, &ApiWorker::checkLiveStatus,
			Qt::QueuedConnection);
		connect(worker, &ApiWorker::liveStatusChecked, this, &Detector::HandleLiveStatusResult,
			Qt::QueuedConnection);

		connect(workerThread, &QThread::finished, this, [this]() {
			if (workerThread) {
				workerThread->deleteLater();
				workerThread = nullptr;
			}
		});

		workerThread->start();
	}
	FetchApi();          // immediately call once
	timer->start(20000); // 20 seconds
	stopButton->show();
	// Timer logic
	connect(timer, &QTimer::timeout, this, &Detector::FetchApi, Qt::UniqueConnection);
}

void Detector::StopButtonClicked()
{
	m_isStopping = true;
	timer->stop();
	stopButton->hide();

	if (workerThread) {
		disconnect(workerThread, nullptr, nullptr, nullptr); // optional safety

		// Ensure cleanup happens after the thread has actually exited
		connect(workerThread, &QThread::finished, this, [this]() {
			workerThread->deleteLater();
			workerThread = nullptr;
			worker = nullptr;

			resultLabel->setText("");
			webRidInput->setReadOnly(false);
			startButton->show();
			m_isStopping = false;
		});

		workerThread->quit();
	} else {
		// If no thread, reset UI immediately
		resultLabel->setText("");
		webRidInput->setReadOnly(false);
		startButton->show();
		m_isStopping = false;
	}
}

void Detector::refreshBrowserSource(const char *source_name)
{
	obs_source_t *source = obs_get_source_by_name(source_name);
	if (!source)
		return;
	// Get the properties view if it exists
	obs_properties_t *props = obs_source_properties(source);
	obs_property_t *refresh_prop = obs_properties_get(props, "refreshnocache");

	if (refresh_prop && obs_property_get_type(refresh_prop) == OBS_PROPERTY_BUTTON) {
		// Trigger the button click callback
		obs_property_button_clicked(refresh_prop, source);
	}

	obs_properties_destroy(props);
	obs_source_release(source);
}

void Detector::HandleLiveStatusResult(std::optional<bool> isLive, const QString &user)
{
	if (m_isStopping || !isLive.has_value())
		return;
	bool liveStatus = isLive.value();
	QString statusStr = liveStatus ? "直播中" : "空闲";
	resultLabel->setText(user + ": " + statusStr + " (" + QString::number(callCount++) + ")");

	if (liveStatus) {
		if (!obs_frontend_recording_active()) {
			refreshBrowserSource("Browser");
			QTimer::singleShot(3000, this, []() {
				if (!obs_frontend_recording_active()) {
					obs_frontend_recording_start();
				}
			});
		}
	} else {
		if (obs_frontend_recording_active()) {
			obs_frontend_recording_stop();
			QTimer::singleShot(3000, this, [this]() { refreshBrowserSource("Browser"); });
		}
	}
}

void Detector::FetchApi()
{
	if (worker && !m_isStopping) {
		emit requestLiveStatusCheck(getWebRid());
	}
	// emit startCheckLiveStatus(getWebRid()); // Triggers background thread
}

QString Detector::getWebRid() const
{
	return webRidInput->text();
}

void Detector::saveString(const QString &key, const QString &value)
{
	QSettings settings("obsproject", "obs-studio");
	settings.setValue("detector/" + key, value);
}

QString Detector::getString(const QString &key)
{
	QSettings settings("obsproject", "obs-studio");
	return settings.value("detector/" + key, "").toString();
}
