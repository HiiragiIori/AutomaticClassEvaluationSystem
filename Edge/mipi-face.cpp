/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2018, Open AI Lab
 * Author: minglu@openailab.com
 *
 */
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include "mtcnn.hpp"
#include "mtcnn_utils.hpp"
#include "ipc_rtsp.hpp"
#include "ptz_control.h"
#include "client.h"
#include "server.h"

using namespace std;

/* MTCNN -- color definitions */
#define COLOR_GREEN fcv::Scalar(0, 255, 0)
#define COLOR_RED   fcv::Scalar(0, 0, 255)
#define COLOR_BLUE  fcv::Scalar(255, 0, 0)
#define COLOR_YELLOW  fcv::Scalar(0, 255, 255)
#define COLOR_MAGENTA fcv::Scalar(255, 0, 255)
#define COLOR_CYAN    fcv::Scalar(255, 255, 0)
#define COLOR_WHITE fcv::Scalar(255, 255, 255)

Mat image;
bool save_flag_ptz = FALSE;

char filename[128];
char dataname[128];
face_box my_box;
Mat whole_pic;
Mat bbox;
void *thread_ptz(void *);

int frame = 0;
int count_img = 0;
int count_ptz = 1;

const string winname = "The Machine";
const string showpic = "Pic";
const string ipcUrl = "rtsp://admin:fpga6666@192.168.123.100:666/Streaming/Channels/1";
const string ipcUser = "admin";
const string ipcPassword = "fpga6666";

const char *serveraddr = "192.168.123.233";
const char *portnum = "233";

typedef struct _VideoDecoder {
    ipcCamera* ipc;
    Mat* image;
} VideoDecoder;

static void usage(char **argv)
{
    printf(
        "Usage: %s [Options]\n\n"
        "Options:\n"
        "-w, --width                  Destination images's width\n"
        "-h, --height                 Destination images's height\n"
        "-r, --rotate                 Image rotation degree, the value should be 90, 180 or 270\n"
        "-V, --vflip                  Vertical Mirror\n"
        "-H, --hflip                  Horizontal Mirror\n"
        "-c, --crop                   Crop, format: x,y,w,h\n"
        "\n",
        argv[0]);
}

static const char *short_options = "d:w:h:r:VHc:";

static struct option long_options[] = {
    {"decoder", required_argument, NULL, 'd'},
    {"width", required_argument, NULL, 'w'},
    {"height", required_argument, NULL, 'h'},
    {"rotate", required_argument, NULL, 'r'},
    {"vflip", no_argument, NULL, 'V'},
    {"hflip", no_argument, NULL, 'H'},
    {"crop", required_argument, NULL, 'c'},
    {NULL, 0, NULL, 0}
};

void *onVideoDecode(void *data)
{
    VideoDecoder *dec = (VideoDecoder*)data;
    ipcCamera *ipc = dec->ipc;
    Mat *mat = dec->image;
    DecFrame *frame = NULL;

    unsigned long long t1, t2;

    while(1) {
        frame = ipc->dequeue();
        if(frame != NULL) {
            t1 = ipc->microTime();
            ipc->rgaProcess(frame, V4L2_PIX_FMT_RGB24, mat);
            t2 = ipc->microTime();
            usleep(2000);
            //printf("rga time: %llums\n", (t2 - t1)/1000);
            ipc->freeFrame(frame);
        }
        usleep(1000);
    }

    return NULL;
}

static void parse_crop_parameters(char *crop, __u32 *cropx, __u32 *cropy, __u32 *cropw, __u32 *croph)
{
    char *p, *buf;
    const char *delims = ".,";
    __u32 v[4] = {0,0,0,0};
    int i = 0;

    buf = strdup(crop);
    p = strtok(buf, delims);
    while(p != NULL) {
        v[i++] = atoi(p);
        p = strtok(NULL, delims);

        if(i >=4)
            break;
    }

    *cropx = v[0];
    *cropy = v[1];
    *cropw = v[2];
    *croph = v[3];

}

void *thread_ptz(void* args)
{
	int i = 0;
	while(1)
	{
		if(save_flag_ptz)
    	{
			PTZ_GO(count_ptz);
        	sprintf(filename, "images/img_%d.jpg", count_img);

			std::cout << "Face Capturing...\n" << std::endl;
			fcv::imwrite(filename, image);
			std::cout << "Face Capture Successful.\n" << std::endl;
			count_ptz ++;
            count_img ++;
            if (count_ptz >= 15)
            {
            	PTZ_GO(CENTER);
			 	count_ptz = 0;
			 	count_img = 0;
				sendAttendancePic(serveraddr, portnum);
				std::cout << "SEND ATTENDANCE PICS START" << std::endl;
               	save_flag_ptz = FALSE;
            }
		}
	}
}

