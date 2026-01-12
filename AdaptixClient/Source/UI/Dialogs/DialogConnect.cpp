#include <UI/Dialogs/DialogConnect.h>
#include <Client/AuthProfile.h>
#include <Client/Storage.h>
#include <MainAdaptix.h>
#include <Utils/NonBlockingDialogs.h>
#include <QVBoxLayout>
#include <QFile>
#include <QIODevice>
#include <QFrame>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>

static QString defaultProjectDir(const QString &projectName)
{
    return QDir::home().filePath("AdaptixProjects/" + projectName.trimmed());
}

bool DialogConnect::parseUrl(QString &host, QString &port, QString &endpoint) const
{
    QUrl url(lineEdit_Url->text().trimmed());
    if (!url.isValid() || url.host().isEmpty())
        return false;

    host = url.host();
    port = url.port(-1) != -1 ? QString::number(url.port()) : "443";
    endpoint = url.path().isEmpty() ? "/" : url.path();
    return true;
}

QString DialogConnect::buildUrl(const QString &host, const QString &port, const QString &endpoint) const
{
    QUrl url;
    url.setScheme("https");
    url.setHost(host);
    if (!port.isEmpty() && port != "443")
        url.setPort(port.toInt());
    url.setPath(endpoint.isEmpty() ? "/" : endpoint);
    return url.toString();
}

DialogConnect::DialogConnect()
{
    createUI();
    loadProjects();

    connect(cardWidget,          &QListWidget::itemPressed,                this, &DialogConnect::onProfileSelected);
    connect(cardWidget,          &QListWidget::customContextMenuRequested, this, &DialogConnect::handleContextMenu);
    connect(buttonNewProfile,    &QPushButton::clicked,                    this, &DialogConnect::onButton_NewProfile);
    connect(buttonLoad,          &QPushButton::clicked,                    this, &DialogConnect::onButton_Load);
    connect(buttonSave,          &QPushButton::clicked,                    this, &DialogConnect::onButton_Save);
    connect(lineEdit_Project,    &QLineEdit::textChanged,                  this, &DialogConnect::onProjectNameChanged);
    connect(lineEdit_ProjectDir, &QLineEdit::textEdited,                   this, &DialogConnect::onProjectDirEdited);
    connect(buttonConnect,       &QPushButton::clicked,                    this, &DialogConnect::onButton_Connect);

    auto connectReturnPressed = [this](QLineEdit* edit) {
        connect(edit, &QLineEdit::returnPressed, this, &DialogConnect::onButton_Connect);
    };
    connectReturnPressed(lineEdit_Project);
    connectReturnPressed(lineEdit_Url);
    connectReturnPressed(lineEdit_User);
    connectReturnPressed(lineEdit_Password);

    auto action = lineEdit_ProjectDir->addAction(QIcon(":/icons/folder"), QLineEdit::TrailingPosition);
    connect(action, &QAction::triggered, this, &DialogConnect::onSelectProjectDir);
}

DialogConnect::~DialogConnect() = default;

