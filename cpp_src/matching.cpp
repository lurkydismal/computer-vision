///////////////
/// @file opencv.cpp
/// @brief \c MatchingMethod and \c MatchingMethodWindow definition.
///////////////
#ifdef __WIN32__
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <fmt/core.h>

#include <stdexcept>
#include <thread>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <iterator>

//! <b>[define]</b>
/// @code{.cpp}
#define RESULT_WINDOW_NAME "Result window"
/// @endcode
//! <b>[define]</b>

#ifdef __WIN32__
///////////////
/// @brief Get cv::Mat object from window capture.
/// @param[in] _sourceWindowName Window handle to capture from.
/// @return Window capture.
///////////////
static cv::Mat getMatFromWindow( std::string _sourceWindowName ) {
    HWND l_sourceWindowHandle = FindWindow( NULL, _sourceWindowName.c_str() );
    //! <b>[declare]</b>
    /// Local variables.
    /// @code{.cpp}
    cv::Mat l_sourceImage;
    BITMAPINFOHEADER l_bitmapInfo;
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[window_info]</b>
    /// Get window parameters.
    /// @code{.cpp}
    HDC l_handleWindowDeviceContext           = GetDC( l_sourceWindowHandle );
    HDC l_handleWindowCompatibleDeviceContext = CreateCompatibleDC( l_handleWindowDeviceContext );

    SetStretchBltMode(
        l_handleWindowCompatibleDeviceContext,
        COLORONCOLOR
    );

    RECT l_windowSize;

    GetClientRect(
        l_sourceWindowHandle,
        &l_windowSize
    );

    unsigned int l_sourceHeight = l_windowSize.bottom;
    unsigned int l_sourceWidth  = l_windowSize.right;
    unsigned int l_strechHeight = ( l_windowSize.bottom / 1 );
    unsigned int l_strechWidth  = ( l_windowSize.right / 1 );
    /// @endcode
    //! <b>[window_info]</b>

    //! <b>[window_capture]</b>
    /// Create empty image.
    /// @code{.cpp}
    l_sourceImage.create(
        l_strechHeight,
        l_strechWidth,
        CV_8UC4
    );
    /// @endcode
    //! <b>[window_capture]</b>

    //! <b>[bitmap]</b>
    /// Create and assign values.
    /// @code{.cpp}
    HBITMAP l_handleBitmapWindow = CreateCompatibleBitmap(
        l_handleWindowDeviceContext,
        l_strechWidth,
        l_strechHeight
    );

    l_bitmapInfo.biSize          = sizeof( BITMAPINFOHEADER );
    l_bitmapInfo.biWidth         = l_strechWidth;
    l_bitmapInfo.biHeight        = -l_strechHeight; // This is the line that makes it draw upside down or not
    l_bitmapInfo.biPlanes        = 1;
    l_bitmapInfo.biBitCount      = 32;
    l_bitmapInfo.biCompression   = BI_RGB;
    l_bitmapInfo.biSizeImage     = 0;
    l_bitmapInfo.biXPelsPerMeter = 0;
    l_bitmapInfo.biYPelsPerMeter = 0;
    l_bitmapInfo.biClrUsed       = 0;
    l_bitmapInfo.biClrImportant  = 0;
    /// @endcode
    //! <b>[bitmap]</b>

    //! <b>[window_capture]</b>
    /// Use the previously created device context with the bitmap.
    /// @code{.cpp}
    SelectObject(
        l_handleWindowCompatibleDeviceContext,
        l_handleBitmapWindow
    );
    /// @endcode

    /// Copy from the window device context to the bitmap device context. Change SRCCOPY to NOTSRCCOPY for wacky colors.
    /// @code{.cpp}
    StretchBlt(
        l_handleWindowCompatibleDeviceContext,
        0,
        0,
        l_strechWidth,
        l_strechHeight,
        l_handleWindowDeviceContext,
        0,
        0,
        l_sourceWidth,
        l_sourceHeight,
        SRCCOPY
    );
    /// @endcode

    /// Copy from l_handleWindowCompatibleDeviceContext to l_handleBitmapWindow.
    /// @code{.cpp}
    GetDIBits(
        l_handleWindowCompatibleDeviceContext,
        l_handleBitmapWindow,
        0,
        l_strechHeight,
        l_sourceImage.data,
        (BITMAPINFO*)&l_bitmapInfo,
        DIB_RGB_COLORS
    );
    /// @endcode
    //! <b>[window_capture]</b>

    //! <b>[clean]</b>
    /// Avoid memory leak.
    /// @code{.cpp}
    DeleteObject( l_handleBitmapWindow );
    DeleteDC( l_handleWindowCompatibleDeviceContext );
    ReleaseDC(
        l_sourceWindowHandle,
        l_handleWindowDeviceContext
    );
    /// @endcode
    //! <b>[clean]</b>

    //! <b>[color]</b>
    /// Convert source image to template's color format.
    /// @code{.cpp}
    cv::Mat t_l_image;

    cv::cvtColor(
        l_sourceImage,
        t_l_image,
        cv::COLOR_RGB2BGR
    );
    /// @endcode
    //! <b>[color]</b>

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( t_l_image );
    /// @endcode
    //! <b>[return]</b>
}

