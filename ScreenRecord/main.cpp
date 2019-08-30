#include <iostream>
#include "ScreenRecorder.h"

using namespace std;

int main() {

    ScreenRecorder screen_record;

    screen_record.openDevice();

    cout << "open device success \n";

    screen_record.init_outputfile();

    cout << "init output file success \n";

    screen_record.CaptureVideoFrames();
    return 0;
}