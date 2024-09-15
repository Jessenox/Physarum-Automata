#include <iostream>
#include <libfreenect.hpp>
#include <pthread.h>
#include <vector>
#include <mutex>

using namespace std;

class MyFreenectDevice : public Freenect::FreenectDevice {
public:
    MyFreenectDevice(freenect_context *ctx, int index)
        : Freenect::FreenectDevice(ctx, index), buffer_video(freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB).bytes), new_frame(false) {
        // Inicializa el buffer de video y el estado del frame
    }

    // Callback que se ejecuta cuando se captura un nuevo frame de video
    void VideoCallback(void* _rgb, uint32_t timestamp) override {
        std::lock_guard<std::mutex> lock(mutex);
        uint8_t* rgb = static_cast<uint8_t*>(_rgb);
        std::copy(rgb, rgb + getVideoBufferSize(), buffer_video.begin());
        new_frame = true;
    }

    // Función para obtener el frame RGB y preparar el buffer para la transmisión
    bool getRGBFrame(std::vector<uint8_t>& output) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!new_frame) {
            return false;
        }

        // Copiar los datos del buffer al output
        output = buffer_video;
        new_frame = false;
        return true;
    }

private:
    std::vector<uint8_t> buffer_video; // Buffer de datos del video
    std::mutex mutex;                  // Control de concurrencia
    bool new_frame;                    // Indica si hay un nuevo frame
};

int main() {
    Freenect::Freenect freenect;
    MyFreenectDevice& device = freenect.createDevice<MyFreenectDevice>(0);

    // Iniciar la captura de video desde el Kinect
    device.startVideo();

    std::vector<uint8_t> frame_data;
    while (true) {
        if (device.getRGBFrame(frame_data)) {
            // Enviar los datos de video (RGB24) a stdout para que ffmpeg los procese
            fwrite(frame_data.data(), 1, frame_data.size(), stdout);
            fflush(stdout); // Asegurarse de que los datos se envíen inmediatamente
        }
    }

    // Detener la captura de video cuando se salga del bucle
    device.stopVideo();
    return 0;
}
