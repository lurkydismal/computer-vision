///////////////
/// @file opencv.cpp
/// @brief \c MatchingMethod and \c MatchingMethodWindow definition.
///////////////

#include <opencv5/opencv2/imgcodecs.hpp>
#include <opencv5/opencv2/highgui.hpp>
#include <opencv5/opencv2/imgproc.hpp>
#include <thread>
#include <fmt/core.h>

#include <cstring>
#include <array>
#include <vector>
#include <map>
#include <iterator>

//! <b>[define]</b>
/// @code{.cpp}
#define RESULT_WINDOW_NAME "Result window"
/// @endcode
//! <b>[define]</b>

///////////////
/// @brief Get cv::Mat object from window capture.
/// @param[in] _sourceWindowHandle Window handle to capture from.
/// @return Window capture.
///////////////
// HWND
static cv::Mat getMatFromWindow( uint32_t _sourceWindowHandle ) {
    #ifdef __WIN32__
    //! <b>[declare]</b>
    /// Local variables.
    /// @code{.cpp}
    cv::Mat l_sourceImage;
    BITMAPINFOHEADER bitmapInfo;
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[window_info]</b>
    /// Get window parameters.
    /// @code{.cpp}
    HDC l_handleWindowDeviceContext           = GetDC( _sourceWindowHandle );
    HDC l_handleWindowCompatibleDeviceContext = CreateCompatibleDC( l_handleWindowDeviceContext );

    SetStretchBltMode(
        l_handleWindowCompatibleDeviceContext,
        COLORONCOLOR
    );

    RECT l_windowSize;
    GetClientRect(
        _sourceWindowHandle,
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
    HBITMAP handleBitmapWindow = CreateCompatibleBitmap(
        l_handleWindowDeviceContext,
        l_strechWidth,
        l_strechHeight
    );

    bitmapInfo.biSize          = sizeof( BITMAPINFOHEADER );
    bitmapInfo.biWidth         = l_strechWidth;
    bitmapInfo.biHeight        = -l_strechHeight; // This is the line that makes it draw upside down or not
    bitmapInfo.biPlanes        = 1;
    bitmapInfo.biBitCount      = 32;
    bitmapInfo.biCompression   = BI_RGB;
    bitmapInfo.biSizeImage     = 0;
    bitmapInfo.biXPelsPerMeter = 0;
    bitmapInfo.biYPelsPerMeter = 0;
    bitmapInfo.biClrUsed       = 0;
    bitmapInfo.biClrImportant  = 0;
    /// @endcode
    //! <b>[bitmap]</b>

    //! <b>[window_capture]</b>
    /// Use the previously created device context with the bitmap.
    /// @code{.cpp}
    SelectObject(
        l_handleWindowCompatibleDeviceContext,
        handleBitmapWindow
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

    /// Copy from l_handleWindowCompatibleDeviceContext to handleBitmapWindow.
    /// @code{.cpp}
    GetDIBits(
        l_handleWindowCompatibleDeviceContext,
        handleBitmapWindow,
        0,
        l_strechHeight,
        l_sourceImage.data,
        (BITMAPINFO*)&bitmapInfo,
        DIB_RGB_COLORS
    );
    /// @endcode
    //! <b>[window_capture]</b>

    //! <b>[clean]</b>
    /// Avoid memory leak.
    /// @code{.cpp}
    DeleteObject( handleBitmapWindow );
    DeleteDC( l_handleWindowCompatibleDeviceContext );
    ReleaseDC(
        _sourceWindowHandle,
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
    #else
    return ( cv::Mat{} );
    #endif
}

///////////////
/// @brief Compares a template against overlapped image regions.
/// @param[in] _match_method Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _image 2D image array where the search is running. It must be 8-bit or 32-bit floating-point.
/// @param[in] _templateImages_v Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _showResult Will print out squares of found images to other window.
/// @param[in] _image_display 2D image array with printed rectangles of found images.
/// @param[in] _templateMap Map of comparison results. Check the "0" field for errors.
///////////////
static void MatchTemplates(
    unsigned int _match_method,
    cv::Mat _image,
    std::vector< char* > _templateImages_v,
    const bool _showResult,
    cv::Mat& _image_display,
    std::map< std::string, std::array< unsigned int, 2 > >& _templateMap
) {
    //! <b>[check_image]</b>
    /// Return FAILED if 2D image array is empty.
    /// @code{.cpp}
    if ( _image.empty() ) {
        fmt::print( "Can't read source image\n" );

        _templateMap[ "0" ] = { 1, 1 };

        return;
    }
    /// @endcode
    //! <b>[check_image]</b>

    //! <b>[copy_source]</b>
    /// Source image to display.
    /// @code{.cpp}
    _image.copyTo( _image_display );
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

    auto MatchTemplate = [ & ]( char* _templateImage ) {
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
        int l_resultImage_cols = ( _image.cols - l_templateImage.cols + 1 );
        int l_resultImage_rows = ( _image.rows - l_templateImage.rows + 1 );

        l_resultImage.create(
            l_resultImage_rows,
            l_resultImage_cols,
            CV_32FC1
        );
        /// @endcode
        //! <b>[create_result_array]</b>

        //! <b>[match_template]</b>
        /// Do Matching.
        /// @code{.cpp}
        cv::matchTemplate(
            _image,                          // Source
            l_templateImage,                 // Destination
            l_resultImage,
            _match_method
        );
        /// @endcode
        //! <b>[match_template]</b>

        //! <b>[normalize]</b>
        /// Do Normalize.
        /// @code{.cpp}
        cv::normalize(
            l_resultImage,                        // Source
            l_resultImage,                        // Destination
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
        double l_minimumValue, l_maximumValue;
        cv::Point l_minimumLocation, l_maximumLocation;
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
        if( ( _match_method  == cv::TM_SQDIFF ) || ( _match_method == cv::TM_SQDIFF_NORMED ) ) {
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
            _image_display,
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
    _templateMap[ "0" ] = { 1, 1 };

    for ( auto _templateImage : _templateImages_v ) {
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
/// @param[in] _match_method Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _sourceImage Image where the search is running. It must be 8-bit or 32-bit floating-point.
/// @param[in] _templateImages_v Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _showResult Will print out squares of found images to other window.
/// @return Map of comparison results. Check the "0" field for errors.
///////////////
extern "C" std::map< std::string, std::array< unsigned int, 2 > > MatchingMethod(
    unsigned int _match_method,
    char* _sourceImage,
    std::vector< char* > _templateImages_v,
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
    cv::Mat l_image_display;
    std::map< std::string, std::array< unsigned int, 2 > > l_templateMap;

    MatchTemplates(
        _match_method,
        l_image,
        _templateImages_v,
        _showResult,
        l_image_display,
        l_templateMap
    );

    if ( l_image_display.empty() || !l_templateMap[ "0" ][ 0 ] || !l_templateMap[ "0" ][ 1 ] ) {
        return (
            std::map< std::string, std::array< unsigned int, 2 > >{ {
                "0", { 0, 0 }
            } }
        );
    }
    /// @endcode
    //! <b>[match]</b>

    //! <b>[imshow]</b>
    /// Show me what you got.
    /// @code{.cpp}
    if ( _showResult ) {
        cv::imshow( RESULT_WINDOW_NAME, l_image_display );
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
/// @param[in] _match_method Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _sourceWindowHandle Window where the search is running. It must be 8-bit or 32-bit floating-point.
/// @param[in] _templateImages_v Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _showResult Will print out squares of found images to other window.
/// @return Map of comparison results. Check the "0" field for errors.
///////////////
extern "C" std::map < std::string, std::array< unsigned int, 2 > > MatchingMethodWindow(
    unsigned int _match_method,
    uint32_t _sourceWindowHandle, // HWND
    std::vector< char* > _templateImages_v,
    const bool _showResult
) {
    //! <b>[load_image]</b>
    /// Get windows capture.
    /// @code{.cpp}
    cv::Mat l_image = getMatFromWindow( _sourceWindowHandle );
    /// @endcode
    //! <b>[load_image]</b>

    //! <b>[match]</b>
    /// Match template images on source image.
    /// @code{.cpp}
    cv::Mat l_image_display;
    std::map< std::string, std::array< unsigned int, 2 > > l_templateMap;

    MatchTemplates(
        _match_method,
        l_image,
        _templateImages_v,
        _showResult,
        l_image_display,
        l_templateMap );

    if ( l_image_display.empty() || !l_templateMap[ "0" ][ 0 ] || !l_templateMap[ "0" ][ 1 ] ) {
        return (
            std::map< std::string, std::array< unsigned int, 2 > >{ {
                "0", { 0, 0 }
            } }
        );
    }
    /// @endcode
    //! <b>[match]</b>

    //! <b>[imshow]</b>
    /// Show me what you got.
    /// @code{.cpp}
    cv::Mat t_l_image;
    cv::cvtColor(
        l_image_display,
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

// g++ -static-libgcc -static-libstdc++ -I "/usr/local/include/opencv5" matching.cpp -lfmt -fPIC -shared -o matching.so
