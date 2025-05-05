#include "detector.hpp"
#include <QVBoxLayout>
// #include <curl/curl.h>
// #include <nlohmann/json.hpp>
// #include <random>
#include "obs-websocket-api.h"
#include "worker.hpp"
#include <QThread>

Detector::Detector(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    startButton = new QPushButton("Start", this);
    stopButton = new QPushButton("Stop", this);
    resultLabel = new QLabel("", this);
    webRidInput = new QLineEdit(this);
    webRidInput->setPlaceholderText("123456789012");
    QLabel *webRidLabel = new QLabel("https://live.douyin.com/123456789012\n电脑网页打开的后11,12位左右的数字", this);

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

    // Timer logic
    connect(timer, &QTimer::timeout, this, &Detector::FetchApi);

    setupWorkerThread();
}

Detector::~Detector() {
    StopButtonClicked();
}

void Detector::PostLoad() {
	vendor = obs_websocket_register_vendor("detector");
	if (!vendor)
		return;
}

void Detector::StartButtonClicked() {
    callCount = 0;
    webRidInput->setReadOnly(true);
    startButton->hide();
    stopButton->show();
    FetchApi(); // immediately call once
    timer->start(20000); // 20 seconds
}

void Detector::StopButtonClicked() {
    webRidInput->setReadOnly(false);
    timer->stop();
    stopButton->hide();
    startButton->show();
    resultLabel->setText("");
}

// void Detector::FetchApi() {
//     resultLabel->setText("Fetching...");
//     if (isLiveRoomStarted(Detector::getWebRid())) {
//         QMetaObject::invokeMethod(this, [this] {
//             if (!obs_frontend_recording_active()) {
//                 this->refreshBrowserSource("Browser");
//                 obs_frontend_recording_start();
//             }
//         }, Qt::QueuedConnection);
//     } else {
//         QMetaObject::invokeMethod(this, [] {
//             if (obs_frontend_recording_active()) {
//                 obs_frontend_recording_stop();
//             }
//         }, Qt::QueuedConnection);
//     }
// }

void Detector::refreshBrowserSource(const char *source_name) {
    obs_source_t *source = obs_get_source_by_name(source_name);
    if (!source) return;
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

void Detector::setupWorkerThread() {
    QThread *thread = new QThread(this);
    ApiWorker *worker = new ApiWorker();
    worker->moveToThread(thread);

    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Detector::startCheckLiveStatus, worker, &ApiWorker::checkLiveStatus);
    connect(worker, &ApiWorker::liveStatusChecked, this, &Detector::handleLiveStatusResult);

    thread->start();
}

void Detector::handleLiveStatusResult(bool isLive, const QString &user) {
    QString statusStr = isLive ? "直播中" : "空闲";
    resultLabel->setText(user + ": " + statusStr + " (" + QString::number(callCount++) + ")");

    if (isLive) {
        if (!obs_frontend_recording_active()) {
            refreshBrowserSource("Browser");
            obs_frontend_recording_start();
        }
    } else {
        if (obs_frontend_recording_active()) {
            obs_frontend_recording_stop();
        }
    }
}

void Detector::FetchApi() {
    emit startCheckLiveStatus(getWebRid());  // Triggers background thread
}

// using json = nlohmann::json;

// QString randomUserAgent() {
//     const QStringList models = {
//         "SM-G981B", "SM-G9910", "SM-S9080", "SM-S9110", "SM-S921B",
//         "Pixel 5", "Pixel 6", "Pixel 7", "Pixel 7 Pro", "Pixel 8"
//     };

//     int androidVersion = 9 + rand() % 6;
//     QString mobile = models[rand() % models.size()];
//     int chromeVersion = 70 + rand() % 40;

//     return QString("Mozilla/5.0 (Linux; Android %1; %2) AppleWebKit/537.36 "
//                    "(KHTML, like Gecko) Chrome/%3.0.0.0 Mobile Safari/537.36")
//         .arg(androidVersion)
//         .arg(mobile)
//         .arg(chromeVersion);
// }

// static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
//     ((std::string*)userp)->append((char*)contents, size * nmemb);
//     return size * nmemb;
// }

// static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
//     std::string header(buffer, size * nitems);
//     if (header.find("set-cookie:") == 0) {
//         std::string* cookies = (std::string*)userdata;
//         *cookies += header;
//     }
//     return size * nitems;
// }

// QString getTtwid() {
//     CURL *curl = curl_easy_init();
//     std::string response, cookies;

//     if (curl) {
//         curl_easy_setopt(curl, CURLOPT_URL, "https://live.douyin.com/1-2-3-4-5-6-7-8-9-0");
//         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
//         curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
//         curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
//         curl_easy_setopt(curl, CURLOPT_HEADERDATA, &cookies);

//         curl_easy_perform(curl);
//         curl_easy_cleanup(curl);

//         size_t pos = cookies.find("ttwid=");
//         if (pos != std::string::npos) {
//             size_t end = cookies.find(';', pos);
//             return QString::fromStdString(cookies.substr(pos + 6, end - pos - 6));
//         }
//     }
//     return QString();
// }

// bool Detector::isLiveRoomStarted(const QString &webRid) {
//     CURL *curl = curl_easy_init();
//     if (!curl) return false;

//     std::string response;
//     QString ttwid = getTtwid();
//     if (ttwid.isEmpty()) return false;
//     QString url = QString("https://live.douyin.com/webcast/room/web/enter/?web_rid=%1"
//                           "&aid=6383&device_platform=web&browser_language=zh-CN"
//                           "&browser_platform=Win32&browser_name=Mozilla&browser_version=5.0")
//                       .arg(webRid);

//     curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
//     curl_easy_setopt(curl, CURLOPT_USERAGENT, randomUserAgent().toStdString().c_str());

//     QString cookieHeader = QString("Cookie: ttwid=%1").arg(ttwid);
//     struct curl_slist *headers = nullptr;
//     headers = curl_slist_append(headers, cookieHeader.toStdString().c_str());
//     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

//     CURLcode res = curl_easy_perform(curl);
//     curl_slist_free_all(headers);
//     curl_easy_cleanup(curl);

//     if (res != CURLE_OK) return false;

//     try {
//         json j = json::parse(response);
//         int status = j["data"]["data"][0]["status"];
//         std::string user = j["data"]["user"]["nickname"];
//         QString statusStr;
//         if (status == 2) {
//             statusStr = "直播中";
//         } else {
//             statusStr = "空闲";
//         }
//         resultLabel->setText(QString::fromStdString(user) + ": " + statusStr +" (" + QString::number(callCount++) + ")");
//         return status == 2;
//     } catch (...) {
//         resultLabel->setText("Failed to parse JSON");
//     }

//     return false;
// }

QString Detector::getWebRid() const {
    return webRidInput->text();
}

