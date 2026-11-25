# Подготовка сборок

`cmake --install` вызывает Qt deployment script, который переносит нужные Qt-библиотеки, platform/image plugins и QML imports. Скрипты `macos/build.sh` и `windows/build.ps1` сначала выполняют тесты. `cpack --config build/<platform>/CPackConfig.cmake` может создать DMG (macOS) или ZIP (Windows).

Workflow `build-and-test` собирает Windows и macOS и запускает официальный Telegram Desktop. Тег вида `v0.4.0-rc.1` публикует предварительный релиз. Для стабильного релиза запустите workflow на проверенном коммите с параметрами `publish_release` и `manual_qa_passed` после [аппаратной приёмки](../docs/manual-test-plan.md). Текст изменений берётся из `docs/releases/<версия>.md`.

- Windows x64: распакуйте `TFX-windows-x64.zip` и запустите `bin/tfx.exe`.
- macOS (Apple Silicon и Intel): распакуйте универсальный архив `TFX-macos.zip` и откройте `TFX.app`.

В стабильной сборке Actions импортирует сертификат Developer ID Application и проверяет нотариальный билет. Настройте секреты репозитория `MACOS_CERTIFICATE_P12` (PKCS#12 в base64), `MACOS_CERTIFICATE_PASSWORD`, `MACOS_SIGN_IDENTITY`, `MACOS_APPLE_ID`, `MACOS_TEAM_ID` и `MACOS_APP_PASSWORD` (пароль приложения). Предварительные сборки используют ad-hoc. Windows-установщик `TFX-setup-x64.exe` обновляет файлы в каталоге установки.

## macOS

1. Собирать с официальным Apple SDK, Qt для нужных архитектур и deployment target 13.0.
2. Установить через `cmake --install`, проверить запуск `.app` вне build-каталога.
3. Для распространения подписать вложенные frameworks / dylibs / plugins изнутри наружу действительным Developer ID Application, затем сам bundle с hardened runtime. Не использовать `--deep` как замену корректному порядку подписи.
4. Проверить `codesign --verify --deep --strict --verbose=2`, создать архив, отправить `xcrun notarytool submit ... --wait` с собственными credentials, stapling и `spctl --assess`.
5. На чистом Mac выполнить ручной протокол. Не удалять quarantine массово и не отключать Gatekeeper.

`TFX.app` отображает фирменную иконку в Finder и Dock. Управление также доступно через иконку TFX в строке меню. Нет Accessibility / Screen Capture entitlements или sandbox-обходов. App Store sandbox не является целью этой версии.

## Windows

1. Собирать MSVC x64 + соответствующим Qt. Manifest включает Per-Monitor V2 DPI и `asInvoker`.
2. Проверить весь `dist/windows` на машине без Qt, включая Visual C++ Runtime (поставить официально при необходимости).
3. Для релиза подписать executable и установщик Authenticode действительным сертификатом организации; timestamp обязателен.
4. Запаковать **весь** каталог, не только exe. Не отключать SmartScreen или Defender.

Готовую сборку Qt для MinGW можно упаковать в один файл. Передайте абсолютный префикс в `cmake --install`, затем из корня репозитория запустите:

```powershell
.\packaging\windows\package-portable.ps1 -InstallDirectory .\dist\windows `
  -MinGWBin C:\Qt\Tools\mingw1310_64\bin -OutputFile .\dist\tfx.exe
```

Для включения лицензионных текстов и уведомлений поставки используйте параметр `-NoticesDirectory`. При первом запуске EXE распаковывает Qt и приложение в `%LOCALAPPDATA%\TelegramEffectsPortable`. Повторный запуск использует уже распакованную сборку.

## Лицензии Qt

Используются динамические Qt Core/Gui/Widgets/Qml/Quick/Quick Controls/SVG/Test. При выборе open-source Qt необходимо включить лицензионные тексты и уведомления соответствующей поставки, обеспечить права получателя по применимым лицензиям (включая возможность замены динамических библиотек и доступ к соответствующему исходному коду Qt). Перед публикацией пакета проверьте его состав и лицензионные обязательства; лицензионный выбор самого приложения остаётся за владельцем проекта.

В поставку не включены товарные знаки, исполняемые файлы или ресурсы Telegram / Flake.

## Проверка Mac-архива после развёртывания Qt

`packaging/macos/package.sh` подписывает все вложенные Mach-O файлы, затем frameworks и app bundle после завершения Qt deployment. По умолчанию используется ad-hoc подпись для проверки целостности, а не сертификат доверенного издателя. Проверка `codesign --verify --deep --strict` обязательна до и после ZIP. Распакованная копия проходит стартовый тест; рядом с архивом создаётся SHA-256.

При наличии сертификата задайте `MACOS_SIGN_IDENTITY='Developer ID Application: …'`. Для нотариализации дополнительно задайте `MACOS_NOTARY_PROFILE` — имя профиля, заранее сохранённого через `notarytool store-credentials`. Скрипт отправит подписанный архив Apple, прикрепит ticket и проверит Gatekeeper. Пароли и приватные ключи не хранятся в репозитории.

Без Developer ID и нотариализации исправленная целостная сборка всё ещё может потребовать разрешение для конкретного приложения в «Системные настройки → Конфиденциальность и безопасность → Всё равно открыть». Это не эквивалент доверенной нотариализованной поставке. Не отключайте Gatekeeper глобально и не снимайте quarantine массово.
