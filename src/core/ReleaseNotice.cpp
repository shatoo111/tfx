#include "core/ReleaseNotice.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QVersionNumber>

namespace te {
namespace {
constexpr qint64 answerLimit = 64 * 1024;
constexpr qint64 quietMs = 30 * 60 * 1000;
#ifdef Q_OS_WIN
const QString platformName = QStringLiteral("win");
#else
const QString platformName = QStringLiteral("mac");
#endif
QVersionNumber number(const QString& version) { return QVersionNumber::fromString(version).normalized(); }
}
ReleaseNotice::ReleaseNotice(QObject* parent, const QUrl& endpoint) : QObject(parent), endpoint_(endpoint) {
    timer_.setInterval(4 * 60 * 60 * 1000);
    timer_.setTimerType(Qt::VeryCoarseTimer);
    connect(&timer_, &QTimer::timeout, this, &ReleaseNotice::check);
}
QString ReleaseNotice::current() const {
    return QCoreApplication::applicationVersion().section(QLatin1Char('-'), 0, 0);
}
void ReleaseNotice::start() {
    if (started_) return;
    started_ = true;
    timer_.start();
    QTimer::singleShot(3000, this, &ReleaseNotice::check);
}
void ReleaseNotice::refresh() {
    if (started_ && last_.isValid() && last_.elapsed() > quietMs) check();
}
void ReleaseNotice::dismiss() {
    if (dismissed_ == version_) return;
    dismissed_ = version_;
    emit changed();
}
void ReleaseNotice::check() {
    if (busy_) return;
    busy_ = true;
    last_.start();
    QNetworkRequest request(endpoint_);
    request.setRawHeader("User-Agent", "TFX");
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(20000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    auto* reply = network_.get(request);
    connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received) { if (received > answerLimit) reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        busy_ = false;
        if (!reply->error() && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200) {
            const QString found = newer(reply->read(answerLimit), platformName, current());
            if (found != version_) {
                version_ = found;
                emit changed();
            }
        }
        emit checked();
    });
}
QString ReleaseNotice::newer(const QByteArray& answer, const QString& platform, const QString& installed) {
    const auto version = QJsonDocument::fromJson(answer).object().value(platform).toObject().value(QLatin1String("version")).toString();
    static const QRegularExpression shape(QStringLiteral("^[0-9]{1,4}[.][0-9]{1,4}[.][0-9]{1,4}$"));
    if (!shape.match(version).hasMatch() || number(installed).isNull()) return {};
    return number(version) > number(installed) ? version : QString();
}
}
