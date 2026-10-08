#include "ScriptManager.h"
#include "ScriptEngineHost.h"
#include "Config/ConfigStore.h"
#include "LuaScript/Engine/TypeScriptTranspiler.h"

#include <QApplication>
#include <QCursor>
#include <QFileInfo>

ScriptManager::ScriptManager(ScriptEngineHost* host, ConfigStore* config, QWidget* parent)
    : QObject(parent)
    , m_host(host)
    , m_config(config)
    , m_parentWidget(parent)
    , m_pCurrentScriptEditor(nullptr)
    , m_nCurrentScriptIndex(-1)
{
    if (m_host) {
        connect(m_host, &ScriptEngineHost::scriptFinished, this,
                [this](int index, bool ok, const QString& err) {
                    if (index >= 0 && index < m_rowPhase.size()
                        && m_rowPhase[index] == ScriptRunPhase::Running) {
                        setRowPhase(index, ScriptRunPhase::Idle);
                    }
                    if (m_pCurrentScriptEditor && index == m_nCurrentScriptIndex)
                        m_pCurrentScriptEditor->onRunFinished(ok, err);
                    if (ok)
                        emit logMessage(tr("脚本 %1 执行完成").arg(index + 1));
                });
    }
}

ScriptManager::~ScriptManager() = default;

void ScriptManager::setScriptCount(int count)
{
    m_scriptCount = count;
    m_languages.resize(count);
    m_rowPhase.resize(count);
    for (int i = 0; i < count; ++i) {
        m_languages[i] = ScriptLanguage::Lua;
        m_rowPhase[i] = ScriptRunPhase::Idle;
    }
}

ScriptRunPhase ScriptManager::rowPhase(int index) const
{
    if (index < 0 || index >= m_rowPhase.size())
        return ScriptRunPhase::Idle;
    return m_rowPhase[index];
}

QString ScriptManager::scriptDir() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/Config/LuaScript");
}

QString ScriptManager::scriptPath(int index) const
{
    const ScriptLanguage lang = language(index);
    return scriptDir() + QLatin1Char('/')
         + ScriptLanguageUtil::fileBaseName(index, lang);
}

ScriptLanguage ScriptManager::language(int index) const
{
    if (index < 0 || index >= m_languages.size())
        return ScriptLanguage::Lua;
    return m_languages[index];
}

void ScriptManager::setLanguage(int index, ScriptLanguage lang)
{
    if (index < 0 || index >= m_languages.size())
        return;
    m_languages[index] = lang;
    if (index < m_langCombos.size() && m_langCombos[index]) {
        const int comboIndex = m_langCombos[index]->findData(static_cast<int>(lang));
        if (comboIndex >= 0)
            m_langCombos[index]->setCurrentIndex(comboIndex);
    }
}

void ScriptManager::loadLanguagePrefs()
{
    if (!m_config || m_scriptCount <= 0)
        return;

    QStringList langs;
    if (!m_config->LoadScriptLanguages(langs))
        return;

    for (int i = 0; i < m_scriptCount && i < langs.size(); ++i)
        setLanguage(i, ScriptLanguageUtil::fromConfigValue(langs[i]));
}

void ScriptManager::saveLanguagePrefs()
{
    if (!m_config || m_scriptCount <= 0)
        return;

    QStringList langs;
    for (int i = 0; i < m_scriptCount; ++i)
        langs << ScriptLanguageUtil::toConfigValue(language(i));
    m_config->SaveScriptLanguages(langs);
}

void ScriptManager::setRowPhase(int index, ScriptRunPhase phase)
{
    if (index < 0 || index >= m_rowPhase.size())
        return;
    if (m_rowPhase[index] == phase)
        return;

    if (m_rowPhase[index] == ScriptRunPhase::Compiling && phase != ScriptRunPhase::Compiling)
        emit scriptCompileFinished(index);
    if (phase == ScriptRunPhase::Compiling)
        emit scriptCompileStarted(index);

    m_rowPhase[index] = phase;
    updateExecuteButton(index);
    emit scriptPhaseChanged(index, phase);
}