#else

static Window windowSearch(
    Display*    _display,
    Window      _defaultWindow,
    std::string _windowName
) {
    Window l_window = 0;
    Window l_root;
    Window l_parent;
    Window* l_children;
    uint32_t l_childrenCount;
    char* l_name = NULL;

    if ( XFetchName( _display, _defaultWindow, &l_name ) ) {
        bool l_isNeedle = ( _windowName == l_name );

        XFree( l_name );

        if ( l_isNeedle ) {
            return ( _defaultWindow );
        }
    }

    if (
        XQueryTree(
            _display,
            _defaultWindow,
            &l_root,
            &l_parent,
            &l_children,
            &l_childrenCount
        )
    ) {
        for ( unsigned i = 0; i < l_childrenCount; ++i ) {
            l_window = windowSearch( _display, l_children[ i ], _windowName );

            if ( l_window ) {
                break;
            }
        }

        XFree( l_children );
    }

    return ( l_window );
}

static Window getWindowCapture( std::string _windowName ) {
    Display* l_display = XOpenDisplay( NULL );

    Window l_window = windowSearch(
        l_display,
        XDefaultRootWindow( l_display ),
        _windowName
    );

    XCloseDisplay( l_display );

    return ( l_window );
}

static cv::Mat getMatFromWindow(
    std::string    _sourceWindowName,
    const uint32_t _captureWidth  = ( 800 >> 0 ),
    const uint32_t _captureHeight = ( 600 >> 0 )
) {

    Display* l_display = XOpenDisplay( NULL );
    Window l_root = getWindowCapture( _sourceWindowName );
    XWindowAttributes l_windowAttributes;

    XGetWindowAttributes(
        l_display,
        l_root,
        &l_windowAttributes
    );

    Screen* l_screen = l_windowAttributes.screen;
    XShmSegmentInfo l_shminfo;

    XImage* l_xImage = XShmCreateImage(
        l_display,
        DefaultVisualOfScreen( l_screen ),
        DefaultDepthOfScreen( l_screen ),
        ZPixmap,
        NULL,
        &l_shminfo,
        _captureWidth,
        _captureHeight
    );

    l_shminfo.shmid = shmget(
        IPC_PRIVATE,
        ( l_xImage->bytes_per_line * l_xImage->height ),
        ( IPC_CREAT | 0777 )
    );
    l_shminfo.shmaddr = l_xImage->data = (char*)shmat( l_shminfo.shmid, 0, 0 );
    l_shminfo.readOnly = false;

    if ( !l_shminfo.shmid ) {
        puts("Fatal shminfo error!");
    }

    XShmAttach( l_display, &l_shminfo );

    XShmGetImage(
        l_display,
        l_root,
        l_xImage,
        0,
        0,
        0x00ffffff
    );

    cv::Mat l_image = cv::Mat(
        _captureHeight,
        _captureWidth,
        CV_8UC4,
        l_xImage->data
    );

    //! <b>[color]</b>
    /// Convert source image to template's color format.
    /// @code{.cpp}
    cv::Mat t_l_image;

    cv::cvtColor(
        l_image,
        t_l_image,
        cv::COLOR_RGB2BGR
    );
    /// @endcode
    //! <b>[color]</b>

    XShmDetach( l_display, &l_shminfo );
    XDestroyImage( l_xImage );
    shmdt( l_shminfo.shmaddr );
    XCloseDisplay( l_display );

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( t_l_image );
    /// @endcode
    //! <b>[return]</b>
}

#endif

