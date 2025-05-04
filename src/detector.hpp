#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QString>
#include <obs-frontend-api.h>
#include <obs-module.h>

class Detector : public QWidget {
	Q_OBJECT
public:
    explicit Detector(QWidget *parent = nullptr);
    ~Detector();
private:
    void *vendor;
    QPushButton *startButton;
    QPushButton *stopButton;
    QLabel *resultLabel;
    QLineEdit *webRidInput;
    QTimer *timer;
    int callCount = 0;
public:
    void PostLoad();
    void FetchApi();
    void StartButtonClicked();
    void StopButtonClicked();
    bool isLiveRoomStarted(const QString &webRid);
    void refreshBrowserSource(const char *source_name);
    QString getWebRid() const;
};