void ScriptManager::updateExecuteButton(int index)
{
    if (index < 0 || index >= m_execBtns.size())
        return;
    QPushButton* btn = m_execBtns[index];
    if (!btn)
        return;

    switch (m_rowPhase[index]) {
    case ScriptRunPhase::Compiling:
        btn->setEnabled(false);
        btn->setText(tr("编译中…"));
        break;
    case ScriptRunPhase::Running:
        btn->setEnabled(true);
        btn->setText(tr("停止"));
        break;
    case ScriptRunPhase::Idle:
    default:
        btn->setEnabled(true);
        btn->setText(tr("执行"));
        break;
    }
}

void ScriptManager::retranslateScriptRows()
{
    for (int i = 0; i < m_langCombos.size(); ++i) {
        QComboBox* combo = m_langCombos[i];
        if (!combo)
            continue;

        const QVariant langData = combo->currentData();
        combo->blockSignals(true);
        combo->clear();
        combo->addItem(tr("Lua"), static_cast<int>(ScriptLanguage::Lua));
        combo->addItem(tr("TS"), static_cast<int>(ScriptLanguage::TypeScript));
        combo->setToolTip(tr("脚本语言: Lua 或 TypeScript (编译为 Lua 后执行)"));

        const int comboIndex = combo->findData(langData);
        if (comboIndex >= 0)
            combo->setCurrentIndex(comboIndex);
        combo->blockSignals(false);
    }

    for (int i = 0; i < m_execBtns.size(); ++i)
        updateExecuteButton(i);
}

bool ScriptManager::resolveLuaSourceSync(int index, const QString& editorContent, QString& luaOut,
                                          QString& errorMsg) const
{
    const ScriptLanguage lang = language(index);

    if (lang == ScriptLanguage::Lua) {
        if (!editorContent.isNull()) {
            luaOut = editorContent;
            return true;
        }
        const QString path = scriptPath(index);
        if (!QFile::exists(path)) {
            errorMsg = tr("脚本不存在: %1").arg(path);
            return false;
        }
        luaOut = readTextFile(path);
        return true;
    }

    QString tsSource = editorContent.isNull() ? readTextFile(scriptPath(index)) : editorContent;
    if (tsSource.isEmpty() && !QFile::exists(scriptPath(index))) {
        errorMsg = tr("脚本不存在: %1").arg(scriptPath(index));
        return false;
    }

    const QString cacheKey = editorContent.isNull()
                                 ? scriptPath(index)
                                 : QStringLiteral("slot:%1").arg(index);
    return TypeScriptTranspiler::transpileFromSource(tsSource, luaOut, errorMsg, cacheKey);
}

bool ScriptManager::checkLuaSource(ScriptLanguage lang, const QString& source, QString& errorMsg) const
{
    if (lang == ScriptLanguage::TypeScript) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QString luaOut;
        const bool ok = TypeScriptTranspiler::transpileFromSource(source, luaOut, errorMsg);
        QApplication::restoreOverrideCursor();
        return ok;
    }

    if (!m_host)
        return false;
    return m_host->checkScript(source, errorMsg);
}

void ScriptManager::failRun(int index, const QString& title, const QString& errorMsg)
{
    setRowPhase(index, ScriptRunPhase::Idle);
    QMessageBox::critical(m_parentWidget, title, errorMsg);
    emit logMessage(errorMsg);
    if (m_pCurrentScriptEditor && index == m_nCurrentScriptIndex)
        m_pCurrentScriptEditor->onRunFinished(false, errorMsg);
}

