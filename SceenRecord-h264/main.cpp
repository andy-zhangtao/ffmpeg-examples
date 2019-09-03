#include <iostream>
#include "ScreenRecord.h"

using namespace std;

int main() {
    int value = -1;
    ScreenRecord sr;
    value = sr.openDevice();
    if (value) {
        cout << "Open Device Error. err: " << value;
        exit(-1);
    }

    value = sr.init_outputfile();
    if (value) {
        cout << "Open OutFile Error. err: " << value;
        exit(-1);
    }

    value = sr.CaptureVideoFrames();
    if (value) {
        cout << "Capture Error. err: " << value;
        exit(-1);
    }
    return 0;
}