#include <iostream>
#include "filter.h"

using namespace std;
int main() {
    int value = -1;
    filter sr;
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

    value = sr.init_filter();
    if (value) {
        cout << "Init Filter Graph Error. err: " << value;
        exit(-1);
    }

    value = sr.CaptureVideoFrames();
    if (value) {
        cout << "Capture Error. err: " << value;
        exit(-1);
    }
    return 0;
}