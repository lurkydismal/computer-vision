///////////////
/// @file matching.cpp
/// @brief \c matchingMethodFile and \c matchingMethodWindow definition.
///////////////
#ifdef _WIN32

#include <windows.h>

#else // _WIN32

#include <unistd.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#endif // _WIN32

// Copyright (C) 2000-2022, Intel Corporation, all rights reserved.
// Copyright (C) 2009-2011, Willow Garage Inc., all rights reserved.
// Copyright (C) 2009-2016, NVIDIA Corporation, all rights reserved.
// Copyright (C) 2010-2013, Advanced Micro Devices, Inc., all rights reserved.
// Copyright (C) 2015-2022, OpenCV Foundation, all rights reserved.
// Copyright (C) 2008-2016, Itseez Inc., all rights reserved.
// Copyright (C) 2019-2022, Xperience AI, all rights reserved.
// Copyright (C) 2019-2022, Shenzhen Institute of Artificial Intelligence and
//                          Robotics for Society, all rights reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgcodecs.hpp>
#include <opencv4/opencv2/imgproc.hpp>

// Copyright (c) 2012 - present, Victor Zverovich

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#include <fmt/core.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <map>
#include <string>
#include <stdlib.h>
#include <stdexcept>
#include <thread>
#include <vector>

//! <b>[define]</b>
/// @code{.cpp}
#define RESULT_WINDOW_NAME "Result window"

#ifndef _WIN32

#define portable_usleep( _time ) select( 0, NULL, NULL, NULL, _time )

#endif // _WIN32
/// @endcode
//! <b>[define]</b>

//! <b>[enum]</b>
/// @code{.cpp}
#ifdef _WIN32

enum click_t {
    MOUSE_RIGHT_CLICK = 0x0008,
    MOUSE_LEFT_CLICK  = 0x0002
};

#else // _WIN32

enum click_t {
    MOUSE_RIGHT_CLICK,
    MOUSE_LEFT_CLICK
};

#endif // _WIN32
/// @endcode
//! <b>[enum]</b>

#ifdef _WIN32

