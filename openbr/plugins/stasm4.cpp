#include <stasm_lib.h>
#include <opencv2/objdetect/objdetect.hpp>
#include "openbr_internal.h"

using namespace cv;

namespace br
{

class CascadeResourceMaker : public ResourceMaker<CascadeClassifier>
{
    QString file;

public:
    CascadeResourceMaker(const QString &model)
    {
        file = Globals->sdkPath + "/share/openbr/models/";
        if      (model == "Ear")         file += "haarcascades/haarcascade_ear.xml";
        else if (model == "Eye")         file += "haarcascades/haarcascade_eye_tree_eyeglasses.xml";
        else if (model == "FrontalFace") file += "haarcascades/haarcascade_frontalface_alt2.xml";
        else if (model == "ProfileFace") file += "haarcascades/haarcascade_profileface.xml";
        else                             qFatal("Invalid model.");
    }

private:
    CascadeClassifier *make() const
    {
        CascadeClassifier *cascade = new CascadeClassifier();
        if (!cascade->load(file.toStdString()))
            qFatal("Failed to load: %s", qPrintable(file));
        return cascade;
    }
};

/*!
 * \ingroup initializers
 * \brief Initialize Stasm
 * \author Scott Klum \cite sklum
 */
class StasmInitializer : public Initializer
{
    Q_OBJECT

    void initialize() const
    {
        Globals->abbreviations.insert("RectFromStasmEyes","RectFromPoints([29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47],0.125,6.0)+Resize(44,164)");
        Globals->abbreviations.insert("RectFromStasmBrow","RectFromPoints([17, 18, 19, 20, 21, 22, 23, 24],0.15,6)+Resize(28,132)");
        Globals->abbreviations.insert("RectFromStasmNose","RectFromPoints([48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58],0.15,1.25)+Resize(44,44)");
        Globals->abbreviations.insert("RectFromStasmMouth","RectFromPoints([59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76],0.3,3.0)+Resize(28,68)");
    }
};

BR_REGISTER(Initializer, StasmInitializer)

/*!
 * \ingroup transforms
 * \brief Wraps STASM key point detector
 * \author Scott Klum \cite sklum
 */
class StasmTransform : public UntrainableTransform
{
    Q_OBJECT

    //QList<ASM_MODEL> models;
    mutable QMutex mutex;

    Resource<CascadeClassifier> cascadeResource;

    void init()
    {
        if (!stasm_init(qPrintable(Globals->sdkPath + "/share/openbr/models/stasm"), 0)) qFatal("Failed to initalize stasm.");
        cascadeResource.setResourceMaker(new CascadeResourceMaker("FrontalFace"));
    }

    void project(const Template &src, Template &dst) const
    {
        QMutexLocker locker(&mutex);

        CascadeClassifier *cascade = cascadeResource.acquire();

        int foundface;
        float landmarks[2 * stasm_NLANDMARKS]; // x,y coords (note the 2)
        stasm_search_single(&foundface, landmarks, reinterpret_cast<const char*>(src.m().data), src.m().cols, src.m().rows, *cascade, NULL, NULL);

        cascadeResource.release(cascade);

        if (!foundface)
             qDebug() << "No face found in " << qPrintable("/Users/scottklum/facesketchid/data/img/" + src.file.path() + "/" + src.file.fileName());

        for (int i = 0; i < stasm_NLANDMARKS; i++) {
            dst.file.appendPoint(QPointF(landmarks[2 * i], landmarks[2 * i + 1]));
        }

        dst.m() = src.m();
    }
};

BR_REGISTER(Transform, StasmTransform)

} // namespace br

#include "stasm4.moc"
