export PKG_CONFIG_PATH=/usr/local/AID/pkgconfig:$PKG_CONFIG_PATH

LDDFLAGS= -lrtspclient -lrockchip_rga -lrockchip_mpp -ldrm -lcam_engine_cifisp -lcam_ia -lpthread  `pkg-config --libs fastcv` `pkg-config --libs gtk+-3.0` `pkg-config --libs tengine` -lrockchip_vendor
INCFLAGS= `pkg-config --cflags fastcv` `pkg-config --cflags gtk+-3.0` `pkg-config --cflags tengine`
LIBDIR= -L/home/openailab/hiiragiiori/face_rtsp_ptz_tcp -ldom -lduration -lmecevp -lptz -lsmdevp -lsoapClient -lsoapC -lstdsoap2 -lthreads -lwsaapi -lwsseapi -lclient -lserver
all:
	g++ -c mtcnn.cpp mtcnn_utils.cpp ipc_rtsp.cpp mipi-face.cpp ${INCFLAGS}
	g++ mtcnn.o mtcnn_utils.o ipc_rtsp.o mipi-face.o ${LDDFLAGS} -o start ${LIBDIR}
