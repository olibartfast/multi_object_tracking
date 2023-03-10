#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include "YoloV5/YoloV5.hpp"
#include "SORT/KalmanTracker.hpp"
#include "SORT/Sort.hpp"

static const std::string params = "{ help h   |   | print help message }"
      "{ detector     |  yolov5x | mobilenet, svm, yolov4-tiny, yolov4, yolov5s, yolov5x | detector model}"
      "{ link l   |   | capture video from ip camera}"
      "{ labels lb   |  ../labels | path to class labels file}"
      "{ tracker tr   |  SORT | tracking algorithm}"
      "{ class cl   |  car | class from coco dataset to track}"
      "{ model_path mp   |  ../models | path to models}";



std::vector<std::string> readLabelNames(const std::string& fileName)
{
    if(!std::filesystem::exists(fileName)){
        std::cerr << "Wrong path to labels " <<  fileName << std::endl;
        exit(1);
    } 
    std::vector<std::string> classes;
    std::ifstream ifs(fileName.c_str());
    std::string line;
    while (getline(ifs, line))
    classes.push_back(line);
    return classes;   
}

std::unique_ptr<Detector> createDetector(
    const std::string& detectorType,
    const std::string& labelsPath,
    const std::string& modelPath){
    std::vector<std::string> classes; 
    std::string modelConfiguration; 
    std::string modelBinary;   
    if(detectorType.find("yolov5") != std::string::npos)  
    {
        std::string modelBinary;
        classes = readLabelNames(labelsPath);   
        return std::make_unique<YoloV5>(classes, "", modelPath);
    }
    return nullptr;
}     

std::unique_ptr<Tracker> createTracker(const std::string& trackingAlgorithm)
{
    if(trackingAlgorithm.find("SORT") != std::string::npos)  
    {   
        auto sortTracker = std::make_unique<Sort>();
        return sortTracker;
    }     
    return nullptr;
}

int main(int argc, char** argv) {

    cv::CommandLineParser parser(argc, argv, params);
    parser.about("Multi object tracker");
    if (parser.has("help")){
      parser.printMessage();
      return 0;  
    }

    const std::string modelPath = parser.get<std::string>("model_path");
    const std::string labelsPath = parser.get<std::string>("labels");
    const std::string detectorType = parser.get<std::string>("detector");
    const std::string trackingAlgorithm = parser.get<std::string>("tracker");
    const std::string classToTrack = parser.get<std::string>("class");
    const std::vector<std::string> track_classes = {classToTrack};

    std::vector<std::string> classes = readLabelNames(labelsPath);


    // Open video file
    cv::VideoCapture cap(parser.get<std::string>("link"));
    std::unique_ptr<Detector> detector = createDetector(detectorType, labelsPath, modelPath); 
    std::unique_ptr<Tracker> tracker = createTracker(trackingAlgorithm);

    if(!detector || !tracker)
        std::exit(1);

    // randomly generate colors, only for display
    cv::RNG rng(0xFFFFFFFF);
    cv::Scalar_<int> randColor[20];
    for (int i = 0; i < 20; i++)
        rng.fill(randColor[i], cv::RNG::UNIFORM, 0, 256);

    // Process video frames
    cv::Mat frame;
    while (cap.read(frame)) 
    {

        // Run multi-object tracker on frame
        std::vector<t_prediction> detections = detector->run_detection(frame);
        auto tracksOutput = tracker->track(detections, classes, track_classes);

        // show detection box in white
        for(auto detection_result : detections) 
        {
            cv::rectangle(frame, cv::Rect(detection_result.x, detection_result.y, detection_result.width, detection_result.height), cv::Scalar(255,255,255), 2, 8, 0);
            cv::putText(frame, classes[detection_result.class_index], cv::Point(detection_result.x + detection_result.width/2, detection_result.y-15), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255, 255, 255), 2, 8, 0);
        }

        // show tracking box in color
        for (auto tb : tracksOutput) {
            cv::rectangle(frame, tb.box, randColor[tb.id % 20], 2, 8, 0);
            cv::putText(frame, std::to_string(tb.id), cv::Point(tb.box.x, tb.box.y-15), cv::FONT_HERSHEY_PLAIN, 2, cv::Scalar(255, 255, 255), 2, 8, 0);
        }
        
        cv::imshow(trackingAlgorithm, frame);
        cv::waitKey(1);

    }

    return 0;
}
