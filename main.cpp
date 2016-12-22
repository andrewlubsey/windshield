#include "Capture.h"
#include "Compute.h"
#include "ImageView.h"
#include "Timer.h"

#include <GL/glew.h>
#include <GL/freeglut.h>


static VideoSource* cap = NULL;
static Compute* compute = NULL;

static int viewWidth, viewHeight;
static ImageView* view = NULL;

static const int refreshRate = 30;
static Timer timer;

static GLsync rendering = 0;

static void Display() {
    OutputData data;
    compute->SwapOutputData(data);
    if (! data.image.empty()) {
        view->Draw(data, viewWidth, viewHeight);

        glDeleteSync(rendering);
        rendering = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

        glutSwapBuffers();
    }
}

static void Refresh(int what) {
    if (rendering) {
        switch (glClientWaitSync(rendering, 0, 1000000000/refreshRate)) {
        case GL_ALREADY_SIGNALED:
        case GL_CONDITION_SATISFIED:
            glDeleteSync(rendering);
            rendering = 0;
            break;
        }
    }

    if (rendering) {
        glutTimerFunc(0, Refresh, what);
    } else {
        Display();
        glutTimerFunc(timer.NextSleep(refreshRate,INT_MAX)/1000, Refresh, what);
    }
}

static void Cleanup() {
    compute->Stop();
    cap->Stop();

    delete view; view = NULL;
    delete compute; compute = NULL;
    delete cap; cap = NULL;
    glDeleteSync(rendering); rendering = 0;
}

static void Init(int& argc, char**argv) {
    cap = new Capture(0);

    viewWidth  = cap->imageWidth;
    viewHeight = cap->imageHeight;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(viewWidth, viewHeight);
    glutCreateWindow("Windshield");
    glewInit();

    view = new ImageView(cap->imageWidth, cap->imageHeight);
    compute = new Compute(cap);

    cap->Start();
    compute->Start();

    glutTimerFunc(0, Refresh, 0);
    glutDisplayFunc(Display);
    glutCloseFunc(Cleanup);

    timer.Start();
}

int main(int argc, char**argv) {
    Init(argc, argv);
    glutMainLoop();
    return 0;
}
