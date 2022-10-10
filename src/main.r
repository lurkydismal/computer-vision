# Copyright (c) 2020 stringr authors

# Permission is hereby granted, free of charge,
# to any person obtaining a copy of this software and associated
# documentation files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy, modify,
# merge, publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:

# The above copyright notice and this permission notice
# shall be included in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
library(stringr)

print("Launching...")

enum <- function(...) {
    values <- sapply(
        match.call(expand.dots = TRUE)[-1L],
        deparse
    )

    stopifnot(identical(unique(values), values))

    res <- setNames(seq_along(values), values)
    res <- as.environment(as.list(res))

    lockEnvironment(res, bindings = TRUE)

    res
}

# Matching methods:
# TM_SQDIFF        - 0
# TM_SQDIFF_NORMED - 1
# TM_CCORR         - 2
# TM_CCORR_NORMED  - 3
# TM_CCOEFF        - 4
# TM_CCOEFF_NORMED - 5
# More on https://docs.opencv.org/4.x/df/dfb/group__imgproc__object.html#ga3a7850640f1fe1f58fe91a2d7583695d

match_method <- 1
image_file_directory <- "image"
image_file_extension <- ".png"

files <- list.files(
    path        = image_file_directory,
    pattern     = paste("\\", image_file_extension, "$", sep = ""),
    ignore.case = TRUE
)
source_t    <- enum(SAMPLE, TEMPLATE)
files       <- sub(paste("\\", image_file_extension, "$", sep = ""), "", files)
data        <- new.env()
coordinates <- new.env()

if (.Platform$OS.type == "windows") {
    dyn.load("matching.dll")

} else if (.Platform$OS.type == "unix") {
    dyn.load("matching.so")
}

for (file_index in seq_len(length(files))) {
    file <- str_split(files[file_index], "\\.")[[1]]

    if (file[1] == "sample") {
        type <- source_t$SAMPLE

    } else if (file[1] == "template") {
        type <- source_t$TEMPLATE
    }

    if (type == source_t$TEMPLATE) {
        key   <- file[2:length(file)][1]
        value <- file[2:length(file)]

        data[[key]] <- c(
            data[[key]],
            paste(
                value[seq_len(length(file) - 1)],
                sep      = "",
                collapse = "."
            )
        )
    }
}

print("data:")
mget(ls(data), envir = data)

for (key_index in seq_len(length(data))) {
    key <- ls(data)[key_index]

    for (value_index in seq_len(length(data[[key]]))) {
        value <- data[[key]][value_index]

        returned_value <- .C(
            "matchingMethodFile",
            match_method = as.integer(match_method),
            source_image = paste(
                paste(image_file_directory, "/", sep = ""),
                "sample.",
                key,
                image_file_extension,
                sep = ""
            ),
            template_images = paste(
                paste(image_file_directory, "/", sep = ""),
                "template.",
                value,
                image_file_extension,
                sep = ""
            ),
            search_results = as.double(1:2),
            show_result    = FALSE
        )

        coordinates[[value]] <- c(
            returned_value$search_results[1],
            returned_value$search_results[2]
        )
    }
}

print("coordinates:")
mget(ls(coordinates), envir = coordinates)

window_name <- "Window Name"

Sys.sleep(3)

while (TRUE) {
    for (key_index in seq_len(length(data))) {
        key <- ls(data)[key_index]

        for (value_index in seq_len(length(data[[key]]))) {
            value      <- data[[key]][value_index]
            coordinate <- coordinates[[value]]

            returned_value <- .C(
                "matchingMethodWindow",
                match_method       = as.integer(match_method),
                source_window_name = window_name,
                template_images    = paste(
                    paste(image_file_directory, "/", sep = ""),
                    "template.",
                    value,
                    image_file_extension,
                    sep = ""
                ),
                search_results = as.double(1:2),
                show_result    = TRUE
            )

            # if (coordinate == returned_value$search_results) {
            #     returned_value <- .C(
            #         "leftMouseClick",
            #         window_name = window_name,
            #         x = as.integer(coordinate[1]),
            #         y = as.integer(coordinate[2])
            #     )
            # }
        }
    }

    Sys.sleep(1)
}
