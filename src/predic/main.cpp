#include <iostream>
#include "ProgramOption.h"
#include "ABoostDetection.h"
#include "Classification.h"
#include "libSVM.h"
#include "extractor.h"
#include <algorithm>
using namespace std;
using namespace cv;


class Classify {

private:

    struct svm_model *ModelClasificator;

public:

    void load_modelLibClass(const string model);


    double detekuj(vector<float> value);

    double detekuj_prob(vector<float> value);


    vector<float> extractHog(Rect_<int> &vector, Mat mat);
};


void Classify::load_modelLibClass(const string model) {
    cout << "Loading model for LibSVM...." << endl;


    if ((this->ModelClasificator = libSVM::getInstance()->loadModelFromFile(model)) == 0) {
        cerr << "Can't load SVM model " << model << endl;
    }
}


bool InitModels(ABoostDetection *p_adet) {

    if (!p_adet->LoadModel()) {
        cerr << "ERROR: " << "Loading  detector model of adaboost:" << endl;
        return false;
    }
    cout << "Model Adaboost is loaded" << endl;


    /*
    if (!pClassification->LoadModel()){
        cerr << "ERROR: " << "Loading  classificator model of svm" << endl;
        return false;
    }
    cout << "Model SVM is loaded" << endl;
*/
    return true;
}


bool InitVideoCapture(VideoCapture *pCapture, int input_cam) {

    pCapture->open(input_cam);
    return pCapture->isOpened();
}

bool InitVideoCapture(VideoCapture *pCapture, string input_file) {

    pCapture->open(input_file);
    return pCapture->isOpened();
}

const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

