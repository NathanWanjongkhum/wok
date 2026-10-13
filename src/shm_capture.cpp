#include "shm_capture.h"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdexcept>

ShmCapture::ShmCapture(Display *display, int max_width, int max_height) : display_(display)
{
    if (!XShmQueryExtension(display_))
    {
        throw std::runtime_error("X server does not support the MIT-SHM extension");
    }

    int screen = DefaultScreen(display_);
    Visual *visual = DefaultVisual(display_, screen);
    int depth = DefaultDepth(display_, screen);

    image_ = XShmCreateImage(display_, visual, depth, ZPixmap, nullptr, &shm_info_, max_width, max_height);
    if (image_ == nullptr)
    {
        throw std::runtime_error("XShmCreateImage failed");
    }

    if (image_->bits_per_pixel != 32)
    {
        XDestroyImage(image_);
        throw std::runtime_error("Unsupported X visual: expected 32 bits per pixel");
    }
    bytes_per_pixel_ = image_->bits_per_pixel / 8;

    shm_info_.shmid = shmget(IPC_PRIVATE, static_cast<size_t>(image_->bytes_per_line) * image_->height,
                              IPC_CREAT | 0600);
    if (shm_info_.shmid < 0)
    {
        XDestroyImage(image_);
        throw std::runtime_error("shmget failed");
    }

    shm_info_.shmaddr = static_cast<char *>(shmat(shm_info_.shmid, nullptr, 0));
    if (shm_info_.shmaddr == reinterpret_cast<char *>(-1))
    {
        shmctl(shm_info_.shmid, IPC_RMID, nullptr);
        XDestroyImage(image_);
        throw std::runtime_error("shmat failed");
    }
    image_->data = shm_info_.shmaddr;
    shm_info_.readOnly = False;

    if (!XShmAttach(display_, &shm_info_))
    {
        shmdt(shm_info_.shmaddr);
        shmctl(shm_info_.shmid, IPC_RMID, nullptr);
        XDestroyImage(image_);
        throw std::runtime_error("XShmAttach failed");
    }

    // XShmAttach only queues the attach request; the server's shmat() runs
    // later, asynchronously. IPC_RMID blocks any *new* shmat() on Linux, so
    // without this XSync the server can lose the race and fail with
    // BadAccess. Sync first to be sure it has actually attached, then mark
    // the segment for removal — the kernel keeps it alive until every
    // attach (ours, and the server's) is released, then reclaims it for us.
    XSync(display_, False);
    shmctl(shm_info_.shmid, IPC_RMID, nullptr);
}

ShmCapture::~ShmCapture()
{
    if (image_ != nullptr)
    {
        XShmDetach(display_, &shm_info_);
        // Prevent XDestroyImage from XFree()-ing shm-owned memory.
        image_->data = nullptr;
        XDestroyImage(image_);
        shmdt(shm_info_.shmaddr);
    }
}

cv::Mat ShmCapture::capture(Drawable drawable, int x, int y, int w, int h)
{
    // Temporarily shrink the shared XImage to the requested region; the
    // backing buffer is sized for the constructor's max dimensions, so any
    // w <= max_width, h <= max_height fits without reallocating.
    image_->width = w;
    image_->height = h;
    image_->bytes_per_line = w * bytes_per_pixel_;

    if (!XShmGetImage(display_, drawable, image_, x, y, AllPlanes))
    {
        return cv::Mat();
    }

    cv::Mat view(h, w, CV_8UC4, image_->data, static_cast<size_t>(image_->bytes_per_line));
    return view.clone(); // own the data; the shm buffer gets reused on the next capture
}