void DialogConnect::createUI()
{
    resize(720, 420);
    setFixedSize(720, 420);
    setWindowTitle("连接");
    setProperty("Main", "base");

    groupUserInfo = new QGroupBox("用户信息", this);
    auto userLayout = new QGridLayout(groupUserInfo);
    userLayout->setContentsMargins(10, 10, 10, 10);
    userLayout->setHorizontalSpacing(12);
    userLayout->setVerticalSpacing(8);

    label_User = new QLabel("用户名:", this);
    label_Password = new QLabel("密码:", this);

    lineEdit_User = new QLineEdit(this);
    lineEdit_User->setToolTip("输入用户名");

    lineEdit_Password = new QLineEdit(this);
    lineEdit_Password->setEchoMode(QLineEdit::Password);
    lineEdit_Password->setToolTip("输入密码");

    userLayout->addWidget(label_User,        0, 0);
    userLayout->addWidget(lineEdit_User,     0, 1);
    userLayout->addWidget(label_Password,    1, 0);
    userLayout->addWidget(lineEdit_Password, 1, 1);
    userLayout->setColumnMinimumWidth(0, 100);

    groupServerDetails = new QGroupBox("服务器详情", this);
    auto serverLayout = new QGridLayout(groupServerDetails);
    serverLayout->setContentsMargins(10, 10, 10, 10);
    serverLayout->setHorizontalSpacing(12);
    serverLayout->setVerticalSpacing(8);

    label_Url = new QLabel("地址:", this);

    lineEdit_Url = new QLineEdit(this);
    lineEdit_Url->setPlaceholderText("https://address:4321/endpoint");
    lineEdit_Url->setToolTip("输入服务器地址 (例如: https://host:port/endpoint)");

    serverLayout->addWidget(label_Url,     0, 0);
    serverLayout->addWidget(lineEdit_Url,  0, 1);
    serverLayout->setColumnMinimumWidth(0, 100);

    groupProject = new QGroupBox("项目", this);
    auto projectLayout = new QGridLayout(groupProject);
    projectLayout->setContentsMargins(10, 10, 10, 10);
    projectLayout->setHorizontalSpacing(12);
    projectLayout->setVerticalSpacing(8);

    label_Project = new QLabel("名称:", this);
    label_ProjectDir = new QLabel("目录:", this);

    lineEdit_Project = new QLineEdit(this);
    lineEdit_Project->setToolTip("输入项目名称");

    lineEdit_ProjectDir = new QLineEdit(this);
    lineEdit_ProjectDir->setToolTip("输入项目目录路径（留空则自动生成）");

    projectLayout->addWidget(label_Project,      0, 0);
    projectLayout->addWidget(lineEdit_Project,   0, 1);
    projectLayout->addWidget(label_ProjectDir,   1, 0);
    projectLayout->addWidget(lineEdit_ProjectDir,1, 1);
    projectLayout->setColumnMinimumWidth(0, 100);

    buttonConnect = new QPushButton(this);
    buttonConnect->setProperty("ButtonStyle", "dialog_apply");
    buttonConnect->setText("连接");
    buttonConnect->setFixedWidth(160);
    buttonConnect->setFocus();

    auto actionButtonsLayout = new QHBoxLayout();
    actionButtonsLayout->addStretch();
    actionButtonsLayout->addWidget(buttonConnect);
    actionButtonsLayout->addStretch();

    auto formLayout = new QVBoxLayout();
    formLayout->setContentsMargins(10, 10, 10, 10);
    formLayout->setSpacing(10);
    formLayout->addWidget(groupUserInfo);
    formLayout->addWidget(groupServerDetails);
    formLayout->addWidget(groupProject);
    formLayout->addStretch(1);
    formLayout->addLayout(actionButtonsLayout);

    auto formWidget = new QWidget(this);
    formWidget->setLayout(formLayout);

    menuContext = new QMenu(this);
    menuContext->addAction("删除", this, &DialogConnect::itemRemove);

    label_Profiles = new QLabel(this);
    label_Profiles->setAlignment(Qt::AlignCenter);
    label_Profiles->setText("配置文件");

    cardWidget = new CardListWidget(this);
    cardWidget->setFixedWidth(240);
    cardWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    cardWidget->addAction(menuContext->menuAction());
    cardWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    cardWidget->setFocusPolicy(Qt::NoFocus);

    buttonNewProfile = new QPushButton(this);
    buttonNewProfile->setProperty("ButtonStyle", "dialog");
    buttonNewProfile->setText("新建配置");
    buttonNewProfile->setMinimumSize(QSize(10, 30));

    buttonLoad = new QPushButton(QIcon(":/icons/file_open"), "", this);
    buttonLoad->setProperty("ButtonStyle", "dialog");
    buttonLoad->setIconSize(QSize(20, 20));
    buttonLoad->setFixedSize(QSize(30, 30));
    buttonLoad->setToolTip("从文件加载配置");

    buttonSave = new QPushButton(QIcon(":/icons/save_as"), "", this);
    buttonSave->setProperty("ButtonStyle", "dialog");
    buttonSave->setIconSize(QSize(20, 20));
    buttonSave->setFixedSize(QSize(30, 30));
    buttonSave->setToolTip("保存配置到文件");

    auto profileButtonsLayout = new QHBoxLayout();
    profileButtonsLayout->addWidget(buttonNewProfile);
    profileButtonsLayout->addWidget(buttonLoad);
    profileButtonsLayout->addWidget(buttonSave);
    profileButtonsLayout->setSpacing(5);
    profileButtonsLayout->setContentsMargins(0, 0, 0, 0);

    auto profileButtonsWidget = new QWidget(this);
    profileButtonsWidget->setLayout(profileButtonsLayout);

    auto separatorLine = new QFrame(this);
    separatorLine->setFrameShape(QFrame::VLine);
    separatorLine->setFrameShadow(QFrame::Sunken);
    separatorLine->setStyleSheet("QFrame { color: rgba(100, 100, 100, 50); background-color: rgba(100, 100, 100, 50); }");

    gridLayout = new QGridLayout(this);
    gridLayout->setContentsMargins(5, 5, 5, 5);
    gridLayout->addWidget(formWidget,           0, 0, 3, 1);
    gridLayout->addWidget(separatorLine,        0, 1, 3, 1);
    gridLayout->addWidget(label_Profiles,       0, 2, 1, 1);
    gridLayout->addWidget(cardWidget,           1, 2, 1, 1);
    gridLayout->addWidget(profileButtonsWidget, 2, 2, 1, 1);

    gridLayout->setRowStretch( 0, 0 );
    gridLayout->setRowStretch( 1, 1 );
    gridLayout->setRowStretch( 2, 0 );
    gridLayout->setColumnStretch( 0, 1 );
    gridLayout->setColumnStretch( 1, 0 );
    gridLayout->setColumnStretch( 2, 0 );
    gridLayout->setColumnStretch( 3, 0 );
}