int main(int argc, char **argv) {

    // vytvoreni konstruktoru a prace se vstupem
    ProgramOption *opt = new ProgramOption(argc, argv);

    // nacteni a kontrola vstunich dat
    vector<string> list_input;
    list_input = opt->GetVectorOfFiles();

    // vytvoreni konruktoru ori aboost detektor
    ABoostDetection *adet = new ABoostDetection(opt->GetPathModelAboost());

    //Classification *classify = new Classification(opt->GetPathModelSVM());

    //nacteni model
    if (!InitModels(adet)) {
        delete opt;
        delete adet;
        return EXIT_FAILURE;
    }


    Classify *classif = new Classify();
    classif->load_modelLibClass(opt->GetPathModelSVM());

    Classification *clasifon = new Classification("model");

    clasifon->LoadImagesIcon();

    vector<Mat> classif_icon = clasifon->GetImgIcon();

    time_t start, end;

    // fps calculated using number of frames / seconds
    double fps = 0;

    // frame counter
    int counter = 0;

    // floating point seconds elapsed since start
    double sec;


    VideoCapture capture;
    Mat frame;
    Mat original;
    Mat small_image;
    int cont = 84;
    string WindowName;

    if (opt->GetMode() == 1) {
        WindowName = "Camera";
        if (!InitVideoCapture(&capture, opt->GetCameraRun())) {
            cerr << "RROOR load camera: " << opt->GetCameraRun() << endl;

            delete opt;
            delete adet;
            return EXIT_FAILURE;
        }

        // start the clock
        time(&start);
        cont += 1;

        while (capture.read(frame)) {
            if (frame.empty()) {
                printf(" --(!) No captured frame -- Break!");
                break;
            }

            resize(frame, frame, Size(FRAME_WIDTH, FRAME_HEIGHT), INTER_CUBIC);
            original = frame.clone();

            std::vector<Rect> sign;
            std::vector<float> descriptors;

            sign = adet->Detection(frame, fps);


            if (!sign.empty() && !opt->GetModelClassif()) {

                for (int i = 0; i < sign.size(); ++i) {

                    descriptors = classif->extractHog(sign.at(i), original);

                    //cout<<classif->detekuj(descriptors);

                    Mat cropedImage = original(Rect(sign.at(i).x, sign.at(i).y, sign.at(i).width, sign.at(i).height));

                    string pat = "/tmp/aa/" + to_string(cont) + "-" + to_string(counter) + ".jpg";
                    imwrite(pat, cropedImage);


                    double index = classif->detekuj_prob(descriptors);
                    cout << index;

                    Mat small_image = imread(LUT_image_filename[abs((int) index) - 1]);
                    resize(small_image, small_image, Size(32, 32), INTER_CUBIC);


                    try {

                        small_image.copyTo(
                                frame(cv::Rect(sign.at(i).x, sign.at(i).y, small_image.cols, small_image.rows)));
                    }
                    catch (int e) {
                        cerr << "Ohraniceni" << endl;
                    }

                }
            }

            if (opt->GetModeShow()) {

                namedWindow(WindowName, WINDOW_AUTOSIZE);
                imshow(WindowName, frame);
                waitKey(1);

                //imshow("window_name", frame);
                //waitKey(1);
            }


            // see how much time has elapsed
            time(&end);
            // calculate current FPS
            ++counter;
            sec = difftime(end, start);

            fps = counter / sec;

            // overflow protection
            if (counter == (INT_MAX - 1000))
                counter = 0;
        }
    } else if (opt->GetMode() == 2) {
        WindowName = "Video";

        for (int i = 0; i < list_input.size(); i++) {
            cout << list_input.at(i) << ":" << list_input.size() << endl;
            if (!InitVideoCapture(&capture, list_input.at(i))) {
                cerr << "RROOR load file: " << list_input.at(i) << endl;

                delete opt;
                delete adet;
                return EXIT_FAILURE;
            }

            cont += 1;
            string pat = "/tmp/" + to_string(i) + ".avi";
            //VideoWriter video(pat, CV_FOURCC('D', 'I', 'V', 'X'), capture.get(CV_CAP_PROP_FPS), Size(640, 480), true);

            // start the clock
            counter = 0;
            fps = 0;
            double t, td, tc, te;

            while (capture.read(frame)) {
                t = (double) getTickCount();

                if (frame.empty()) {
                    printf(" --(!) No captured frame -- Break!");
                    break;
                }


                resize(frame, frame, Size(FRAME_WIDTH, FRAME_HEIGHT), INTER_CUBIC);
                original = frame.clone();

                std::vector<Rect> sign;
                std::vector<float> descriptors;

                sign = adet->Detection(frame, fps);

                if (!sign.empty() && !opt->GetModelClassif()) {
                    for (int j = 0; j < sign.size(); ++j) {

                        Mat cropedImage = original(Rect(sign.at(j).x, sign.at(j).y, sign.at(j).width, sign.at(j).height));
                        imwrite("/tmp/aa/" + to_string(counter) + ".jpg", cropedImage);

                        td = (double) getTickCount();

                        descriptors = classif->extractHog(sign.at(j), original);
                        td = ((double) getTickCount() - td) / getTickFrequency();

                        tc = (double) getTickCount();

                        double index = classif->detekuj_prob(descriptors);
                        tc = ((double) getTickCount() - tc) / getTickFrequency();

                        cout << index << endl;

                        te = (double) getTickCount();
                        if (clasifon->CheckRoi(sign.at(j).x, sign.at(j).height)) {
                            small_image = classif_icon.at(abs((int) index) - 1);
                            resize(small_image, small_image, Size(sign.at(j).height, sign.at(j).width), INTER_CUBIC);
                            small_image.copyTo(
                                    frame(cv::Rect(sign.at(j).x - sign.at(j).height, sign.at(j).y, small_image.cols,
                                                   small_image.rows)));
                            small_image.release();
                        }
                        te = ((double) getTickCount() - te) / getTickFrequency();


                    }
                }
                else{
                    td = tc = te = 0;
                }

                if (opt->GetModeShow()) {

                    namedWindow(WindowName, WINDOW_AUTOSIZE);
                    imshow(WindowName, frame);
                    waitKey(1);

                }

                //video.write(frame);


                //sign.clear();
                descriptors.clear();
                frame.release();
                original.release();

                t = ((double) getTickCount() - t) / getTickFrequency();

                cout << "QQ:" << t << ":" << 1 / t << ":" << td << ":" << tc <<":" <<te<< endl;

                fps = 1 / t;
                counter++;
            }


            capture.release();
            //video.release();


        }
    }
    else {
        WindowName = "Image";
        for (int i = 0; i < list_input.size(); ++i) {
            cout << list_input.at(i) << endl;

            frame = imread(list_input.at(i));

            // start the clock
            time(&start);
            counter = 0;
            std::vector<Rect> sign;
            std::vector<float> descriptors;

            sign = adet->Detection(frame, fps);

            if (!sign.empty() && !opt->GetModelClassif()) {

                for (int i = 0; i < sign.size(); ++i) {

                    descriptors = classif->extractHog(sign.at(i), original);

                    //cout<<classif->detekuj(descriptors);

                    Mat cropedImage = original(
                            Rect(sign.at(i).x, sign.at(i).y, sign.at(i).width, sign.at(i).height));

                    string pat = "/tmp/aa/" + to_string(cont) + "-" + to_string(counter) + ".jpg";
                    imwrite(pat, cropedImage);


                    double index = classif->detekuj_prob(descriptors);
                    cout << index;

                    Mat small_image = imread(LUT_image_filename[abs((int) index) - 1]);
                    resize(small_image, small_image, Size(32, 32), INTER_CUBIC);


                    try {

                        small_image.copyTo(
                                frame(cv::Rect(sign.at(i).x, sign.at(i).y, small_image.cols, small_image.rows)));
                    }
                    catch (int e) {
                        cerr << "Ohraniceni" << endl;
                    }

                }
            }


            if (opt->GetModeShow()) {

                namedWindow(WindowName, WINDOW_AUTOSIZE);
                imshow(WindowName, frame);
                waitKey(1);
            }


            // see how much time has elapsed
            time(&end);
            // calculate current FPS
            ++counter;
            sec = difftime(end, start);

            fps = counter / sec;

            // overflow protection
            if (counter == (INT_MAX - 1000))
                counter = 0;


        }
    }


    delete opt;
    delete adet;

    return 0;
}