///////////////
/// @brief Compares a template against overlapped image regions.
/// @param[in] _matchMethod Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _image 2D image array where the search is running. It must be 8-bit or 32-bit floating-point.
/// @param[in] _templateImages Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _showResult Will print out squares of found images to other window.
/// @param[in] _imageDisplay 2D image array with printed rectangles of found images.
/// @param[in] _templateMap Map of comparison results. Throws ios_base::failure at error.
///////////////
static void MatchTemplates(
    unsigned int _matchMethod,
    cv::Mat _image,
    std::vector< std::string > _templateImages,
    const bool _showResult,
    cv::Mat& _imageDisplay,
    std::map< std::string, std::array< unsigned int, 2 > >& _templateMap
) {
    //! <b>[check_image]</b>
    /// Return FAILED if 2D image array is empty.
    /// @code{.cpp}
    if ( _image.empty() ) {
        throw std::ios_base::failure( "Can't read source image" );
    }
    /// @endcode
    //! <b>[check_image]</b>

    //! <b>[copy_source]</b>
    /// Source image to display.
    /// @code{.cpp}
    _image.copyTo( _imageDisplay );
    /// @endcode
    //! <b>[copy_source]</b>

    //! <b>[create_window]</b>
    /// Create window if needed.
    /// @code{.cpp}
    if ( _showResult ) {
        cv::namedWindow( RESULT_WINDOW_NAME, cv::WINDOW_AUTOSIZE );
    }
    /// @endcode
    //! <b>[create_window]</b>

    auto MatchTemplate = [ & ]( std::string _templateImage ) {
        //! <b>[declare]</b>
        /// 2D image array for result.
        /// @code{.cpp}
        cv::Mat l_resultImage;
        /// @endcode
        //! <b>[declare]</b>

        //! <b>[load_template]</b>
        /// Load template image.
        /// @code{.cpp}
        cv::Mat l_templateImage = cv::imread( _templateImage, cv::IMREAD_COLOR );

        if ( l_templateImage.empty() ) {
            fmt::print( "Can't read template image {}\n", _templateImage );

            return ( cv::Point( 0, 0 ) );
        }
        /// @endcode
        //! <b>[load_template]</b>

        //! <b>[create_result_array]</b>
        /// Create the result 2D image array.
        /// @code{.cpp}
        l_resultImage.create(
            ( _image.rows - l_templateImage.rows + 1 ),
            ( _image.cols - l_templateImage.cols + 1 ),
            CV_32FC1
        );
        /// @endcode
        //! <b>[create_result_array]</b>

        //! <b>[match_template]</b>
        /// Do Matching.
        /// @code{.cpp}
        cv::matchTemplate(
            _image,          // Source
            l_templateImage, // Destination
            l_resultImage,
            _matchMethod
        );
        /// @endcode
        //! <b>[match_template]</b>

        //! <b>[normalize]</b>
        /// Do Normalize.
        /// @code{.cpp}
        cv::normalize(
            l_resultImage, // Source
            l_resultImage, // Destination
            0,
            1,
            cv::NORM_MINMAX,
            -1,
            cv::Mat()
        );
        /// @endcode
        //! <b>[normalize]</b>

        //! <b>[best_match]</b>
        /// Localizing the best match with minMaxLoc.
        /// @code{.cpp}
        double l_minimumValue;
        double l_maximumValue;
        cv::Point l_minimumLocation;
        cv::Point l_maximumLocation;
        cv::Point l_matchLocation;

        cv::minMaxLoc(
            l_resultImage,
            &l_minimumValue,
            &l_maximumValue,
            &l_minimumLocation,
            &l_maximumLocation,
            cv::Mat()
        );
        /// @endcode
        //! <b>[best_match]</b>

        //! <b>[match_loc]</b>
        /// For SQDIFF and SQDIFF_NORMED, the best matches are lower values. For all the other methods, the higher the better.
        /// @code{.cpp}
        if ( ( _matchMethod  == cv::TM_SQDIFF ) || ( _matchMethod == cv::TM_SQDIFF_NORMED ) ) {
            l_matchLocation = l_minimumLocation;

        } else {
            l_matchLocation = l_maximumLocation;
        }
        /// @endcode
        //! <b>[match_loc]</b>

        //! <b>[draw_rectangles]</b>
        /// Draw rectangles on images.
        /// @code{.cpp}
        cv::rectangle(
            _imageDisplay,
            l_matchLocation,
            cv::Point(
                ( l_matchLocation.x + l_templateImage.cols ),
                ( l_matchLocation.y + l_templateImage.rows )
            ),
            cv::Scalar::all( 0 ),
            2,
            8,
            0
        );
        /// @endcode
        //! <b>[draw_rectangles]</b>

        //! <b>[return]</b>
        /// End of lambda.
        /// @code{.cpp}
        return (
            cv::Point(
                l_matchLocation.x + ( l_templateImage.cols / 2 ),
                l_matchLocation.y + ( l_templateImage.rows / 2 )
            )
        );
        /// @endcode
        //! <b>[return]</b>
    };

    //! <b>[match_templates]</b>
    /// Matching all templates on source image and generating map of template's coordinates.
    /// @code{.cpp}
    std::vector< std::thread > l_matchTemplateThreads;
    cv::Point l_matchedLocation;

    for ( auto _templateImage : _templateImages ) {
        l_matchTemplateThreads.push_back(
            std::thread( [ &, _templateImage ]{
                l_matchedLocation = MatchTemplate( _templateImage );

                _templateMap[ _templateImage ] = {
                    static_cast< unsigned int >( l_matchedLocation.x ),
                    static_cast< unsigned int >( l_matchedLocation.y )
                };
            } )
        );
    }

    for ( unsigned int matchTemplateThread = 0; matchTemplateThread < l_matchTemplateThreads.size(); matchTemplateThread++ ) {
        l_matchTemplateThreads[ matchTemplateThread ].join();
    }
    /// @endcode
    //! <b>[match_templates]</b>
}

