AC_DEFUN([AX_ORDERING],
[
has_relaxed_ordering=false
AC_ARG_WITH(relaxed_ordering,
    [  --with-relaxed-ordering=<8, 16>  Allows for up to the given number of out of order fields in the message body], 
    [if test $withval == "no"
     then
       has_relaxed_ordering=false
     else
       has_relaxed_ordering=true
     fi],
    has_relaxed_ordering=false
)

if test "x$with_relaxed_ordering" = "x8"
then
    AC_DEFINE_UNQUOTED(ENABLE_RELAXED_ORDERING, 8, Allow up to 8 out of order fields in the message body)
elif test "x$with_relaxed_ordering" = "x16" || test "x$with_relaxed_ordering" == "xyes"
then
    AC_DEFINE_UNQUOTED(ENABLE_RELAXED_ORDERING, 16, Allow up to 16 out of order fields in the message body)
elif test "x$with_relaxed_ordering" != "xno" && test "x$with_relaxed_ordering" != "x"
then
    AC_MSG_ERROR([invalid value: $with_relaxed_ordering])
fi
])
