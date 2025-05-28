#include <QApplication>
#include "yuvWidget.h"
#include <QFile>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    yuvWidget window;
    window.show();

    int w = 640, h = 480;
    int ySize = w * h;
    int uSize = ySize / 4;
    int vSize = ySize / 4;

    QFile file("test_yuv420p.yuv");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to open YUV file!");
        return 0;
    }

    QByteArray buf = file.readAll();
    file.close();

    if (buf.size() < ySize + uSize + vSize) {
        qWarning("YUV file is too small!");
        return 0;
    }

    // 分配内存并填充
    uint8_t *y = reinterpret_cast<uint8_t *>(buf.data());
    uint8_t *u = y + ySize;
    uint8_t *v = u + uSize;

    uint8_t *data[3] = {y, u, v};

    // 行跨度（stride）
    // Y 分量: 每行 w 个像素
    // U/V 分量: 每行 w/2 个像素，高度 h/2
    int linesize[3] = {w, w / 2, w / 2};

    window.setTextureYUVData(data, linesize, w, h);

    return QApplication::exec();
}