///////////////
/// @brief Compares a template against overlapped image regions.
/// @param[in] _matchMethod Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _sourceImage Image where the search is running. It must be 8-bit or 32-bit floating-point.
/// @param[in] _templateImages Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _showResult Will print out squares of found images to other window.
/// @return Map of comparison results. Throws ios_base::failure at error.
///////////////
extern "C" std::map< std::string, std::array< unsigned int, 2 > > MatchingMethod(
    unsigned int _matchMethod,
    char* _sourceImage,
    std::vector< std::string > _templateImages,
    const bool _showResult
) {
    //! <b>[load_image]</b>
    /// Load image.
    /// @code{.cpp}
    cv::Mat l_image = cv::imread( _sourceImage, cv::IMREAD_COLOR );
    /// @endcode
    //! <b>[load_image]</b>

    //! <b>[match]</b>
    /// Match template images on source image.
    /// @code{.cpp}
    cv::Mat l_imageDisplay;
    std::map< std::string, std::array< unsigned int, 2 > > l_templateMap;

    MatchTemplates(
        _matchMethod,
        l_image,
        _templateImages,
        _showResult,
        l_imageDisplay,
        l_templateMap
    );

    if ( l_imageDisplay.empty() ) {
        throw std::ios_base::failure( "Can't read source image" );
    }
    /// @endcode
    //! <b>[match]</b>

    //! <b>[imshow]</b>
    /// Show me what you got.
    /// @code{.cpp}
    if ( _showResult ) {
        cv::imshow( RESULT_WINDOW_NAME, l_imageDisplay );
        cv::waitKey( 30 );
    }
    /// @endcode
    //! <b>[imshow]</b>

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( l_templateMap );
    /// @endcode
    //! <b>[return]</b>
}

///////////////
/// @brief Compares a template against overlapped image regions.
/// @param[in] _matchMethod Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _sourceWindowName Window where the search is running.
/// @param[in] _templateImages Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _showResult Will print out squares of found images to other window.
/// @return Map of comparison results. Check the "0" field for errors.
///////////////
std::map < std::string, std::array< unsigned int, 2 > > MatchingMethodWindow(
    unsigned int _matchMethod,
    char* _sourceWindowName,
    std::vector< std::string > _templateImages,
    const bool _showResult
) {
    //! <b>[load_image]</b>
    /// Get window capture.
    /// @code{.cpp}
    cv::Mat l_image = getMatFromWindow( _sourceWindowName );
    /// @endcode
    //! <b>[load_image]</b>

    //! <b>[match]</b>
    /// Match template images on source image.
    /// @code{.cpp}
    cv::Mat l_imageDisplay;
    std::map< std::string, std::array< unsigned int, 2 > > l_templateMap;

    MatchTemplates(
        _matchMethod,
        l_image,
        _templateImages,
        _showResult,
        l_imageDisplay,
        l_templateMap );

    if ( l_imageDisplay.empty() ) {
        throw std::ios_base::failure( "Can't read source image" );
    }
    /// @endcode
    //! <b>[match]</b>

    //! <b>[imshow]</b>
    /// Show me what you got.
    /// @code{.cpp}
    cv::Mat t_l_image;
    cv::cvtColor(
        l_imageDisplay,
        t_l_image,
        cv::COLOR_BGR2RGB
    );

    if ( _showResult ) {
        cv::imshow( RESULT_WINDOW_NAME, t_l_image );
        cv::waitKey( 30 );
    }
    /// @endcode
    //! <b>[imshow]</b>

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( l_templateMap );
    /// @endcode
    //! <b>[return]</b>
}

// R CMD SHLIB -c cpp_src/matching.cpp && mv cpp_src/matching.so .
