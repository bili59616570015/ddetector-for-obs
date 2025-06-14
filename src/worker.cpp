#include "worker.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <random>
#include <QStringList>
#include <QThread>

using json = nlohmann::json;

ApiWorker::ApiWorker(QObject *parent) : QObject(parent)
{
	// Initialize nothing here - do it all in initialize()
}

ApiWorker::~ApiWorker()
{
	curl_global_cleanup();
}

QString ApiWorker::randomUserAgent()
{
	const QStringList models = {"SM-G981B", "SM-G9910", "SM-S9080", "SM-S9110",    "SM-S921B",
				    "Pixel 5",  "Pixel 6",  "Pixel 7",  "Pixel 7 Pro", "Pixel 8"};

	int androidVersion = 9 + rand() % 6;
	QString mobile = models[rand() % models.size()];
	int chromeVersion = 70 + rand() % 40;

	return QString("Mozilla/5.0 (Linux; Android %1; %2) AppleWebKit/537.36 "
		       "(KHTML, like Gecko) Chrome/%3.0.0.0 Mobile Safari/537.36")
		.arg(androidVersion)
		.arg(mobile)
		.arg(chromeVersion);
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string *)userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}

static size_t HeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	std::string header(buffer, size * nitems);
	if (header.find("Set-Cookie:") == 0 || header.find("set-cookie:") == 0) {
		std::string *cookies = (std::string *)userdata;
		*cookies += header;
	}
	return size * nitems;
}

void ApiWorker::initialize()
{
	if (m_initialized)
		return;

	// 1. Global CURL initialization
	curl_global_init(CURL_GLOBAL_DEFAULT);

	// 3. Cache Initialization - get initial cookie
	m_ttwid = getTtwid();

	m_initialized = true;
}

QString ApiWorker::getTtwid()
{
	CURL *curl = curl_easy_init();
	std::string response, cookies;

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "https://live.douyin.com/1-2-3-4-5-6-7-8-9-0");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &cookies);

		curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		size_t pos = cookies.find("ttwid=");
		if (pos != std::string::npos) {
			size_t end = cookies.find(';', pos);
			return QString::fromStdString(cookies.substr(pos + 6, end - pos - 6));
		}
	}
	return QString();
}

void ApiWorker::checkLiveStatus(const QString &webRid)
{
	if (!m_initialized) {
		initialize(); // Fallback in case initialization wasn't called
	}
	if (QThread::currentThread()->isInterruptionRequested()) {
		emit liveStatusChecked(std::nullopt, QString("Thread interrupted"));
		return;
	}
	CURL *curl = curl_easy_init();
	if (!curl) {
		emit liveStatusChecked(std::nullopt, QString("CURL initialization failed"));
		return;
	};

	std::string response;
	QString ttwid = m_ttwid.isEmpty() ? getTtwid() : m_ttwid;
	if (ttwid.isEmpty()) {
		emit liveStatusChecked(std::nullopt, QString("Failed to get ttwid"));
		return;
	};
	QString url = QString("https://live.douyin.com/webcast/room/web/enter/?web_rid=%1"
			      "&aid=6383&device_platform=web&browser_language=zh-CN"
			      "&browser_platform=Win32&browser_name=Mozilla&browser_version=5.0")
			      .arg(webRid);

	curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
	curl_easy_setopt(curl, CURLOPT_USERAGENT, randomUserAgent().toStdString().c_str());

	QString cookieHeader = QString("Cookie: ttwid=%1").arg(ttwid);
	struct curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, cookieHeader.toStdString().c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	CURLcode res = curl_easy_perform(curl);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		emit liveStatusChecked(std::nullopt, "Network Error");
		return;
	};

	try {
		json j = json::parse(response);
		int status = -1; // default value
		if (!j["data"]["data"].empty()) {
			status = j["data"]["data"][0].value("status", -1);
		}
		std::string user = j["data"]["user"]["nickname"];
		emit liveStatusChecked(std::optional(status == 2), QString::fromStdString(user));
		return;
	} catch (...) {
		emit liveStatusChecked(std::nullopt, QString("JSON parsing error"));
		return;
	}
}
