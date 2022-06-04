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

dyn.load("matching.so")

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
            match_method = as.integer(1),
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

window_name <- "Window name"

while (TRUE) {
    for (key_index in seq_len(length(data))) {
        key <- ls(data)[key_index]

        for (value_index in seq_len(length(data[[key]]))) {
            value      <- data[[key]][value_index]
            coordinate <- coordinates[[value]]

            returned_value <- .C(
                "matchingMethodWindow",
                match_method       = as.integer(1),
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

            if (coordinate == returned_value$search_results) {
                returned_value <- .C(
                    "leftMouseClick",
                    window_name = window_name,
                    x = as.integer(coordinate[1]),
                    y = as.integer(coordinate[2])
                )
            }
        }
    }

    Sys.sleep(1)
}
