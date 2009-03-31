extern "C" typedef struct SDL_Surface SDL_Surface;
namespace Sirikata { namespace Graphics {
class SDLInputManager {
    SDL_Surface *mScreen;
public:
    SDLInputManager(unsigned int width,
                    unsigned int height,
                    bool fullscreen,
                    const Ogre::PixelFormat&fmt,
                    bool grabCursor);
    bool tick(Time currentTime, Duration frameTime);
    ~SDLInputManager();
};
} }
