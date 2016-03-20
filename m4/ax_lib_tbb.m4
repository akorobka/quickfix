AC_DEFUN([AX_LIB_TBB],
[
has_tbb=false
AC_ARG_WITH(tbb,
    [  --with-tbb=<path>       prefix of TBB installation. e.g. /opt/intel/tbb/3.0],
    [if test $withval == "no"
     then
       has_tbb=false
     else
       has_tbb=true
     fi],
    has_tbb=false
)
TBB_PREFIX=$with_tbb
AC_SUBST(TBB_PREFIX)

has_tbb_arch=false
AC_ARG_WITH(tbb-arch,
    [  --with-tbb-arch=<arch>  TBB arch subdir, e.g. intel64/cc4.1.0_libc2.4_kernel2.6.16.21 ],
    [if test $withval == "no"
     then
       has_tbb_arch=false
     else
       has_tbb_arch=true
     fi],
    has_tbb_arch=false
)
TBB_ARCH=$with_tbb_arch
AC_SUBST(TBB_ARCH)

if test $has_tbb = true
then
    TBB_CFLAGS="-I${TBB_PREFIX}/include"
    AC_SUBST(TBB_CFLAGS)
    TBB_LIBS="-L${TBB_PREFIX}/lib/${TBB_ARCH} -ltbbmalloc -ltbb"
    AC_SUBST(TBB_LIBS)
    AC_DEFINE(HAVE_TBB, 1, Define if you have Intel TBB framework)
fi
])
AC_DEFUN([AX_LIB_TBB_CHECK],
[
if test $has_tbb == true
then
        AC_MSG_CHECKING(for Intel TBB)
        AC_TRY_COMPILE(
                [#include <tbb/spin_mutex.h>],
                [tbb::spin_mutex m;],
                AC_MSG_RESULT(yes),
                AC_MSG_ERROR(no))
fi
])
