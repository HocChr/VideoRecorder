
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

    if (videoDefault->isOpened())
    {
        return true;
    }
    return false;
}

void VideoWriter::write()
{
    if (!video || !properties || properties->FRAME_RATE == 0)
        return;

    int sleepTime = (1000 / properties->FRAME_RATE);

    auto begin = std::chrono::steady_clock::now();

    while (doRecord)
    {
        std::unique_lock<std::mutex> recMutex(recordFinishedMutex);

        auto now = std::chrono::steady_clock::now();

        if (chrono::duration_cast<chrono::milliseconds>(now - begin).count() >= sleepTime)
        {
            std::unique_lock<std::mutex> lk(writeFrameMutex);
            begin = std::chrono::steady_clock::now();
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
    video = make_unique<cv::VideoWriter>(filePath, codec, properties->FRAME_RATE,
                                         cv::Size(properties->WIDTH, properties->HEIGHT), true);
    thread         = std::thread(&VideoWriter::write, this);
    recordFinished = false;
}

void VideoWriter::setFrame(const cv::Mat frameArg)
{
    std::lock_guard<std::mutex> l(writeFrameMutex);

    cv::cvtColor(frameArg, frame, cv::COLOR_BGR2RGB);

    if (!properties)
    {
        return;
    }
    if (!video)
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

    std::unique_lock<std::mutex> lk(recordFinishedMutex);
    condition.wait(lk, [this] { return recordFinished; });

    video->release();

    if (thread.joinable())
    {
        thread.detach();
    }
    cv::destroyAllWindows();
}
