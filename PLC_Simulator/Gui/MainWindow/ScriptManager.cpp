#include "ScriptManager.h"
#include "ScriptEngineHost.h"

ScriptManager::ScriptManager(ScriptEngineHost* host, QWidget* parent)
    : QObject(parent)
    , m_host(host)
    , m_parentWidget(parent)
    , m_pCurrentScriptEditor(nullptr)
    , m_nCurrentScriptIndex(-1)
{
}

ScriptManager::~ScriptManager()
{
    // 编辑器会自动删除（设置了 WA_DeleteOnClose）
}

void ScriptManager::initScriptExecution()
{
    connect(this, &ScriptManager::executeLuaScript, this, [this](int nLuaIndex, QString strLuaFile) {
        if (m_host == nullptr) return;
        m_host->runScript(nLuaIndex, strLuaFile);
    });

    // 脚本完成 → 若是当前编辑器对应索引,转交编辑器收尾并提示(GUI 线程)
    if (m_host) {
        connect(m_host, &ScriptEngineHost::scriptFinished, this,
                [this](int index, bool ok, const QString& err) {
                    if (m_pCurrentScriptEditor && index == m_nCurrentScriptIndex - 1)
                        m_pCurrentScriptEditor->onRunFinished(ok, err);
                });
    }
}

void ScriptManager::executeScript(int index)
{
    if (m_host == nullptr) return;

    QString strLuaPath = QCoreApplication::applicationDirPath();
    strLuaPath += "/Config/LuaScript/";
    QString luaFileName = QString("LuaFile%1.lua").arg(index + 1);
    strLuaPath += luaFileName;

    emit executeLuaScript(index, strLuaPath);
}

void ScriptManager::setLoopEnable(int index, bool enable)
{
    if (m_host == nullptr) return;

    m_host->setLoopValid(index, enable);
}

void ScriptManager::connectExecuteButton(int index, QPushButton* button)
{
    if (!button) return;

    connect(button, &QPushButton::clicked, this, [this, index]() {
        executeScript(index);
    });
}

void ScriptManager::connectEditButton(int scriptIndex, QPushButton* button)
{
    if (!button) return;

    connect(button, &QPushButton::clicked, this, [this, scriptIndex]() {
        openScriptEditor(scriptIndex);
    });
}

void ScriptManager::connectLoopCheckBox(int index, QCheckBox* checkBox)
{
    if (!checkBox) return;

    connect(checkBox, &QCheckBox::stateChanged, this, [this, index](int state) {
        setLoopEnable(index, state == Qt::Checked);
    });
}

void ScriptManager::openScriptEditor(int scriptIndex)
{
    if (m_host == nullptr) return;

    // 获取可执行文件所在目录
    QString appDir = QCoreApplication::applicationDirPath();
    QString scriptPath = appDir + "/Config/LuaScript/LuaFile" + QString::number(scriptIndex) + ".lua";

    // 确保目录存在
    QDir dir(appDir + "/Config/LuaScript");
    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    // 如果文件不存在，创建一个空文件
    QFile file(scriptPath);
    if (!file.exists())
    {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << "-- Lua Script File " << scriptIndex << "\n";
            out << "-- Write your Lua code here\n";
            file.close();
        }
    }

    // 检查是否已经有编辑器打开
    if (m_pCurrentScriptEditor != nullptr)
    {
        // 如果要打开的脚本就是当前打开的脚本，直接置于前台
        if (m_nCurrentScriptIndex == scriptIndex)
        {
            m_pCurrentScriptEditor->raise();
            m_pCurrentScriptEditor->activateWindow();
            return;
        }

        // 弹出确认对话框
        QMessageBox msgBox(m_parentWidget);
        msgBox.setWindowTitle("确认切换脚本");
        msgBox.setText("是否保存当前脚本更改并切换到新的脚本？");
        msgBox.setIcon(QMessageBox::Question);

        QPushButton* saveButton = msgBox.addButton("是", QMessageBox::YesRole);
        QPushButton* discardButton = msgBox.addButton("否", QMessageBox::NoRole);
        QPushButton* cancelButton = msgBox.addButton("取消", QMessageBox::RejectRole);

        msgBox.exec();

        QAbstractButton* clickedButton = msgBox.clickedButton();

        if (clickedButton == cancelButton)
        {
            return;
        }
        else if (clickedButton == saveButton)
        {
            QString currentScriptPath = appDir + "/Config/LuaScript/LuaFile" +
                QString::number(m_nCurrentScriptIndex) + ".lua";
            QFile currentFile(currentScriptPath);
            if (currentFile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream out(&currentFile);
                out.setEncoding(QStringConverter::Utf8);
                out << m_pCurrentScriptEditor->getScriptContent();
                currentFile.close();
            }
        }

        // 同步销毁当前编辑器:直接 delete(不调 close(),避免 WA_DeleteOnClose 的延迟删除与此处同步 delete 并存)。
        // delete 会同步触发 destroyed 信号,使下方连接的清理槽在重新赋值前先把指针置空。
        delete m_pCurrentScriptEditor;
        m_pCurrentScriptEditor = nullptr;
        m_nCurrentScriptIndex = -1;
    }

    // 创建新的编辑器窗口，注入运行/检查 lambda
    ScriptEditor* pScriptEditor = new ScriptEditor(m_parentWidget);
    const int idx = scriptIndex - 1;
    ScriptEngineHost* host = m_host;
    pScriptEditor->setRunFn(
        [host, idx](const QString& content) {
            host->runScriptAsync(idx, content);
        });
    pScriptEditor->setCheckFn(
        [host](const QString& script, QString& err) {
            return host->checkScript(script, err);
        });
    pScriptEditor->setFunctionDocs(m_host->functionDocs());
    pScriptEditor->setAttribute(Qt::WA_DeleteOnClose);
    pScriptEditor->setWindowModality(Qt::ApplicationModal);
    pScriptEditor->setScriptName(scriptPath);

    // 加载文件内容
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        pScriptEditor->loadScript(in.readAll());
        file.close();
    }

    // 保存编辑器指针
    m_pCurrentScriptEditor = pScriptEditor;
    m_nCurrentScriptIndex = scriptIndex;

    // 连接关闭信号，清理指针
    connect(pScriptEditor, &QObject::destroyed, this, [this]() {
        m_pCurrentScriptEditor = nullptr;
        m_nCurrentScriptIndex = -1;
    });

    pScriptEditor->show();
}
