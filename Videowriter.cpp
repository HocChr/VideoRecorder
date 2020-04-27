
// own header
#include "Videowriter.h"

// system includes
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace std;

//-Video Properties------------------------------------------------------------

bool VideoProperties::operator==(const VideoProperties &rhs)
{
    return this->WIDTH == rhs.WIDTH && this->HEIGHT == rhs.HEIGHT && this->FRAME_RATE == rhs.FRAME_RATE;
}

bool VideoProperties::operator!=(const VideoProperties &rhs)
{
    return !(*this == rhs);
}

VideoProperties &VideoProperties::operator=(VideoProperties rhs)
{
    std::swap(WIDTH, rhs.WIDTH);
    std::swap(HEIGHT, rhs.HEIGHT);
    std::swap(FRAME_RATE, rhs.FRAME_RATE);
    return *this;
}

//-VideoWriter-----------------------------------------------------------------

VideoWriter::VideoWriter(const string &filePathArg)
    : filePath{filePathArg}
{
    codec = cv::VideoWriter::fourcc('m', 'p', '4', 'V');
}

bool VideoWriter::isStateValid()
{
    auto videoDefault = make_unique<cv::VideoWriter>(filePath, codec, 20, cv::Size(100, 100), true);

    return videoDefault->isOpened() ? true : false;
}

void VideoWriter::write()
{
    if (!video || !properties || properties->FRAME_RATE == 0)
    {
        return;
    }

    std::unique_lock<std::mutex> recMutex(recordFinishedMutex); // #2 look at the end of file

    while (doRecord)
    {
        if(writeFrame)
        {
            std::lock_guard<std::mutex> lockguard(writeFrameMutex); // #1 look at the end of file
            writeFrame = false;
            if (video->isOpened())
            {
                video->write(frame);
            }
        }
    }

    recordFinished = true;
    condition.notify_one();
}

void VideoWriter::initAndStartRecording()
{
  video = make_unique<cv::VideoWriter>(
      filePath, codec, properties->FRAME_RATE,
      cv::Size(properties->WIDTH, properties->HEIGHT), true);
  thread = std::thread(&VideoWriter::write, this);
  recordFinished = false;
}

void VideoWriter::setFrame(const cv::Mat frameArg)
{
    std::lock_guard<std::mutex> lockguard(writeFrameMutex); // #1 look at the end of file

    cv::cvtColor(frameArg, frame, cv::COLOR_BGR2RGB);
    writeFrame = true;

    bool success = properties != nullptr ? true : false;
    if (success && video == nullptr)
    {
        initAndStartRecording();
    }
}

const VideoProperties *VideoWriter::getProperties() const
{
    return properties.get();
}

void VideoWriter::setProperties(VideoProperties &propertiesArg)
{
    properties = make_unique<VideoProperties>(propertiesArg);
}

VideoWriter::~VideoWriter()
{
    if (!video)
    {
        return;
    }

    doRecord = false;

    std::unique_lock<std::mutex> lockguard(recordFinishedMutex); // #2 look at the end of file
    condition.wait(lockguard, [this] { return recordFinished; });

    video->release();

    if (thread.joinable())
    {
        thread.detach();
    }
    cv::destroyAllWindows();
}

// #1 use std::lock_guard<> if neither functionality for de-locking nor functionality for waiting is needed
// #2 use std::unique_lock<> because the wait-functionality is required
