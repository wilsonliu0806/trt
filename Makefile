##����ϵͳҪ�����Ļ�������
include $(NGBILLING_HOME)/src/make/ngmk.con

##���õ�ǰĿ¼��(����)
WORK_DIR = APP_TRT

##������ҪҪ�����Դ�ļ�,�ÿո�ָ�,��\����(����)
SRCS         = trt.cpp

##���ɵ�Ŀ�����(����,��ѡһ)
PROGRAM_EXE    = trt

##���ɵ�Ŀ�꾲̬��
STATIC_LIBRARY =

##���ɵ�Ŀ�궯̬��
SHARE_LIBRARY  =

##������صĲ�������
USR_FLAGS=-DSOOS_LOG_TEST -D_DBC_ORA_OR_INFO -D_DBC_MDB -D__RATING_MDBSUPPORT

##ָ���������Ӧ���������Ŀ���
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


##����֮��Ķ���,дһЩ��Ϣ��.
AFTER_MAKE  =

##������������
include $(NGBILLING_MAKE)/ngmk.cmd

#ԭ���ϲ���֧��ԭHBϵͳһ��Ŀ¼�±���N��Ӧ��,����Ҫ����Ӧ�õĲ�ͬ���ı�.�Ժ�ÿ��Ӧ����Ҫ���Լ���Ŀ¼.
#����:
#billdisct.exe:billdisct.o
#   $(CCC) -o billdisct  ....
#dup.exe:DupApp.o
#   $(CCC) -o dup  ....
