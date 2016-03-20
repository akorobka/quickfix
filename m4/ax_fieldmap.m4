AC_DEFUN([AX_FIELDMAP],
[
has_fieldmap=false
AC_ARG_WITH(fieldmap,
    [  --with-fieldmap=<avl, flat>  Selects container for field storage], 
    [if test $withval == "no"
     then
       has_fieldmap=false
     else
       has_fieldmap=true
     fi],
    has_fieldmap=false
)

if test "x$with_fieldmap" = "xflat"
then
    AC_DEFINE_UNQUOTED(ENABLE_FLAT_FIELDMAP, 1, Define to use flat set container for the message body)
elif test "x$with_fieldmap" != "xavl" && test "x$with_fieldmap" != "x"
then
    AC_MSG_ERROR([invalid value: $with_fieldmap])
fi
])
