dyn.load("matching.so")

match_method         <- 1
source_window_handle <- 2
template_images      <- "template_images"
search_results       <- 3:1
show_result          <- FALSE

ret_val <- .C(
    "test"
    # match_method = as.integer(match_method),
    # source_window_handle = as.integer(source_window_handle),
    # template_images = template_images,
    # search_results = as.double(search_results),
    # show_result = show_result
)

# search_results
# ret_val
# ret_val$show_result
# ret_val[[2]]