///////////////
/// @brief Get \c cv::Mat object from window capture.
/// @param[in] _sourceWindowName Window handle.
/// @return Window capture.
///////////////
static cv::Mat getMatFromWindow( const std::string& _sourceWindowName ) {
    //! <b>[declare]</b>
    /// @code{.cpp}
    cv::Mat          l_sourceImage;
    BITMAPINFOHEADER l_bitmapInfo;
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[window_info]</b>
    /// @code{.cpp}
    HWND l_sourceWindowHandle                  = FindWindow( NULL, _sourceWindowName.c_str() );
    HDC  l_handleWindowDeviceContext           = GetDC( l_sourceWindowHandle );
    HDC  l_handleWindowCompatibleDeviceContext = CreateCompatibleDC( l_handleWindowDeviceContext );

    SetStretchBltMode(
        l_handleWindowCompatibleDeviceContext,
        COLORONCOLOR
    );

    RECT l_windowSize;

    GetClientRect(
        l_sourceWindowHandle,
        &l_windowSize
    );

    uint32_t l_sourceHeight = l_windowSize.bottom;
    uint32_t l_sourceWidth  = l_windowSize.right;
    uint32_t l_strechHeight = ( l_windowSize.bottom / 1 );
    uint32_t l_strechWidth  = ( l_windowSize.right / 1 );
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

    /// Copy from the window device context to the bitmap device context.
    /// Change SRCCOPY to NOTSRCCOPY for wacky colors.
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

#else // _WIN32

///////////////
/// @brief Get \c Window to needed window by name on \c Display .
/// @details Recursive function
/// @param[in] _display \c Display pointer.
/// @param[in] _window default root window.
/// @param[in] _windowName Window name.
/// @return \c Window information.
///////////////
static Window windowSearch(
    Display*       _display,
    Window         _window,
    const regex_t* _windowNameRegExp
) {
    //! <b>[declare]</b>
    /// @code{.cpp}
    Window        l_window = 0;
    Window        l_root;
    Window        l_parent;
    Window*       l_children;
    uint32_t      l_childrenCount;
    int           l_windowNamesCount = 0;
    XTextProperty l_xTextProperty;
    char**        l_windowNamesList  = NULL;
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[check]</b>
    /// Compare root window name to received.
    /// @code{.cpp}
    XGetWMName( _display, _window, &l_xTextProperty );

    if ( l_xTextProperty.nitems > 0 ) {
        Xutf8TextPropertyToTextList(
            _display,
            &l_xTextProperty,
            &l_windowNamesList,
            &l_windowNamesCount
        );

        for (
            int _windowNameIndex = 0;
            _windowNameIndex < l_windowNamesCount;
            _windowNameIndex++
        ) {
            if (
                regexec(
                    _windowNameRegExp,
                    l_windowNamesList[ _windowNameIndex ],
                    0,
                    NULL,
                    0
                ) == 0
            ) {
                XFreeStringList( l_windowNamesList );
                XFree( l_xTextProperty.value );

                return ( _window );
            }
        }
    }

    XFreeStringList( l_windowNamesList );
    XFree( l_xTextProperty.value );
    /// @endcode
    //! <b>[check]</b>

    //! <b>[check_next]</b>
    /// Compare all window names to received.
    /// @code{.cpp}
    if (
        XQueryTree(
            _display,
            _window,
            &l_root,
            &l_parent,
            &l_children,
            &l_childrenCount
        )
    ) {
        for (
            uint32_t _childrenIndex = 0;
            _childrenIndex < l_childrenCount;
            ++_childrenIndex
        ) {
            l_window = windowSearch( _display, l_children[ _childrenIndex ], _windowNameRegExp );

            if ( l_window ) {
                break;
            }
        }

        XFree( l_children );
    }
    /// @endcode
    //! <b>[check_next]</b>

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( l_window );
    /// @endcode
    //! <b>[return]</b>
}

///////////////
/// @brief Get \c Window to needed window by name.
/// @param[in] _windowName Window name.
/// @return \c Window information.
///////////////
static Window getWindowByName( std::string _windowName ) {
    //! <b>[declare]</b>
    /// @code{.cpp}
    Display* l_display = XOpenDisplay( NULL );
    regex_t l_windowNameRegExp;

    regcomp(
        &l_windowNameRegExp,
        _windowName.c_str(),
        REG_EXTENDED | REG_ICASE
    );
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[search]</b>
    /// Get \c Window by window name.
    /// @code{.cpp}
    Window l_window = windowSearch(
        l_display,
        XDefaultRootWindow( l_display ),
        &l_windowNameRegExp
    );
    /// @endcode
    //! <b>[search]</b>

    //! <b>[close]</b>
    /// @code{.cpp}
    regfree( &l_windowNameRegExp );
    XCloseDisplay( l_display );
    /// @endcode
    //! <b>[close]</b>

    //! <b>[error]</b>
    /// @code{.cpp}
    if ( !l_window ) {
        fmt::print(
            stderr,
            "Window search failed: {}\n",
            _windowName
        );
    }
    /// @endcode
    //! <b>[error]</b>

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( l_window );
    /// @endcode
    //! <b>[return]</b>
}

///////////////
/// @brief Get \c cv::Mat object from window capture.
/// @details Capture width and height should be less or equal to window's.
/// @param[in] _sourceWindowName Window handle.
/// @param[in] _captureWidth Capture width. Optional.
/// @param[in] _captureHeight Capture height. Optional.
/// @return Window capture.
///////////////
static cv::Mat getMatFromWindow(
    const std::string& _sourceWindowName,
    uint32_t           _captureWidth  = 0,
    uint32_t           _captureHeight = 0
) {
    //! <b>[declare]</b>
    /// @code{.cpp}
    Display*          l_display = XOpenDisplay( NULL );
    Window            l_window  = getWindowByName( _sourceWindowName );
    XWindowAttributes l_windowAttributes;

    XGetWindowAttributes(
        l_display,
        l_window,
        &l_windowAttributes
    );

    if ( !_captureWidth ) {
        _captureWidth = l_windowAttributes.width;
    }

    if ( !_captureHeight ) {
        _captureHeight = l_windowAttributes.height;
    }

    Screen*         l_screen = l_windowAttributes.screen;
    XShmSegmentInfo l_shminfo;
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[canvas]</b>
    /// @code{.cpp}
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
    /// @endcode
    //! <b>[canvas]</b>

    //! <b>[prepare]</b>
    /// Prepare window information to capture.
    /// @code{.cpp}
    l_shminfo.shmid = shmget(
        IPC_PRIVATE,
        ( l_xImage->bytes_per_line * l_xImage->height ),
        ( IPC_CREAT | 0777 )
    );
    l_shminfo.shmaddr  = l_xImage->data = static_cast< char* >( shmat( l_shminfo.shmid, 0, 0 ) );
    l_shminfo.readOnly = false;
    /// @endcode
    //! <b>[prepare]</b>

    //! <b>[error]</b>
    /// @code{.cpp}
    if ( !l_shminfo.shmid ) {
        fmt::print(
            stderr,
            "Fatal shminfo error!"
        );
    }
    /// @endcode
    //! <b>[error]</b>

    //! <b>[attach]</b>
    /// Attach to display with \c l_shminfo.
    /// @code{.cpp}
    XShmAttach( l_display, &l_shminfo );
    /// @endcode
    //! <b>[attach]</b>

    //! <b>[capture]</b>
    /// @code{.cpp}
    XShmGetImage(
        l_display,
        l_window,
        l_xImage,
        0,
        0,
        0x00ffffff
    );
    /// @endcode
    //! <b>[capture]</b>

    //! <b>[color]</b>
    /// Convert source image to template's color format.
    /// @code{.cpp}
    cv::Mat l_image = cv::Mat(
        _captureHeight,
        _captureWidth,
        CV_8UC4,
        l_xImage->data
    );

    cv::Mat t_l_image;

    cv::cvtColor(
        l_image,
        t_l_image,
        cv::COLOR_RGB2BGR
    );
    /// @endcode
    //! <b>[color]</b>

    //! <b>[close]</b>
    /// @code{.cpp}
    XShmDetach( l_display, &l_shminfo );
    XDestroyImage( l_xImage );
    shmdt( l_shminfo.shmaddr );
    XCloseDisplay( l_display );
    /// @endcode
    //! <b>[close]</b>

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( t_l_image );
    /// @endcode
    //! <b>[return]</b>
}

#endif // _WIN32

///////////////
/// @brief Compares a template against overlapped image regions.
/// @details Throws ios_base::failure at error.
/// @param[in] _matchMethod Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _image 2D image array where the search is running. It must be 8-bit or 32-bit floating-point.
/// @param[in] _templateImages Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _showResult Will print out squares of found images to other window.
/// @param[in] _imageDisplay 2D image array with printed rectangles of found images.
/// @param[in] _templateMap Map of comparison results.
///////////////
static void matchTemplates(
    uint32_t   _matchMethod,
    cv::Mat    _image,
    std::vector< std::string > _templateImages,
    const bool _showResult,
    cv::Mat&   _imageDisplay,
    std::map< std::string, std::array< uint32_t, 2 > >& _templateMap
) {
    //! <b>[check_image]</b>
    /// @code{.cpp}
    if ( _image.empty() ) {
        throw std::ios_base::failure( "Can't read image cv::Mat" );
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

    auto matchTemplate = [ & ]( std::string _templateImage ) {
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
            throw std::ios_base::failure(
                fmt::format(
                    "Can't read template image {}",
                    _templateImage
                )
            );
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

    for ( std::string _templateImage : _templateImages ) {
        l_matchTemplateThreads.push_back(
            std::thread( [ &, _templateImage ]{
                l_matchedLocation = matchTemplate( _templateImage );

                _templateMap[ _templateImage ] = {
                    static_cast< uint32_t >( l_matchedLocation.x ),
                    static_cast< uint32_t >( l_matchedLocation.y )
                };
            } )
        );
    }

    for (
        uint32_t _matchTemplateThreadIndex = 0;
        _matchTemplateThreadIndex < l_matchTemplateThreads.size();
        _matchTemplateThreadIndex++
    ) {
        l_matchTemplateThreads[ _matchTemplateThreadIndex ].join();
    }
    /// @endcode
    //! <b>[match_templates]</b>
}

///////////////
/// @brief Compares a template against overlapped image regions.
/// @details Throws ios_base::failure at error.
/// @param[in] _matchMethod Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _sourceImage Image where the search is running. It must be 8-bit or 32-bit floating-point.
/// @param[in] _templateImages Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _showResult Will print out squares of found images to window.
/// @return Map of comparison results.
///////////////
std::map< std::string, std::array< uint32_t, 2 > > matchingMethodFile(
    uint32_t     _matchMethod,
    std::string  _sourceImage,
    const std::vector< std::string >& _templateImages,
    const bool   _showResult
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
    std::map< std::string, std::array< uint32_t, 2 > > l_templateMap;

    matchTemplates(
        _matchMethod,
        l_image,
        _templateImages,
        _showResult,
        l_imageDisplay,
        l_templateMap
    );

    if ( l_imageDisplay.empty() ) {
        throw std::ios_base::failure(
            fmt::format(
                "Can't read source image {}",
                _sourceImage
            )
        );
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
/// @details Throws ios_base::failure at error.
/// @param[in] _matchMethod Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _sourceWindowName Window where the search is running.
/// @param[in] _templateImages Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _showResult Will print out squares of found images to other window.
/// @return Map of comparison results.
///////////////
std::map < std::string, std::array< uint32_t, 2 > > matchingMethodWindow(
    uint32_t    _matchMethod,
    const std::string& _sourceWindowName,
    const std::vector< std::string >& _templateImages,
    const bool  _showResult
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
    std::map< std::string, std::array< uint32_t, 2 > > l_templateMap;

    matchTemplates(
        _matchMethod,
        l_image,
        _templateImages,
        _showResult,
        l_imageDisplay,
        l_templateMap );

    if ( l_imageDisplay.empty() ) {
        throw std::ios_base::failure(
            fmt::format(
                "Can't read source window {}",
                _sourceWindowName
            ));
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

///////////////
/// @brief Compares a template against overlapped image regions.
/// @details Throws ios_base::failure at error.
/// @param[in] _matchMethod Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _sourceImage Image where the search is running. It must be 8-bit or 32-bit floating-point.
/// @param[in] _templateImage Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _searchResults Array to store result.
/// @param[in] _showResult Will print out squares of found images to window.
///////////////
extern "C" void matchingMethodFile(
    uint32_t*   _matchMethod,
    char**      _sourceImage,
    char**      _templateImage,
    double*     _searchResults,
    const bool* _showResult
) {
    std::map<
        std::string,
        std::array< uint32_t, 2 >
    > l_coordinates = matchingMethodFile(
        *_matchMethod,
        std::string( *_sourceImage ),
        { std::string( *_templateImage ) },
        *_showResult
    );

    _searchResults[ 0 ] = l_coordinates[ std::string( *_templateImage ) ][ 0 ];
    _searchResults[ 1 ] = l_coordinates[ std::string( *_templateImage ) ][ 1 ];
}

///////////////
/// @brief Compares a template against overlapped image regions.
/// @details Throws ios_base::failure at error.
/// @param[in] _matchMethod Parameter specifying the comparison method, see cv::TemplateMatchModes.
/// @param[in] _sourceWindowName Window where the search is running.
/// @param[in] _templateImage Searched template. It must be not greater than the source image and have the same data type.
/// @param[in] _searchResults Array to store result.
/// @param[in] _showResult Will print out squares of found images to other window.
///////////////
extern "C" void matchingMethodWindow(
    uint32_t*   _matchMethod,
    char**      _sourceWindowName,
    char**      _templateImage,
    double*     _searchResults,
    const bool* _showResult
) {
    std::map<
        std::string,
        std::array< uint32_t, 2 >
    > l_coordinates = matchingMethodWindow(
        *_matchMethod,
        std::string( *_sourceWindowName ),
        { std::string( *_templateImage ) },
        *_showResult
    );

    _searchResults[ 0 ] = l_coordinates[ std::string( *_templateImage ) ][ 0 ];
    _searchResults[ 1 ] = l_coordinates[ std::string( *_templateImage ) ][ 1 ];
}

#ifdef _WIN32

///////////////
/// @brief Clicks on window by coordinates.
/// @param[in] _windowName Window name.
/// @param[in] _coordinateX X relative to window.
/// @param[in] _coordinateY Y relative to window.
/// @param[in] _button Button of \c click_t .
/// @param[in] _sleepTime Pause between hold and release.
/// @return Clicked or not.
///////////////
extern "C" bool mouseClick(
    std::string    _windowName,
    const uint32_t _coordinateX,
    const uint32_t _coordinateY,
    click_t        _button,
    uint32_t       _sleepTime
) {
    //! <b>[declare]</b>
    /// @code{.cpp}
    HWND  l_windowHandle = FindWindow( NULL, _windowName.c_str() );
    POINT l_point;

    l_point.x = _coordinateX;
    l_point.y = _coordinateY;
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[]</b>
    /// ClientToScreen function converts the client-area coordinates of a specified l_point to screen coordinates.
    /// @code{.cpp}
    if ( ClientToScreen(
        l_windowHandle, // A handle to the window whose client area is used for the conversion.
        &l_point        // A l_pointer to a \c POINT structure that contains the client coordinates to be converted. The new screen coordinates are copied into this structure if the function succeeds.
        )
    ) {
    /// @endcode
    //! <b>[]</b>
        //! <b>[set_cursor_position]</b>
        /// Setting cursor position by X and Y.
        /// @code{.cpp}
        SetCursorPos(
            l_point.x,              // X
            l_point.y               // Y
        );
        /// @endcode
        //! <b>[set_cursor_position]</b>

        //! <b>[input]</b>
        /// Creating the \c INPUT structure with parameters to down left mouse button.
        /// @code{.cpp}
        INPUT l_mouseInput = { 0 }; // Empty

        l_mouseInput.type       = INPUT_MOUSE;
        l_mouseInput.mi.dwFlags = _button;
        /// @endcode
        //! <b>[input]</b>

        //! <b>[click]</b>
        /// Sendind input with \c INPUT structure to \c winuser.
        /// @code{.cpp}
        SendInput(
            1,                      // The number of structures in the \c pInputs array.
            &l_mouseInput,          // An array of \c INPUT structures. Each structure represents an event to be inserted into the keyboard or mouse input stream.
            sizeof( l_mouseInput )  // The size, in bytes, of an \c INPUT structure. If cbSize is not the size of an \c INPUT structure, the function fails.
        );
        /// @endcode
        //! <b>[click]</b>

        //! <b>[sleep]</b>
        /// Pause between mouse down and mouse up.
        /// @code{.cpp}
        Sleep( _sleepTime );
        /// @endcode
        //! <b>[sleep]</b>

        //! <b>[input]</b>
        /// Editing the \c INPUT structure with parameters to up left mouse button.
        /// @code{.cpp}
        l_mouseInput.type       = INPUT_MOUSE;
        l_mouseInput.mi.dwFlags = ( _button + 0x0002 );
        /// @endcode
        //! <b>[input]</b>

        //! <b>[click]</b>
        /// Sendind input with \c INPUT structure to \c winuser.
        /// @code{.cpp}
        SendInput(
            1,                      // The number of structures in the pInputs array.
            &l_mouseInput,            // An array of INPUT structures. Each structure represents an event to be inserted into the keyboard or mouse input stream.
            sizeof( l_mouseInput )    // The size, in bytes, of an INPUT structure. If cbSize is not the size of an INPUT structure, the function fails.
        );
        /// @endcode
        //! <b>[click]</b>

        //! <b>[return]</b>
        /// End of function.
        /// @code{.cpp}
        return ( true );
        /// @endcode
        //! <b>[return]</b>
    }

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( false );
    /// @endcode
    //! <b>[return]</b>
}

///////////////
/// @brief Left clicks on window by coordinates.
/// @param[in] _windowName Window name.
/// @param[in] _coordinateX X relative to window.
/// @param[in] _coordinateY Y relative to window.
///////////////
extern "C" void leftMouseClick(
    char**    _windowName,
    uint32_t* _coordinateX,
    uint32_t* _coordinateY
) {
    mouseClick(
        _windowName,
        _coordinateX,
        _coordinateY,
        click_t::MOUSE_LEFT_CLICK,
        ( 0.5 * 1000 )
    );
}

#else // _WIN32

///////////////
/// @brief Clicks on window by coordinates.
/// @param[in] _display \c Display to click.
/// @param[in] _coordinateX X relative to display.
/// @param[in] _coordinateY Y relative to display.
/// @param[in] _button Button of \c click_t .
/// @param[in] _sleepTime Pause between hold and release.
/// @return Clicked or not.
///////////////
static bool mouseClick(
    Display*        _display,
    uint32_t        _coordinateX,
    uint32_t        _coordinateY,
    click_t         _button,
    struct timeval* _sleepTime
) {
    //! <b>[declare]</b>
    /// @code{.cpp}
    Window l_window    = DefaultRootWindow( _display );
    bool   l_isSuccess = true;
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[mouse_move]</b>
    /// @code{.cpp}
    XWarpPointer(
        _display,     // Specifies the connection to the X server.
        None,         // Specifies the source window
        l_window,     // Specifies the destination window
        0,            // Source X
        0,            // Source Y
        0,            // Specify a rectangle width in the source window.
        0,            // Specify a rectangle height in the source window.
        _coordinateX, // Specify the X coordinates within the destination window.
        _coordinateY  // Specify the Y coordinates within the destination window.
    );
    /// @endcode
    //! <b>[mouse_move]</b>

    //! <b>[get_pointer]</b>
    /// Get pointer coordinates.
    /// @code{.cpp}
    XEvent l_xEvent;

    memset( &l_xEvent, 0, sizeof( l_xEvent ) );

    l_xEvent.xbutton.type        = ButtonPress;
    l_xEvent.xbutton.button      = _button;
    l_xEvent.xbutton.same_screen = True;

    XQueryPointer(
        _display,
        l_window,
        &l_xEvent.xbutton.root,
        &l_xEvent.xbutton.window,
        &l_xEvent.xbutton.x_root,
        &l_xEvent.xbutton.y_root,
        &l_xEvent.xbutton.x,
        &l_xEvent.xbutton.y,
        &l_xEvent.xbutton.state
    );

    l_xEvent.xbutton.subwindow = l_xEvent.xbutton.window;

    while ( l_xEvent.xbutton.subwindow ) {
        l_xEvent.xbutton.window = l_xEvent.xbutton.subwindow;

        XQueryPointer(
            _display,
            l_xEvent.xbutton.window,
            &l_xEvent.xbutton.root,
            &l_xEvent.xbutton.subwindow,
            &l_xEvent.xbutton.x_root,
            &l_xEvent.xbutton.y_root,
            &l_xEvent.xbutton.x,
            &l_xEvent.xbutton.y,
            &l_xEvent.xbutton.state
        );
    }
    /// @endcode
    //! <b>[get_pointer]</b>

    //! <b>[press]</b>
    /// @code{.cpp}
    if ( XSendEvent( _display, PointerWindow, True, 0xfff, &l_xEvent ) == 0 ) {
        fmt::print( stderr, "Error on first XSendEvent()" );

        l_isSuccess = false;
    }
    /// @endcode
    //! <b>[press]</b>

    //! <b>[hold]</b>
    /// @code{.cpp}
    XFlush( _display );
    portable_usleep( _sleepTime );
    /// @endcode
    //! <b>[hold]</b>

    //! <b>[release]</b>
    /// @code{.cpp}
    l_xEvent.type          = ButtonRelease;
    l_xEvent.xbutton.state = 0x100;

    if ( XSendEvent( _display, PointerWindow, True, 0xfff, &l_xEvent ) == 0 ) {
        fmt::print( stderr, "Error on second XSendEvent()" );

        l_isSuccess = false;
    }
    /// @endcode
    //! <b>[release]</b>

    //! <b>[apply]</b>
    /// @code{.cpp}
    XFlush( _display );
    /// @endcode
    //! <b>[apply]</b>

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( l_isSuccess );
    /// @endcode
    //! <b>[return]</b>
}

///////////////
/// @brief Left clicks on window by coordinates.
/** @details The coordinates in attr are relative to the parent window.
  * If the parent window is the root window, then the coordinates are correct.
  * If the parent window isn't the root window then we translate them.
**/
/// @param[in] _windowName Window name.
/// @param[in] _coordinateX X relative to window.
/// @param[in] _coordinateY Y relative to window.
///////////////
extern "C" void leftMouseClick(
    char**    _windowName,
    uint32_t* _coordinateX,
    uint32_t* _coordinateY
) {
    //! <b>[declare]</b>
    /// @code{.cpp}
    Display* l_display = XOpenDisplay( NULL );
    Window   l_parentWindow;
    Window   l_rootWindow;
    Window*  l_childrenWindow;
    uint32_t l_childrenCount;
    int      l_windowX;
    int      l_windowY;

    if ( !l_display ) {
        fmt::print( stderr, "Can't open display!\n" );

        return;
    }

    struct timeval l_sleepTime;
    l_sleepTime.tv_sec  = 0;
    l_sleepTime.tv_usec = ( 0.5 * 1000 * 1000 ); // 0.5 seconds

    Window l_window = getWindowByName( std::string( *_windowName ) );

    XWindowAttributes l_windowAttributes;
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[get_attributes]</b>
    /// @code{.cpp}
    XGetWindowAttributes(
        l_display,
        l_window,
        &l_windowAttributes
    );
    /// @endcode
    //! <b>[get_attributes]</b>

    //! <b>[get_window_IDs]</b>
    /// @code{.cpp}
    XQueryTree(
        l_display,
        l_window,
        &l_rootWindow,
        &l_parentWindow,
        &l_childrenWindow,
        &l_childrenCount
    );

    if ( l_childrenWindow != NULL ) {
        XFree( l_childrenWindow );
    }
    /// @endcode
    //! <b>[get_window_IDs]</b>

    //! <b>[check]</b>
    /// @code{.cpp}
    if ( l_parentWindow == l_windowAttributes.root ) {
        l_windowX = l_windowAttributes.x;
        l_windowY = l_windowAttributes.y;

    } else {
        Window l_unusedChildren;

        XTranslateCoordinates(
            l_display,
            l_window,
            l_windowAttributes.root,
            0,
            0,
            &l_windowX,
            &l_windowY,
            &l_unusedChildren
        );
    }
    /// @endcode
    //! <b>[check]</b>

    //! <b>[click]</b>
    /// @code{.cpp}
    mouseClick(
        l_display,
        ( l_windowX + *_coordinateX ),
        ( l_windowY + *_coordinateY ),
        click_t::MOUSE_LEFT_CLICK,
        &l_sleepTime
    );
    /// @endcode
    //! <b>[click]</b>

    //! <b>[close_handle]</b>
    /// @code{.cpp}
    XCloseDisplay( l_display );
    /// @endcode
    //! <b>[close_handle]</b>
}

#endif // _WIN32
