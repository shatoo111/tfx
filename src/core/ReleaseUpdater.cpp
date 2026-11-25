#include "core/ReleaseUpdater.h"
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QVersionNumber>
#include <QRegularExpression>
#include <QSysInfo>

namespace te {
namespace {
const QString endpoint = QStringLiteral("https://api.github.com/repos/shatoo111/tgtopic/releases/latest");
#ifdef Q_OS_WIN
const QString packageName = QStringLiteral("TFX-setup-x64.exe");
#else
const QString packageName = QStringLiteral("TFX-macos.zip");
const QString universalLabel = QStringLiteral("macOS universal (arm64 + x86_64)");
QDir macBundle() {
    QDir bundle(QCoreApplication::applicationDirPath());
    bundle.cdUp();
    bundle.cdUp();
    return bundle;
}
#endif
}
ReleaseUpdater::ReleaseUpdater(QObject* parent) : QObject(parent) {
#ifdef Q_OS_WIN
    QSettings settings;
    const QFileInfo file(settings.value("updates/installer").toString());
    QDir dir = file.absoluteDir();
    const QString name = dir.dirName();
    if (file.fileName() == packageName && name.startsWith("TFX-update-") && dir.cdUp() &&
        dir.canonicalPath() == QDir(QDir::tempPath()).canonicalPath() &&
        !file.isSymLink() && !QFileInfo(file.absolutePath()).isSymLink()) {
        if (!file.exists() || QFile::remove(file.absoluteFilePath())) {
            dir.rmdir(name);
            settings.remove("updates/installer");
        }
    }
#elif defined(Q_OS_MACOS)
    connect(&prepare_, &QProcess::finished, this, &ReleaseUpdater::finishMacPreparation);
    connect(&prepare_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) finishMacPreparation(-1, QProcess::CrashExit);
    });
