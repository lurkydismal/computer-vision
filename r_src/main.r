dyn.load("test.so")

x <- 1:3

ret_val <- .C(
    "test",
    n = length(x),
    as.double(x)
)

x
ret_val
ret_val$n
ret_val[[2]]
