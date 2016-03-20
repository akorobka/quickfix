AC_DEFUN([AX_LIB_SPARSEHASH],
[
has_sparsehash=false
AC_ARG_WITH(sparsehash,
    [  --with-sparsehash=<path>
                          prefix of Google Sparsehash installation. e.g. /usr/local/include/sparsehash],
    [if test $withval == "no"
     then
       has_sparsehash=false
     else
       has_sparsehash=true
     fi],
    has_sparsehash=false
)
SPARSEHASH_PREFIX=$with_sparsehash
AC_SUBST(SPARSEHASH_PREFIX)

if test $has_sparsehash = true
then
    SPARSEHASH_CFLAGS="-I${SPARSEHASH_PREFIX}/include"
    AC_SUBST(SPARSEHASH_CFLAGS)
    AC_DEFINE(HAVE_SPARSEHASH, 1, Define if you have Google Sparsehash)
fi
])
AC_DEFUN([AX_LIB_SPARSEHASH_CHECK],
[
if test $has_sparsehash = true
then
    AC_MSG_CHECKING(for Google Sparsehash)
    AC_TRY_COMPILE(
                [#include <sparsehash/dense_hash_map>],
                [google::dense_hash_map < int, int > x;],
                AC_MSG_RESULT(yes),
                AC_MSG_ERROR(no))
fi
])