void DialogConnect::loadProjects()
{
    cardWidget->clear();
    listProjects = GlobalClient->storage->ListProjects();
    for (auto& profile : listProjects) {
        QString subtitle = profile.GetUsername() + " @ " + profile.GetHost();
        cardWidget->addCard(profile.GetProject(), subtitle);
    }
}

AuthProfile* DialogConnect::StartDialog()
{
    toConnect = false;
    exec();
    if (!toConnect)
        return nullptr;

    QString projectDir = lineEdit_ProjectDir->text().trimmed();
    if (projectDir.isEmpty())
        projectDir = defaultProjectDir(lineEdit_Project->text());

    QString host, port, endpoint;
    parseUrl(host, port, endpoint);

    auto* newProfile = new AuthProfile(lineEdit_Project->text(), lineEdit_User->text(), lineEdit_Password->text(), host, port, endpoint, projectDir);

    if (GlobalClient->storage->ExistsProject(lineEdit_Project->text()))
        GlobalClient->storage->UpdateProject(*newProfile);
    else
        GlobalClient->storage->AddProject(*newProfile);

    return newProfile;
}

void DialogConnect::itemRemove()
{
    auto* item = cardWidget->currentItem();
    if (!item)
        return;

    QString project = item->data(CardListWidget::TitleRole).toString();
    Storage::RemoveAllListenerProfiles(project);
    Storage::RemoveAllAgentProfiles(project);
    GlobalClient->storage->RemoveProject(project);
    delete cardWidget->takeItem(cardWidget->row(item));
    loadProjects();
}

void DialogConnect::onProfileSelected()
{
    auto* item = cardWidget->currentItem();
    if (!item)
        return;

    const QString project = item->data(CardListWidget::TitleRole).toString();
    isNewProject = false;
    projectDirTouched = true;

    for (auto& p : listProjects) {
        if (p.GetProject() == project) {
            lineEdit_Project->setText(p.GetProject());
            lineEdit_ProjectDir->setText(p.GetProjectDir());
            lineEdit_Url->setText(buildUrl(p.GetHost(), p.GetPort(), p.GetEndpoint()));
            lineEdit_User->setText(p.GetUsername());
            lineEdit_Password->setText(p.GetPassword());
            return;
        }
    }
}

void DialogConnect::handleContextMenu(const QPoint &pos)
{
    QPoint globalPos = cardWidget->mapToGlobal( pos );
    menuContext->exec( globalPos );
}

bool DialogConnect::checkValidInput() const
{
    const auto checkEmpty = [](QLineEdit* edit, const QString& msg) {
        if (edit->text().isEmpty()) {
            MessageError(msg);
            return false;
        }
        return true;
    };

    if (!checkEmpty(lineEdit_Project, "项目名不能为空")) return false;
    if (!checkEmpty(lineEdit_Url, "地址不能为空")) return false;
    if (!checkEmpty(lineEdit_User, "用户名不能为空")) return false;
    if (!checkEmpty(lineEdit_Password, "密码不能为空")) return false;

    QString host, port, endpoint;
    if (!parseUrl(host, port, endpoint)) {
        MessageError("地址格式错误（示例: https://host:port/endpoint）");
        return false;
    }

    if (GlobalClient->storage->ExistsProject(lineEdit_Project->text()) && isNewProject) {
        MessageError("项目已存在");
        return false;
    }

    return true;
}

void DialogConnect::onButton_Connect()
{
    if (checkValidInput()) {
        toConnect = true;
        close();
    }
}

void DialogConnect::clearFields()
{
    lineEdit_User->clear();
    lineEdit_Password->clear();
    lineEdit_Project->clear();
    lineEdit_ProjectDir->clear();
    lineEdit_Url->clear();
    cardWidget->clearSelection();
    lineEdit_Project->setFocus();
}

