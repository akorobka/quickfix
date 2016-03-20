AC_DEFUN([AX_LIB_BOOST],
[
has_boost=false
AC_ARG_WITH(boost,
    [  --with-boost=<path>     prefix of boost installation. e.g. /usr/local/include/boost-1_33_1], 
    [if test $withval == "no"
     then
       has_boost=false
     else
       has_boost=true
     fi],
    has_boost=false
)
BOOST_PREFIX=$with_boost
AC_SUBST(BOOST_PREFIX)

if test $has_boost = true
then
    BOOST_LIBS="-L${BOOST_PREFIX}/lib -lboost_system -lboost_thread"
    BOOST_CFLAGS="-I${BOOST_PREFIX}/include"
    AC_SUBST(BOOST_CFLAGS)
    AC_DEFINE(HAVE_BOOST, 1, Define if you have boost framework)
fi
])
AC_DEFUN([AX_LIB_BOOST_CHECK],
[
if test $has_boost = true
then
    AC_MSG_CHECKING(for Boost)
    AC_TRY_COMPILE(
                [#include <boost/version.hpp>],
                [int v = BOOST_VERSION;],
                AC_MSG_RESULT(yes),
                AC_MSG_ERROR(no))
fi
])
