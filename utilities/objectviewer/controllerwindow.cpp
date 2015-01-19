#include "controllerwindow.h"
#include "ui_controllerwindow.h"

#include "md2/Md2Model.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

ControllerWindow::ControllerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ControllerWindow),
    skins(),
    model(NULL)
{
    ui->setupUi(this);
    connect(ui->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(ui->actionQuit, &QAction::triggered, qApp, &QApplication::closeAllWindows);

    QAction *seperator = ui->menuOpenRecent->actions()[0];
    for (size_t i = 0; i < MAX_NUM_OF_RECENT_FILES; i++)
    {
        QAction *curr = new QAction(this);
        curr->setVisible(false);
        connect(curr, &QAction::triggered, this, &ControllerWindow::openRecentFile);
        ui->menuOpenRecent->insertAction(seperator, curr);
        recentList[i] = curr;
    }
    updateRecentActions();
}

ControllerWindow::~ControllerWindow()
{
    delete ui;
}

void ControllerWindow::showWindow()
{
    activateWindow();
}

void ControllerWindow::showAbout()
{
    QMessageBox::about(this, "About EOV-qt", "this does things, version old");
}

void ControllerWindow::doMinimize()
{
    if (isMinimized())
        showNormal();
    else
        showMinimized();
}

void ControllerWindow::doZoom()
{
    if (isMaximized())
        showNormal();
    else
        showMaximized();
}

void ControllerWindow::closeFile()
{
    if (!model)
        return;
    ui->openGLWidget->close();
    delete model;
    foreach (QImage *image, skins) {
        delete image;
    }
    skins.clear();
    ui->comboBox->clear();
    ui->listWidget->clear();
    model = NULL;
}

void ControllerWindow::openFile()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Open Egoboo object");
    doOpen(dir);
}

void ControllerWindow::doOpen(const QString &fileName)
{
    static QStringList extensions;
    if (extensions.isEmpty())
    {
        extensions.append("png");
        extensions.append("bmp");
    }
    closeFile();
    QFile modelFile(fileName + "/tris.md2");
    if (!modelFile.exists())
        return;
    modelFile.open(QIODevice::ReadOnly);
    model = Md2Model::loadFromFile(&modelFile);
    if (!model)
        return;
    for (int i = 0; i < 4; i++)
    {
        foreach (QString ext, extensions)
        {
            QFile file(QString("%1/tris%2.%3").arg(fileName).arg(i).arg(ext));
            if (file.exists())
            {
                QImage *img = new QImage(file.fileName());
                if (img)
                {
                    skins.append(img);
                    ui->comboBox->addItem(QString("Skin %1 (%2)").arg(i).arg(ext));
                }
                continue;
            }
        }
    }
    for (int i = 0; i < model->getNumFrames(); i++)
    {
        ui->listWidget->addItem(model->frameAtIndex(i)->name);
    }
    ui->openGLWidget->showModel(model, skins.isEmpty() ? NULL : skins[0]);
    setWindowFilePath(fileName);
    addRecentFile(fileName);
}

void ControllerWindow::skinChanged(int skin)
{
    if (skins.isEmpty())
        return;
    ui->openGLWidget->changeSkin(skins[skin]);
}

void ControllerWindow::frameListChanged()
{
    QList<int> list;
    QList<QListWidgetItem *> items = ui->listWidget->selectedItems();
    foreach (QListWidgetItem *item, items)
    {
        list.append(ui->listWidget->row(item));
    }
    ui->openGLWidget->setModelFrames(list);
}

void ControllerWindow::addRecentFile(const QString &fileName)
{
    QSettings settings;
    QStringList recentFiles = settings.value("recentList").toStringList();
    recentFiles.removeAll(fileName);
    recentFiles.prepend(fileName);
    while (recentFiles.count() > MAX_NUM_OF_RECENT_FILES)
        recentFiles.removeLast();
    settings.setValue("recentList", recentFiles);
    updateRecentActions();
}

void ControllerWindow::updateRecentActions()
{
    QSettings settings;
    QStringList recentFiles = settings.value("recentList").toStringList();
    for (size_t i = 0; i < MAX_NUM_OF_RECENT_FILES; i++)
    {
        bool isVisible = i < recentFiles.count();
        QAction *action = recentList[i];
        if (isVisible)
        {
            QFileInfo fi(recentFiles[i]);
            action->setData(recentFiles[i]);
            action->setText(fi.fileName());
        }
        action->setVisible(isVisible);
    }
    ui->actionClearRecent->setEnabled(recentFiles.count() > 0);
}

void ControllerWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        doOpen(action->data().toString());
}

void ControllerWindow::clearRecentList()
{
    QSettings settings;
    settings.setValue("recentList", QStringList());
    updateRecentActions();
}
