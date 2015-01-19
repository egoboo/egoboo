#include "modelwidget.h"
#include "md2/Md2Model.h"

#include <QMatrix4x4>
#include <QMouseEvent>

static const float kEgobooAnimationFPS = 10.0f;
static const float kMouseSensitivity = .4f;

ModelWidget::ModelWidget(QWidget *parent)
    :MyOpenGLWidget(parent), rotateX(-90), rotateZ(0), lastMousePos(),
      isDragging(false), model(NULL), texture(QOpenGLTexture::Target2D),
      frameList(), timer(this), frameLerp(0), currentFrame(0), nextFrame(0)
{
    MySurfaceFormat format;
    format.setProfile(MySurfaceFormat::CompatibilityProfile);
    setFormat(format);
    connect(&timer, &QTimer::timeout, this, &ModelWidget::tick);
}

ModelWidget::~ModelWidget()
{
    close();
}

void ModelWidget::initializeGL()
{
}

void ModelWidget::paintGL()
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, -15, -50);
    glRotatef(rotateX, 1, 0, 0);
    glRotatef(rotateZ, 0, 0, 1);

    if (model)
    {
        if (texture.isCreated())
        {
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            texture.bind();
        } else {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
        }

        if (frameList.count() >= 2) {
            int frame1 = frameList[currentFrame];
            int frame2 = frameList[nextFrame];
            model->drawBlendedFrames(frame1, frame2, frameLerp);
        } else if (frameList.count() == 1) {
            model->drawFrame(frameList[0]);
        } else {
            model->drawFrame(0);
        }
    }
}

void ModelWidget::resizeGL(int w, int h)
{
    float aspectRatio = w / static_cast<float>(h ? h : 1);
    QMatrix4x4 mat;
    mat.perspective(75, aspectRatio, 1, 1024);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(mat.data());
}

void ModelWidget::mousePressEvent(QMouseEvent *event)
{
    isDragging = true;
    lastMousePos = event->pos();
}

void ModelWidget::mouseReleaseEvent(QMouseEvent *)
{
    isDragging = false;
}

void ModelWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!isDragging) return;
    QPoint pos = event->pos();
    rotateX += (pos.y() - lastMousePos.y()) * kMouseSensitivity;
    rotateZ += (pos.x() - lastMousePos.x()) * kMouseSensitivity;
    lastMousePos = pos;
    update();
}

void ModelWidget::tick()
{
   if (frameList.count() < 2 || !model) return;

   frameLerp += timer.interval() / 1000.f * kEgobooAnimationFPS;
   if (frameLerp > 1.0f) {
       frameLerp -= 1.0f;
       currentFrame = nextFrame;
       nextFrame++;

       if (nextFrame >= frameList.count()) nextFrame = 0;
   }
   update();
}

void ModelWidget::setModelFrames(const QList<int> &frames)
{
    frameList = frames;
    currentFrame = 0;
    nextFrame = 1;
    frameLerp = 0.0f;
    update();
}

void ModelWidget::showModel(Md2Model *mod, QImage *skin)
{
    close();
    model = mod;
    if (!model)
        return;
    makeCurrent();
    if (skin)
        texture.setData(*skin);
    doneCurrent();
    timer.start(16);
    update();
}

void ModelWidget::changeSkin(QImage *skin)
{
    if (!model)
        return;
    makeCurrent();
    texture.destroy();
    if (skin)
        texture.setData(*skin);
    doneCurrent();
    update();
}

void ModelWidget::close()
{
    if (!model)
        return;
    makeCurrent();
    texture.destroy();
    doneCurrent();
    timer.stop();
    frameList.clear();
    rotateX = -90;
    rotateZ = 0;
    model = NULL;
}
