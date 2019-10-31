#ifndef PTZ_CONTROL_H
#define PTZ_CONTROL_H

#ifdef __cplusplus
extern "C"{
#endif

//Coordinates of students-------------------
#define CENTER_x 	0.5
#define CENTER_y 	0.2
#define CENTER_ZOOM	0
//Liu Yongbing
#define NO1_x 		0.1
#define NO1_y 		-0.65
#define NO1_ZOOM	0.1
//Jin Xiaosong
#define NO2_x 		0.35
#define NO2_y 		-0.6
#define NO2_ZOOM	0.2
//Li Li
#define NO3_x 		0.2
#define NO3_y 		-0.5
#define NO3_ZOOM	0.1
//Zhang Jie
#define NO4_x 		0.45
#define NO4_y 		-0.45
#define NO4_ZOOM	0.2
//Huang Jinguo
#define NO5_x		0.2
#define NO5_y		-0.1
#define NO5_ZOOM	0.1
//Wu Mingmei
#define NO6_x		0.3
#define NO6_y		-0.65
#define NO6_ZOOM	0.1
//Wang Yihong
#define NO7_x		0.35
#define NO7_y		-0.3
#define NO7_ZOOM	0.1
//Lian Jiarui
#define NO8_x		0.45
#define NO8_y		-0.45
#define NO8_ZOOM	0.1
//Xiong Guanghong
#define NO9_x		0.5
#define NO9_y		-0.65
#define NO9_ZOOM	0.2
//Chen Linhui
#define NO10_x		0.3
#define NO10_y		-0.75
#define NO10_ZOOM	0.3
//Liang Songhong
#define NO11_x		0.2
#define NO11_y		-0.75
#define NO11_ZOOM	0.2
//He Linlin
#define NO12_x		0.1
#define NO12_y		-0.75
#define NO12_ZOOM	0
//Tang Xue
#define NO13_x		0.3
#define NO13_y		-0.75
#define NO13_ZOOM	0
//Tian Xiaosi
#define NO14_x      0.35
#define NO14_y      -0.5
#define NO14_ZOOM   0
//Coordinates of students-------------------

#define CENTER 	0
#define NO1		1
#define NO2		2
#define NO3		3
#define NO4		4
#define NO5		5
#define NO6		6
#define NO7		7
#define NO8		8
#define NO9		9
#define NO10	10
#define NO11	11
#define NO12	12
#define NO13	13
#define NO14	14

int PTZ_GO(int command);

#ifdef __cplusplus
}
#endif

#endif