int main(int argc, char **argv)
{
    int ret, ret_ptz, c, r;
    char v4l2_dev[64], isp_dev[64];
    char index = -1;
    pthread_t id;
    pthread_t id_1;
    pthread_t id_ptz;
	pthread_t id_face;

    ret_ptz = pthread_create(&id_ptz, NULL, thread_ptz, NULL);
    if (ret_ptz == 0)
        std::cout << "thread ptz OK!\n";

    struct timeval t0, t1;

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0,&mask);
    
    PTZ_GO(CENTER);

	/* Tengine -- initialization */
	if (init_tengine_library() < 0)
	{
		std::cout << " init tengine failed\n";
		return 1;
	}
	if (request_tengine_version("0.9") < 0)
		return -1;

	/* MTCNN -- default value */
	int min_size = 60;
	float conf_p = 0.6;
	float conf_r = 0.7;
	float conf_o = 0.8;
	float nms_p = 0.5;
	float nms_r = 0.7;
	float nms_o = 0.7;

	std::string model_dir = "./models/";
	std::vector<face_box> face_info;
	
	mtcnn* det = new mtcnn(min_size, conf_p, conf_r, conf_o, nms_p, nms_r, nms_o);
	det->load_3model(model_dir);

    __u32 width = 640, height = 480;
	//__u32 width = 1280, height = 720;
	//__u32 width = 1920, height = 1080;
    RgaRotate rotate = RGA_ROTATE_NONE;
    __u32 cropx = 0, cropy = 0, cropw = 0, croph = 0;
    int vflip = 0, hflip = 0;
    DecodeType type=DECODE_TYPE_H264;

    VideoDecoder* decoder=(VideoDecoder*)malloc(sizeof(VideoDecoder));

    while((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (c) {
        case 'd':
            if(strncmp(optarg, "h264", 5) == 0)
                type = DECODE_TYPE_H264;
            if(strncmp(optarg, "h265", 5) == 0)
                type = DECODE_TYPE_H265;
            break;
        case 'w':
            width = atoi(optarg);
            break;
        case 'h':
            height = atoi(optarg);
            break;
        case 'r':
            r = atoi(optarg);
            switch(r) {
            case 0:
                rotate = RGA_ROTATE_NONE;
                break;
            case 90:
                rotate = RGA_ROTATE_90;
                break;
            case 180:
                rotate = RGA_ROTATE_180;
                break;
            case 270:
                rotate = RGA_ROTATE_270;
                break;
            default:
                printf("roate %d is not supported\n", r);
                return -1;
            }
            break;
        case 'V':
            vflip = 1;
            break;
        case 'H':
            hflip = 1;
            break;
        case 'c':
            parse_crop_parameters(optarg, &cropx, &cropy, &cropw, &croph);
            break;
        default:
            usage(argv);
            return 0;
        }
    }

	printf("decoder=%u, width = %u, height = %u, rotate = %u, vflip = %d, hflip = %d, crop = [%u, %u, %u, %u]\n",
           type, width, height, rotate, vflip, hflip, cropx, cropy, cropw, croph);


	fcv::namedWindow(winname);

    ipcCamera *ipc = new ipcCamera(width, height, rotate, vflip, hflip, cropx, cropy, cropw, croph);

    decoder->ipc = ipc;
    decoder->image = &image;

    RtspClient client(ipcUrl, ipcUser, ipcPassword);
    ret = ipc->init(type);

    image.create(cv::Size(width, height), CV_8UC3);

    if(ret < 0)
        return ret;
    ret = pthread_create(&id_1, NULL, onVideoDecode, decoder);
    if(ret < 0) {
        printf("pthread_create failed\n");
        return ret;
    }

    client.setDataCallback(std::bind(&ipcCamera::onStreamReceive, ipc, std::placeholders::_1, std::placeholders::_2));
    client.enable();
    
	save_flag_ptz = TRUE;

    while(1) {
		
		gettimeofday(&t0, NULL);
        
		/* MTCNN -- detect targets in image */
		face_info.clear();
		det->detect(image, face_info);

        for (unsigned int i = 0; i < face_info.size(); i++)
        {
            my_box = face_info[i];
        }

        save_flag_ptz = TRUE;
        
		frame = 0;
        for (unsigned int i = 0; i < face_info.size(); i++)
		{
			face_box &box = face_info[i];
			std::printf("Face box(%d):( %g , %g ),( %g , %g )\n", i, box.x0, box.y0, box.x1, box.y1);

			//fcv::rectangle(image, fcv::Point(box.x0, box.y0), fcv::Point(box.x1, box.y1), COLOR_RED, 1);
			//for (int l = 0; l < 5; l++)
			//{
			//	fcv::circle(image, fcv::Point(box.landmark.x[l], box.landmark.y[l]), 1, COLOR_MAGENTA, 1);
			//}
			
			fcv::rectangle(image, fcv::Point(box.x0, box.y0), fcv::Point(box.x1, box.y1), COLOR_RED, 1);
			sleep(5);
			fcv::rectangle(image, fcv::Point(box.x0, box.y0), fcv::Point(box.x1, box.y1), COLOR_GREEN, 1);
		}
        
        fcv::imshow(winname, image, NULL);
		gettimeofday(&t1, NULL);

        if(fcv::waitKey(1)=='q')break;
        long elapse = ((t1.tv_sec * 1000000 + t1.tv_usec) - (t0.tv_sec * 1000000 + t0.tv_usec)) / 1000;
    }

	release_tengine_library();
    pthread_join(id, NULL);
	image.release();
	delete ipc;
	free(decoder);
    return 0;
}