void ScriptManager::dispatchLuaRun(int index, const QString& luaOut)
{
    if (!m_host)
        return;
    const bool loop = index >= 0 && index < m_loopChks.size() && m_loopChks[index]
                      && m_loopChks[index]->isChecked();
    m_host->prepareEngineForRun(index, loop);
    setRowPhase(index, ScriptRunPhase::Running);
    m_host->runScriptAsync(index, luaOut);
}

void ScriptManager::stopScript(int index)
{
    if (!m_host || index < 0 || index >= m_rowPhase.size())
        return;
    if (m_rowPhase[index] != ScriptRunPhase::Running)
        return;

    m_host->requestStop(index);
    emit logMessage(tr("正在停止脚本 %1…").arg(index + 1));
}

void ScriptManager::runScriptWithContent(int index, const QString& editorContent)
{
    if (!m_host || index < 0 || index >= m_rowPhase.size())
        return;

    if (m_rowPhase[index] != ScriptRunPhase::Idle) {
        if (m_pCurrentScriptEditor && index == m_nCurrentScriptIndex)
            m_pCurrentScriptEditor->onRunFinished(false, tr("脚本正在编译或运行中。"));
        return;
    }

    const ScriptLanguage lang = language(index);
    const QString errorTitle = lang == ScriptLanguage::TypeScript ? tr("TypeScript 编译错误")
                                                                  : tr("Lua执行错误");

    if (lang == ScriptLanguage::Lua) {
        QString luaOut;
        QString errorMsg;
        if (!resolveLuaSourceSync(index, editorContent, luaOut, errorMsg)) {
            failRun(index, errorTitle, errorMsg);
            return;
        }
        dispatchLuaRun(index, luaOut);
        return;
    }

    QString tsSource = editorContent.isNull() ? readTextFile(scriptPath(index)) : editorContent;
    if (tsSource.isEmpty() && !QFile::exists(scriptPath(index))) {
        failRun(index, errorTitle, tr("脚本不存在: %1").arg(scriptPath(index)));
        return;
    }

    const QString cacheKey = editorContent.isNull()
                                 ? scriptPath(index)
                                 : QStringLiteral("slot:%1").arg(index);

    setRowPhase(index, ScriptRunPhase::Compiling);

    TypeScriptTranspiler::transpileFromSourceAsync(
        tsSource, cacheKey, this,
        [this, index, errorTitle](bool ok, const QString& luaOut, const QString& errorMsg) {
            if (!ok) {
                failRun(index, errorTitle, errorMsg);
                return;
            }
            dispatchLuaRun(index, luaOut);
        });
}

void ScriptManager::runScript(int index)
{
    runScriptWithContent(index, QString());
}

void ScriptManager::bindScriptRow(int index, QPushButton* execBtn, QPushButton* editBtn,
                                QCheckBox* loopChk, QComboBox* langCombo)
{
    if (index >= m_execBtns.size())
        m_execBtns.resize(index + 1);
    m_execBtns[index] = execBtn;
    if (execBtn)
        updateExecuteButton(index);

    if (langCombo) {
        if (index >= m_langCombos.size())
            m_langCombos.resize(index + 1);
        m_langCombos[index] = langCombo;

        langCombo->clear();
        langCombo->addItem(tr("Lua"), static_cast<int>(ScriptLanguage::Lua));
        langCombo->addItem(tr("TS"), static_cast<int>(ScriptLanguage::TypeScript));
        langCombo->setToolTip(tr("脚本语言: Lua 或 TypeScript (编译为 Lua 后执行)"));

        const int comboIndex = langCombo->findData(static_cast<int>(language(index)));
        if (comboIndex >= 0)
            langCombo->setCurrentIndex(comboIndex);

        connect(langCombo, &QComboBox::currentIndexChanged, this, [this, index](int idx) {
            if (idx < 0)
                return;
            const auto lang = static_cast<ScriptLanguage>(m_langCombos[index]->itemData(idx).toInt());
            m_languages[index] = lang;
            saveLanguagePrefs();
        });
    }

    if (execBtn) {
        connect(execBtn, &QPushButton::clicked, this, [this, index]() {
            if (index >= 0 && index < m_rowPhase.size()
                && m_rowPhase[index] == ScriptRunPhase::Running) {
                stopScript(index);
                return;
            }
            runScript(index);
        });
    }

    if (editBtn)
        connect(editBtn, &QPushButton::clicked, this, [this, index]() { openScriptEditor(index); });

    if (loopChk) {
        if (index >= m_loopChks.size())
            m_loopChks.resize(index + 1);
        m_loopChks[index] = loopChk;
        connect(loopChk, &QCheckBox::stateChanged, this, [this, index](int state) {
            if (m_host && (index >= m_rowPhase.size()
                           || m_rowPhase[index] != ScriptRunPhase::Running))
                m_host->setLoopValid(index, state == Qt::Checked);
        });
    }
}