#endif
}
QNetworkRequest ReleaseUpdater::request(const QUrl& url, const QByteArray& accept) const {
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "TFX-update");
    req.setRawHeader("Accept", accept);
    if (url.host() == "api.github.com" && !token_.isEmpty())
        req.setRawHeader("Authorization", "Bearer " + token_.toUtf8());
    req.setTransferTimeout(45000);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    return req;
}
void ReleaseUpdater::finish(const QString& message) {
    busy_ = false;
    message_ = message;
    if (!asset_.isValid()) token_.clear();
    emit changed();
}
void ReleaseUpdater::check(const QString& token) {
    if (busy_) return;
    busy_ = true;
    asset_ = QUrl();
    notes_.clear();
    token_ = token.trimmed();
    message_ = QStringLiteral("Проверяем стабильные релизы…");
    emit changed();
    auto* reply = network_.get(request(QUrl(endpoint), "application/vnd.github+json"));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == 401 || code == 403 || code == 404) {
            needsToken_ = true;
            finish(QStringLiteral("Для закрытого репозитория укажите GitHub-токен с доступом Contents: Read. Также проверьте доступ к репозиторию и лимит запросов."));
            return;
        }
        if (reply->error() || code != 200) {
            finish(QStringLiteral("Проверка не удалась: ") + reply->errorString());
            return;
        }
        needsToken_ = false;
        const auto release = QJsonDocument::fromJson(reply->readAll()).object();
        const auto tag = release.value("tag_name").toString();
        static const QRegularExpression stable(QStringLiteral(R"(^v[0-9]+[.][0-9]+[.][0-9]+$)"));
        if (!stable.match(tag).hasMatch() || release.value("draft").toBool() || release.value("prerelease").toBool()) {
            finish(QStringLiteral("GitHub вернул неожиданный номер стабильной версии."));
            return;
        }
        notes_ = release.value("body").toString().left(12000);
        const auto current = QCoreApplication::applicationVersion();
        const int comparison = QVersionNumber::compare(QVersionNumber::fromString(tag.mid(1)), QVersionNumber::fromString(current));
        if (comparison < 0 || (comparison == 0 && !current.contains('-'))) {
            finish(QStringLiteral("Установлена TFX %1. Последняя стабильная версия — %2.")
                .arg(QCoreApplication::applicationVersion(), tag));
            return;
        }
        for (const auto& entry : release.value("assets").toArray()) {
            const auto a = entry.toObject();
            if (a.value("name").toString() != packageName) continue;
#ifdef Q_OS_MACOS
            // Earlier archives used the same filename for an arm64-only build.
            if (QSysInfo::buildCpuArchitecture() == "x86_64" && a.value("label").toString() != universalLabel)
                continue;
#endif
            const auto digest = a.value("digest").toString();
            static const QRegularExpression sha(QStringLiteral("^sha256:[0-9a-f]{64}$"));
            const auto id = a.value("id").toInteger();
            expected_ = a.value("size").toInteger();
            if (!sha.match(digest).hasMatch() || id <= 0 || expected_ <= 0 || expected_ > 700000000) break;
            digest_ = digest.mid(7).toLatin1();
            asset_ = QUrl(QStringLiteral("https://api.github.com/repos/shatoo111/tgtopic/releases/assets/%1").arg(id));
            break;
        }
        finish(asset_.isValid() ? QStringLiteral("Доступна TFX ") + tag :
            QStringLiteral("Доступна %1. Совместимый пакет с SHA-256 отсутствует.").arg(tag));
    });
}
void ReleaseUpdater::install() {
    if (!available()) return;
    stage_ = std::make_unique<QTemporaryDir>(QDir::tempPath() + "/TFX-update-XXXXXX");
    package_.setFileName(stage_->filePath(packageName));
    if (!stage_->isValid() || !package_.open(QIODevice::WriteOnly)) {
        finish(QStringLiteral("Не удалось создать временный файл обновления."));
        return;
    }
    busy_ = true;
    message_ = QStringLiteral("Скачиваем и проверяем обновление…");
    hash_.reset();
    emit changed();
    download(asset_, 0);
}
void ReleaseUpdater::download(const QUrl& url, int redirects) {
    auto* reply = network_.get(request(url, "application/octet-stream"));
    const auto drain = [this, reply] {
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) return;
        const auto data = reply->readAll();
        if (package_.size() + data.size() > expected_ || package_.write(data) != data.size()) {
            reply->abort();
            return;
        }
        hash_.addData(data);
    };
    connect(reply, &QNetworkReply::readyRead, this, drain);
    connect(reply, &QNetworkReply::finished, this, [this, reply, redirects, drain] {
        reply->deleteLater();
        const auto redir = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redir.isEmpty()) {
            const auto target = reply->url().resolved(redir);
            if (redirects < 3 && target.scheme() == "https" && target.host().endsWith(".githubusercontent.com")) {
                download(target, redirects + 1);
                return;
            }
        }
        drain();
        const bool valid = !reply->error() && package_.flush() && package_.size() == expected_ &&
            hash_.result().toHex() == digest_;
        package_.close();
        if (!valid) {
            stage_.reset();
            finish(QStringLiteral("Загрузка или проверка SHA-256 не удалась. Можно повторить попытку."));
            return;
        }
        launchInstaller();
    });
}
void ReleaseUpdater::launchInstaller() {
#ifdef Q_OS_WIN
    QStringList args{QStringLiteral("/CURRENTUSER")};
    const QDir runtime(QCoreApplication::applicationDirPath());
    if (runtime.dirName().compare("bin", Qt::CaseInsensitive) == 0) {
        QDir install = runtime;
        install.cdUp();
        if (QFileInfo::exists(install.filePath("plugins/platforms/qwindows.dll")))
            args << "/DIR=" + QDir::toNativeSeparators(install.absolutePath());
    }
    if (!QProcess::startDetached(package_.fileName(), args)) {
        finish(QStringLiteral("Не удалось запустить установщик."));
        return;
    }
    QSettings().setValue("updates/installer", package_.fileName());
    stage_->setAutoRemove(false);
    QCoreApplication::quit();
#else
    const QDir bundle = macBundle();
    QDir parent = bundle;
    parent.cdUp();
    updateWorkspace_ = std::make_unique<QTemporaryDir>(parent.filePath(".TFX-update-XXXXXX"));
    const auto script = updateWorkspace_->filePath("apply-update.sh");
    if (!updateWorkspace_->isValid() ||
        !QFile::copy(bundle.filePath("Contents/Resources/apply-update.sh"), script)) {
        updateWorkspace_.reset();
        finish(QStringLiteral("Не удалось подготовить обновление рядом с TFX. Проверьте доступ к папке приложения."));
        return;
    }
    message_ = QStringLiteral("Проверяем подпись и совместимость пакета…");
    emit changed();
    prepare_.start(QStringLiteral("/bin/bash"),
        {script, "prepare", bundle.absolutePath(), updateWorkspace_->path(),
         package_.fileName(), QSysInfo::buildCpuArchitecture()});
#endif
}
#ifdef Q_OS_MACOS
void ReleaseUpdater::finishMacPreparation(int exitCode, QProcess::ExitStatus status) {
    if (status != QProcess::NormalExit || exitCode != 0) {
        updateWorkspace_.reset();
        finish(QStringLiteral("Проверка подписи или совместимости пакета не прошла."));
        return;
    }
    const QStringList args{updateWorkspace_->filePath("apply-update.sh"), "apply",
         macBundle().absolutePath(), updateWorkspace_->path(), QString::number(QCoreApplication::applicationPid())};
    if (!QProcess::startDetached(QStringLiteral("/bin/bash"), args)) {
        updateWorkspace_.reset();
        finish(QStringLiteral("Не удалось запустить установщик."));
        return;
    }
    updateWorkspace_->setAutoRemove(false);
    QCoreApplication::quit();
}
#endif
}
