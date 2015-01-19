#ifndef MODELWIDGET_H
#define MODELWIDGET_H

#include <QOpenGLTexture>
#include <QList>
#include <QTimer>

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
#include <QOpenGLWidget>
typedef QOpenGLWidget MyOpenGLWidget;
typedef QSurfaceFormat MySurfaceFormat;
#else
#include <QGLWidget>
typedef QGLWidget MyOpenGLWidget;
typedef QGLFormat MySurfaceFormat;
#endif

class Md2Model;

class ModelWidget : public MyOpenGLWidget
{
    Q_OBJECT
public:
    explicit ModelWidget(QWidget *parent = 0);
    ~ModelWidget();

    void showModel(Md2Model *model, QImage *skin);
    void changeSkin(QImage *skin);
    void close();

    void setModelFrames(const QList<int> &frames);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public slots:
    void tick();

private:
    float rotateX, rotateZ;

    QPoint lastMousePos;
    bool isDragging;

    Md2Model *model;
    QOpenGLTexture texture;

    QList<int> frameList;

    QTimer timer;
    float frameLerp;
    int currentFrame;
    int nextFrame;
};

#endif // MODELWIDGET_H