void DialogConnect::onProjectNameChanged(const QString &text)
{
    if (projectDirTouched)
        return;

    const QString name = text.trimmed();
    lineEdit_ProjectDir->setText(name.isEmpty() ? QString() : defaultProjectDir(name));
}

void DialogConnect::onProjectDirEdited(const QString &) { projectDirTouched = true; }

void DialogConnect::onSelectProjectDir()
{
    QString current = lineEdit_ProjectDir->text().trimmed();
    if (current.isEmpty())
        current = defaultProjectDir(lineEdit_Project->text());

    QString dir = QFileDialog::getExistingDirectory(this, "选择项目目录", current);
    if (!dir.isEmpty()) {
        lineEdit_ProjectDir->setText(dir);
        projectDirTouched = true;
    }
}

void DialogConnect::onButton_NewProfile()
{
    isNewProject = true;
    projectDirTouched = false;
    clearFields();
}

void DialogConnect::onButton_Load()
{
    QString baseDir = QDir::home().filePath("AdaptixProjects");
    QDir(baseDir).mkpath(".");

    QString projectDir = lineEdit_ProjectDir->text().trimmed();
    if (!projectDir.isEmpty() && QDir(projectDir).exists())
        baseDir = projectDir;

    NonBlockingDialogs::getOpenFileName(this, "加载配置", baseDir, "Adaptix 配置文件 (*.adaptixProfile)",
        [this](const QString& filePath) {
            if (filePath.isEmpty())
                return;

            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                MessageError("打开文件失败");
                return;
            }

            QJsonParseError parseError;
            QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
            file.close();

            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                MessageError("JSON 解析错误");
                return;
            }

            QJsonObject json = document.object();
            const QStringList requiredFields = {"project", "host", "port", "endpoint", "username", "password"};
            for (const QString& field : requiredFields) {
                if (!json.contains(field) || !json[field].isString()) {
                    MessageError(QString("缺少必需参数 '%1'").arg(field));
                    return;
                }
            }

            const QString project = json["project"].toString();
            isNewProject = !GlobalClient->storage->ExistsProject(project);
            lineEdit_Project->setText(project);

            QString projectDirFinal;
            if (json.contains("projectDir") && json["projectDir"].isString()) {
                projectDirFinal = json["projectDir"].toString();
                projectDirTouched = true;
            } else {
                projectDirFinal = defaultProjectDir(project);
                projectDirTouched = false;
            }
            lineEdit_ProjectDir->setText(projectDirFinal);

            lineEdit_User->setText(json["username"].toString());
            lineEdit_Password->setText(json["password"].toString());
            lineEdit_Url->setText(buildUrl(json["host"].toString(), json["port"].toString(), json["endpoint"].toString()));

            AuthProfile loadedProfile(project, json["username"].toString(), json["password"].toString(), json["host"].toString(), json["port"].toString(), json["endpoint"].toString(), projectDirFinal);

            if (isNewProject)
                GlobalClient->storage->AddProject(loadedProfile);
            else
                GlobalClient->storage->UpdateProject(loadedProfile);

            loadProjects();

            for (int i = 0; i < cardWidget->count(); ++i) {
                if (cardWidget->item(i)->data(CardListWidget::TitleRole).toString() == project) {
                    cardWidget->setCurrentRow(i);
                    onProfileSelected();
                    break;
                }
            }
        });
}

void DialogConnect::onButton_Save()
{
    if (!checkValidInput())
        return;

    const QString projectName = lineEdit_Project->text().trimmed();
    QString projectDir = lineEdit_ProjectDir->text().trimmed();
    if (projectDir.isEmpty())
        projectDir = defaultProjectDir(projectName);

    QString host, port, endpoint;
    parseUrl(host, port, endpoint);

    QJsonObject json;
    json["project"] = projectName;
    json["host"] = host;
    json["port"] = port;
    json["endpoint"] = endpoint;
    json["username"] = lineEdit_User->text().trimmed();
    json["password"] = lineEdit_Password->text();
    json["projectDir"] = projectDir;

    QString baseDir = QDir::homePath();
    QString projectDirText = lineEdit_ProjectDir->text().trimmed();
    if (!projectDirText.isEmpty())
        baseDir = projectDirText;

    QString initialPath = QDir(baseDir).filePath(projectName + ".adaptixProfile");

    NonBlockingDialogs::getSaveFileName(this, "保存配置", initialPath, "Adaptix 配置文件 (*.adaptixProfile)",
        [json](const QString& filePath) {
            if (filePath.isEmpty())
                return;

            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly)) {
                MessageError("打开文件失败");
                return;
            }

            file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
            file.close();
            MessageSuccess("配置保存成功");
        });
}