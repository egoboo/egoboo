#ifndef CONTROLLERWINDOW_H
#define CONTROLLERWINDOW_H

#include <QMainWindow>
#include <QList>

namespace Ui {
class ControllerWindow;
}

class Md2Model;

class ControllerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ControllerWindow(QWidget *parent = 0);
    ~ControllerWindow();

public slots:
    void openFile();
    void closeFile();
    void doMinimize();
    void doZoom();
    void showAbout();
    void showWindow();

    void openRecentFile();
    void clearRecentList();

    void skinChanged(int);
    void frameListChanged();

private:
    void doOpen(const QString &fileName);
    void addRecentFile(const QString &fileName);
    void updateRecentActions();

    Ui::ControllerWindow *ui;
    static const size_t MAX_NUM_OF_RECENT_FILES = 10;
    QAction *recentList[MAX_NUM_OF_RECENT_FILES];

    QList<QImage *> skins;
    Md2Model *model;
};

#endif // CONTROLLERWINDOW_H
