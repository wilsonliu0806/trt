##设置系统要依赖的环境变量
include $(NGBILLING_HOME)/src/make/ngmk.con

##设置当前目录名(必填)
WORK_DIR = APP_TRT

##所有需要要编译的源文件,用空格分隔,用\换行(必填)
SRCS         = trt.cpp

##生成的目标程序(必填,三选一)
PROGRAM_EXE    = trt

##生成的目标静态库
STATIC_LIBRARY =

##生成的目标动态库
SHARE_LIBRARY  =

##编译相关的参数定义
USR_FLAGS=-DSOOS_LOG_TEST -D_DBC_ORA_OR_INFO -D_DBC_MDB -D__RATING_MDBSUPPORT

##指定该组件或应用所依赖的库名
APP_LIB_PATH=$(NGBILLING_LIBPATH)
APP_LIB     = -lrating \
              -lpricing -lzxplugin -lxml \
              -lpricingtools \
			  -levsourcehelper \
			  -lshmdb \
			  -lshmconfig \
			  -lmdbcust \
              -lmdbsd \
			  -lshmdata \
			  -lshmfile \
			  -lattr \
			  -llog \
			  -ldate \
			  -lstrdis \
			  -lalgorithm \
			  -lcomponent \
			  -lcompatibility \
			  -ltools \
			  -lfile \
			  -llibrary_sys \
			  -lroamctrlfile \
			  -leventattr \
              $(NGLOG_LIBS)\
              -lzxfunc -lperforassist


##编译之后的动作,写一些信息等.
AFTER_MAKE  =

##设置命令规则等
include $(NGBILLING_MAKE)/ngmk.cmd

#原则上不再支持原HB系统一个目录下编译N个应用,库需要随着应用的不同而改变.以后每个应用需要有自己的目录.
#比如:
#billdisct.exe:billdisct.o
#   $(CCC) -o billdisct  ....
#dup.exe:DupApp.o
#   $(CCC) -o dup  ....