vector<float> Classify::extractHog(Rect_<int> &faces, Mat mat) {
    Mat cropedImage = mat(Rect(faces.x, faces.y, faces.width, faces.height));

    std::vector<float> descriptors;
    descriptors = getHog(cropedImage);
    cropedImage.release();

    return descriptors;
}

double Classify::detekuj(vector<float> value) {

    struct svm_node *svmVecT;
    svmVecT = (struct svm_node *) malloc((value.size() + 1) * sizeof(struct svm_node));
    int i;
    for (i = 0; i < value.size(); i++) {
        svmVecT[i].index = i + 1;
        svmVecT[i].value = value.at(i);
    }

    svmVecT[i].index = -1;   // End of line

    double result = svm_predict(this->ModelClasificator, svmVecT);
    return result;
}

double Classify::detekuj_prob(vector<float> value) {


    struct svm_node *svmVecT;
    svmVecT = (struct svm_node *) malloc((value.size() + 1) * sizeof(struct svm_node));


    int i;
    for (i = 0; i < value.size(); i++) {
        svmVecT[i].index = i + 1;
        svmVecT[i].value = value.at(i);
    }

    svmVecT[i].index = -1;   // End of line

    double prob[45];

    double td = (double) getTickCount();
    double result = svm_predict_probability(this->ModelClasificator, svmVecT, prob);
    td = ((double) getTickCount() - td) / getTickFrequency();
    cout<<value.size() + 1<<":"<<td<<endl;

    //sort(prob, prob+45);

    cerr<<result<<":"<<prob[int (abs(result) +1)]<<endl;
    /*
    for (size_t i = 0; i != 45; ++i)
        cerr << prob[i] << " ";
    cerr<<endl;
*/
    free( svmVecT);
    return result;
}
