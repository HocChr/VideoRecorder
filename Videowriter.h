
#ifndef VIDEOWRITER_H
#define VIDEOWRITER_H

// Includes
//----------------------------------------------------------------------------------------------------------------------
#include <opencv2/core/mat.hpp>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

// Forward declarations
//----------------------------------------------------------------------------------------------------------------------
namespace cv
{
class VideoWriter;
}

//-Video Properties------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// \brief  These are the Video-Properties for Video-Recording
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct VideoProperties
{
    int FRAME_RATE{0};
    int WIDTH{0};
    int HEIGHT{0};

    bool operator == (const VideoProperties& rhs);
    bool operator != (const VideoProperties& rhs);
    VideoProperties& operator = (VideoProperties rhs);

};

//-VideoWriter-----------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// \brief  This is the Video Writer for writing videos
///
/// Howto use it?
/// 1.  Its recommended to make an instance as a (smart-)pointer and
///     specify the path where the video shall be stored
/// 2.  Set Video-Properties
/// 3.  Setting the first frame runs the videowriter
/// 4.  To stop the videowriter, destroy the pointer/instance.
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class VideoWriter
{
public:
    VideoWriter(const std::string &filePath);

    bool isStateValid();

    void setFrame(const cv::Mat frame);

    const VideoProperties *getProperties() const;

    void setProperties(VideoProperties &properties);

    virtual ~VideoWriter();

    VideoWriter() = delete;

private:
    //-Private functions---------------------------------------------

    void write();

    void initAndStartRecording();

    //-Private data--------------------------------------------------

    std::string filePath;
    cv::Mat frame;

    std::unique_ptr<VideoProperties> properties;
    std::unique_ptr<cv::VideoWriter> video;

    int codec{0};

    //-Variables for threading---------------------------------------

    mutable std::mutex writeFrameMutex;
    mutable std::mutex recordFinishedMutex;

    std::thread thread;
    std::condition_variable condition;

    std::atomic<bool> doRecord{true};
    std::atomic<bool> writeFrame{false};
    bool recordFinished{false};
};

#endif // VIDEOWRITER_H