void ScriptManager::openScriptEditor(int index)
{
    if (!m_host)
        return;

    const QString path = scriptPath(index);
    const ScriptLanguage lang = language(index);

    QDir().mkpath(QFileInfo(path).absolutePath());

    if (!prepareForNewEditor(index))
        return;

    ScriptEditor* pScriptEditor = new ScriptEditor(m_parentWidget);
    pScriptEditor->setScriptLanguage(lang);
    pScriptEditor->setRunFn([this, index](const QString& content) {
        runScriptWithContent(index, content);
    });
    pScriptEditor->setCheckFn([this, lang](const QString& script, QString& err) {
        return checkLuaSource(lang, script, err);
    });
    pScriptEditor->setFunctionDocs(m_host->functionDocs());
    pScriptEditor->setAttribute(Qt::WA_DeleteOnClose);
    pScriptEditor->setScriptName(path);

    connect(this, &ScriptManager::scriptPhaseChanged, pScriptEditor,
            [pScriptEditor, index](int i, ScriptRunPhase phase) {
                if (i == index)
                    pScriptEditor->onRunPhaseChanged(phase);
            });

    pScriptEditor->loadScript(readTextFile(path));

    m_pCurrentScriptEditor = pScriptEditor;
    m_nCurrentScriptIndex = index;

    connect(pScriptEditor, &QObject::destroyed, this, [this]() {
        m_pCurrentScriptEditor = nullptr;
        m_nCurrentScriptIndex = -1;
    });

    pScriptEditor->show();
}

bool ScriptManager::prepareForNewEditor(int index)
{
    if (!m_pCurrentScriptEditor)
        return true;

    if (m_nCurrentScriptIndex == index) {
        m_pCurrentScriptEditor->raise();
        m_pCurrentScriptEditor->activateWindow();
        return false;
    }

    if (m_pCurrentScriptEditor->isModified()) {
        QMessageBox msgBox(m_parentWidget);
        msgBox.setWindowTitle(tr("切换脚本"));
        msgBox.setText(tr("当前脚本有未保存的更改,切换前是否保存?"));
        msgBox.setIcon(QMessageBox::Question);

        QPushButton* saveButton = msgBox.addButton(tr("保存"), QMessageBox::YesRole);
        msgBox.addButton(tr("不保存"), QMessageBox::NoRole);
        QPushButton* cancelButton = msgBox.addButton(tr("取消"), QMessageBox::RejectRole);

        msgBox.exec();

        QAbstractButton* clickedButton = msgBox.clickedButton();
        if (clickedButton == cancelButton)
            return false;

        if (clickedButton == saveButton)
            writeTextFile(scriptPath(m_nCurrentScriptIndex), m_pCurrentScriptEditor->getScriptContent());
    }

    delete m_pCurrentScriptEditor;
    m_pCurrentScriptEditor = nullptr;
    m_nCurrentScriptIndex = -1;
    return true;
}

QString ScriptManager::readTextFile(const QString& path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    return in.readAll();
}

void ScriptManager::writeTextFile(const QString& path, const QString& text) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << text;
}
