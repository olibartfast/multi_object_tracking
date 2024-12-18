#pragma once
#include <vector>
#include <opencv2/core/types.hpp>
#include "Detection.hpp"
#include "TrackedObject.hpp"
#include "TrackConfig.hpp"

class BaseTracker {
public:
    virtual ~BaseTracker() = default;
    virtual std::vector<TrackedObject> update(const std::vector<Detection>& detections, const cv::Mat &frame = cv::Mat()) = 0;
};

