#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "soapH.h"
#include "wsaapi.h"
#include "soapStub.h"
//#include "wsdd.nsmap"
#include "stdsoap2.h"

#define USERNAME    "admin"
#if 0
#define PASSWORD    "goldsun123"
#endif
#define PASSWORD    "fpga6666"


#define SOAP_ASSERT     assert
#define SOAP_DBGLOG     printf
#define SOAP_DBGERR     printf

#define SOAP_TO         "urn:schemas-xmlsoap-org:ws:2005:04:discovery"
#define SOAP_ACTION     "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe"

#define SOAP_MCAST_ADDR "soap.udp://239.255.255.250:3702"// onvif multicast address

#define SOAP_ITEM       ""// scope of device
#define SOAP_TYPES      "dn:NetworkVideoTransmitter"// type of device

#define SOAP_SOCK_TIMEOUT    (1)// socket timeout
#define ONVIF_TOKEN_SIZE     65 // 唯一标识该视频编码器的令牌字符串

#define SOAP_CHECK_ERROR(result, soap, str) \
    do { \
        if (SOAP_OK != (result) || SOAP_OK != (soap)->error) { \
            soap_perror((soap), (str)); \
            if (SOAP_OK == (result)) { \
                (result) = (soap)->error; \
            } \
            goto EXIT;\
        } \
    } while (0)

/*configuration of video encode */
struct tagVideoEncoderConfiguration
{
    char token[ONVIF_TOKEN_SIZE];// video token
    int Width;// resolution ratio
    int Height;
};

/* configuration of device */
struct tagProfile {
    char token[ONVIF_TOKEN_SIZE];// device token

    struct tagVideoEncoderConfiguration venc;// video encoding configuration information
};


//enum tt__CapabilityCategory my_category;

void soap_perror(struct soap *soap, const char *str)
{
    if (NULL == str) {
        SOAP_DBGERR("[soap] error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
    } else {
        SOAP_DBGERR("[soap] %s error: %d, %s, %s\n", str, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
    }
    return;
}

void* ONVIF_soap_malloc(struct soap *soap, unsigned int n)
{
    void *p = NULL;

    if (n > 0) {
        p = soap_malloc(soap, n);
        SOAP_ASSERT(NULL != p);
        memset(p, 0x00 ,n);
    }
    return p;
}

struct soap *ONVIF_soap_new(int timeout)
{
    struct soap *soap = NULL;// onvif soap

    SOAP_ASSERT(NULL != (soap = soap_new()));

    soap_set_namespaces(soap, namespaces);// set soap namespaces
    soap->recv_timeout    = timeout;// timeout
    soap->send_timeout    = timeout;
    soap->connect_timeout = timeout;

#if defined(__linux__) || defined(__linux) // 参考https://www.genivia.com/dev.html#client-c的修改：
    soap->socket_flags = MSG_NOSIGNAL;                                          // To prevent connection reset errors
#endif

    soap_set_mode(soap, SOAP_C_UTFSTRING);// set encoding of character

    return soap;
}

void ONVIF_soap_delete(struct soap *soap)
{
    soap_destroy(soap); // remove deserialized class instances (C++ only)
    soap_end(soap); // Clean up deserialized data (except class instances) and temporary data
    soap_done(soap);// Reset, close communications, and remove callbacks
    soap_free(soap);// Reset and deallocate the context created with soap_new or soap_copy
}

void ONVIF_init_header(struct soap *soap)
{
    struct SOAP_ENV__Header *header = NULL;

    SOAP_ASSERT(NULL != soap);

    header = (struct SOAP_ENV__Header *)ONVIF_soap_malloc(soap, sizeof(struct SOAP_ENV__Header));//in heap
    soap_default_SOAP_ENV__Header(soap, header);
    header->wsa__MessageID = (char*)soap_wsa_rand_uuid(soap);//get uuid
    header->wsa__To        = (char*)ONVIF_soap_malloc(soap, strlen(SOAP_TO) + 1);
    header->wsa__Action    = (char*)ONVIF_soap_malloc(soap, strlen(SOAP_ACTION) + 1);
    strcpy(header->wsa__To, SOAP_TO);
    strcpy(header->wsa__Action, SOAP_ACTION);
    soap->header = header;

    return;
}


void ONVIF_init_ProbeType(struct soap *soap, struct wsdd__ProbeType *probe)
{
    struct wsdd__ScopesType *scope = NULL;// 用于描述查找哪类的Web服务

    SOAP_ASSERT(NULL != soap);
    SOAP_ASSERT(NULL != probe);

    scope = (struct wsdd__ScopesType *)ONVIF_soap_malloc(soap, sizeof(struct wsdd__ScopesType));
    soap_default_wsdd__ScopesType(soap, scope);// 设置寻找设备的范围
    scope->__item = (char*)ONVIF_soap_malloc(soap, strlen(SOAP_ITEM) + 1);
    strcpy(scope->__item, SOAP_ITEM);

    memset(probe, 0x00, sizeof(struct wsdd__ProbeType));
    soap_default_wsdd__ProbeType(soap, probe);
    probe->Scopes = scope;
    probe->Types  = (char*)ONVIF_soap_malloc(soap, strlen(SOAP_TYPES) + 1);// 设置寻找设备的类型
    strcpy(probe->Types, SOAP_TYPES);

    return;
}

static int ONVIF_SetAuthInfo(struct soap *soap, const char *username, const char *password)
{
    int result = 0;

    SOAP_ASSERT(NULL != username);
    SOAP_ASSERT(NULL != password);

    result = soap_wsse_add_UsernameTokenDigest(soap, NULL, username, password);
    SOAP_CHECK_ERROR(result, soap, "add_UsernameTokenDigest");

EXIT:

    return result;
}


void ONVIF_DetectDevice(void (*cb)(char *DeviceXAddr) , char *ip)
{
    int i;
    int result = 0;
    unsigned int count = 0;// count of device
    struct soap *soap = NULL;// soap
    struct wsdd__ProbeType      req;// request
    struct __wsdd__ProbeMatches resp;// respone
    struct wsdd__ProbeMatchType *probeMatch;

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    ONVIF_init_header(soap);// init header
    ONVIF_init_ProbeType(soap, &req);//set scope and type
    result = soap_send___wsdd__Probe(soap, SOAP_MCAST_ADDR, NULL, &req);// send request
    while (SOAP_OK == result)// receive respone
    {
        memset(&resp, 0x00, sizeof(resp));
        result = soap_recv___wsdd__ProbeMatches(soap, &resp);
        if (SOAP_OK == result) {
            if (soap->error) {
                soap_perror(soap, "ProbeMatches");
            }
                else
                {
                    for(i = 0; i < resp.wsdd__ProbeMatches->__sizeProbeMatch; i++)//display information
                    {
                        printf("__sizeProbeMatch        : %d\n", resp.wsdd__ProbeMatches->__sizeProbeMatch);
                        printf("wsa__EndpointReference  : %p\n", resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference);
                        printf("Target EP Address       : %s\n", resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.Address);
                        printf("Target Type             : %s\n", resp.wsdd__ProbeMatches->ProbeMatch->Types);
                        printf("Target Service Address  : %s\n", resp.wsdd__ProbeMatches->ProbeMatch->XAddrs);
                        //memcpy(ip , resp.wsdd__ProbeMatches->ProbeMatch->XAddrs , strlen(resp.wsdd__ProbeMatches->ProbeMatch->XAddrs));
                        printf("Target Metadata Version : %d\n", resp.wsdd__ProbeMatches->ProbeMatch->MetadataVersion);
                        if(resp.wsdd__ProbeMatches->ProbeMatch->Scopes)
                        {
                            printf("Target Scopes Address   : %s\n\n", resp.wsdd__ProbeMatches->ProbeMatch->Scopes->__item);
                        }
                   }
               if (NULL != resp.wsdd__ProbeMatches)
                    {
                        count += resp.wsdd__ProbeMatches->__sizeProbeMatch;
                  for(i = 0; i < resp.wsdd__ProbeMatches->__sizeProbeMatch; i++)
                        {
                            probeMatch = resp.wsdd__ProbeMatches->ProbeMatch + i;
                     if (NULL != cb)
                            {
                                cb(probeMatch->XAddrs);    // run callback function
                     }
                  }
                    }
            }
        }
          else if (soap->error)
          {
            break;
        }
    }

    //SOAP_DBGLOG("\ndetect end! It has detected %d devices!\n", count);
    if (NULL != soap)
     {
        ONVIF_soap_delete(soap);
    }
    return ;
}

//set ip address
int ONVIF_SetNetworkInterfaces(char *old_addr ,char *new_ip , char *device_token)
{
    //char new_addr[128] = {"192.168.1.200"};
    int result = SOAP_ERR;
   if( old_addr != NULL && 0 != strlen(old_addr) )
    {
        struct soap soap;
        soap_init(&soap);
        struct _tds__SetNetworkInterfaces req = {0};
        struct _tds__SetNetworkInterfacesResponse rsp;

        //rsp.RebootNeeded = 1;//设置后重启

        //char interface[32] = "eth0";
        req.InterfaceToken = device_token;//要设置的设备网络接口
        //xsd__boolean dhcp = xsd__boolean__true_;
          //network.Enabled = 1;
        enum xsd__boolean netEnable = xsd__boolean__true_;
        enum xsd__boolean ipv4Enable = xsd__boolean__true_;
        enum xsd__boolean DHCP = xsd__boolean__false_;
        enum xsd__boolean autoneg = xsd__boolean__true_;
        enum xsd__boolean duplex = tt__Duplex__Full;
        struct tt__NetworkInterfaceConnectionSetting net_set;
        net_set.AutoNegotiation = autoneg;
        net_set.Speed = 100;
        net_set.Duplex = duplex;


        struct tt__NetworkInterfaceSetConfiguration network;
        soap_default_tt__NetworkInterfaceSetConfiguration(&soap, &network);
        network.Enabled = &netEnable;
        network.Link = &net_set;

        struct tt__IPv4NetworkInterfaceSetConfiguration tt_ipv4;
        soap_default_tt__IPv4NetworkInterfaceSetConfiguration(&soap, &tt_ipv4);

        struct tt__PrefixedIPv4Address  tt_prefAddr;
        soap_default_tt__PrefixedIPv4Address(&soap, &tt_prefAddr);
        tt_prefAddr.Address = new_ip;//the new ip address
        tt_prefAddr.PrefixLength = 24;

        tt_ipv4.Manual = &tt_prefAddr;
        tt_ipv4.__sizeManual = 1;//important
        tt_ipv4.DHCP = &DHCP;
        tt_ipv4.Enabled = &ipv4Enable;

        network.IPv4 = &tt_ipv4;

        int mtuLen = 1500;
        network.MTU = &mtuLen;

        //printf("%d\n", network.IPv4->Manual->PrefixLength);
        //printf("%s\n", network.IPv4->Manual->Address);
        req.NetworkInterface = &network;

        ONVIF_SetAuthInfo(&soap, USERNAME, PASSWORD);
        result = soap_call___tds__SetNetworkInterfaces(&soap,old_addr, NULL, &req, &rsp);
        if(result == SOAP_OK)
        {
            printf("SetNetworkInterfaces successful\n");
        }
        soap_destroy(&soap);
        soap_end(&soap);
        soap_done(&soap);
    }
    return result;


}


int ONVIF_GetDeviceInformation(const char *DeviceXAddr)
{
    int result = 0;
    struct soap *soap = NULL;
    struct _tds__GetDeviceInformation           devinfo_req;
    struct _tds__GetDeviceInformationResponse   devinfo_resp;

    SOAP_ASSERT(NULL != DeviceXAddr);
    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    memset(&devinfo_req, 0x00, sizeof(devinfo_req));
    memset(&devinfo_resp, 0x00, sizeof(devinfo_resp));
    result = soap_call___tds__GetDeviceInformation(soap, DeviceXAddr, NULL, &devinfo_req, &devinfo_resp);
    SOAP_CHECK_ERROR(result, soap, "GetDeviceInformation");
     printf("\n\n");
     printf("SerialNumber number is :%d\n" , devinfo_resp.SerialNumber );

    EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }
    return result;
}

int ONVIF_GetCapabilities(const char *device_server , char *media_ip , char *ptz_ip)
{
    int result = 0;
    struct soap *soap = NULL;
    struct _tds__GetCapabilities            req;
    struct _tds__GetCapabilitiesResponse    resp;

    SOAP_ASSERT(NULL != device_server);
    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));
    memset(&req, 0x00, sizeof(req));

    memset(&resp, 0x00, sizeof(resp));
    req.__sizeCategory = 1;
    req.Category = (enum tt__CapabilityCategory*)soap_malloc (soap, sizeof(int));
    *(req.Category) = (enum tt__CapabilityCategory)5/*(tt__CapabilityCategory__Media)*/;

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    result = soap_call___tds__GetCapabilities(soap, device_server, NULL, &req, &resp);
    printf("\n\n");
    //printf("result is :%d\n", result);
    SOAP_CHECK_ERROR(result, soap, "GetCapabilities");
    if(strlen(resp.Capabilities->Media->XAddr) > 128)
    {
        return -1;
    }
    else
    {
        memcpy(media_ip , resp.Capabilities->Media->XAddr , strlen(resp.Capabilities->Media->XAddr));
    }
    printf("media addr is                 :%s\n" , resp.Capabilities->Media->XAddr);
    /*get PTZ address*/
    req.__sizeCategory = 1;
    req.Category = (enum tt__CapabilityCategory*)soap_malloc (soap, sizeof(int));
    *(req.Category) = (enum tt__CapabilityCategory)6/*(tt__CapabilityCategory__PTZ)*/;
    result = soap_call___tds__GetCapabilities(soap, device_server, NULL, &req, &resp);
    printf("\n\n");
    SOAP_CHECK_ERROR(result, soap, "GetCapabilities");
#if 1
    if(resp.Capabilities->PTZ == NULL)
    {
        printf("unsurpport PTZ\n");
        return -1;
    }
    if(strlen(resp.Capabilities->PTZ->XAddr) > 128)
    {
        return -1;
    }
    else
    {
        //memcpy(ptz_ip , resp.Capabilities->PTZ->XAddr , strlen(resp.Capabilities->PTZ->XAddr));
    }
    //printf("PTZ addr is                   :%s\n" , resp.Capabilities->PTZ->XAddr);
#endif
EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }
    return result;
}

int ONVIF_GetZeroConfiguration(char *addr , char *device_token)
{
    int result = 0;
    struct soap *soap = NULL;
    struct _tds__GetZeroConfiguration  req;
    struct _tds__GetZeroConfigurationResponse resp;

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));
    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    result = soap_call___tds__GetZeroConfiguration(soap , addr , NULL , &req , &resp);
    if(strlen(resp.ZeroConfiguration->InterfaceToken)  > 16)
    {
        return -1;
    }
    else
    {
        memcpy(device_token , resp.ZeroConfiguration->InterfaceToken , strlen(resp.ZeroConfiguration->InterfaceToken));
    }
    printf("\ntoken is       :%s\n" , resp.ZeroConfiguration->InterfaceToken);
    ONVIF_soap_delete(soap);
    return result;

}

int ONVIF_GetStreamUri(const char *MediaXAddr, char *ProfileToken)
{
    int result = 0;
    char uri[128] = {0};
    struct soap *soap = NULL;
    struct tt__StreamSetup              ttStreamSetup;
    struct tt__Transport                ttTransport;
    struct _trt__GetStreamUri           req;
    struct _trt__GetStreamUriResponse   resp;

    SOAP_ASSERT(NULL != MediaXAddr);

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    memset(&req, 0x00, sizeof(req));
    memset(&resp, 0x00, sizeof(resp));
    memset(&ttStreamSetup, 0x00, sizeof(ttStreamSetup));
    memset(&ttTransport, 0x00, sizeof(ttTransport));
    ttStreamSetup.Stream                = tt__StreamType__RTP_Unicast;
    ttStreamSetup.Transport             = &ttTransport;
    ttStreamSetup.Transport->Protocol   = tt__TransportProtocol__RTSP;
    ttStreamSetup.Transport->Tunnel     = NULL;
    req.StreamSetup                     = &ttStreamSetup;
    req.ProfileToken                    = ProfileToken;

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);
    result = soap_call___trt__GetStreamUri(soap, MediaXAddr, NULL, &req, &resp);
    SOAP_CHECK_ERROR(result, soap, "GetServices");
    printf("uri is               :%s\n", resp.MediaUri->Uri);

EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }
    return result;
}
/*get media profile*/
int ONVIF_Media_GetProfiles(const char *Media_ip)
{
    int i = 0;
    int result = 0;
    int stmno = 0;
    struct soap *soap = NULL;
    struct tagProfile *profiles = NULL;
    struct _trt__GetProfiles            req;
    struct _trt__GetProfilesResponse    resp;

    SOAP_ASSERT(NULL != Media_ip);
    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    memset(&req, 0x00, sizeof(req));
    memset(&resp, 0x00, sizeof(resp));
    result = soap_call___trt__GetProfiles(soap, Media_ip, NULL, &req, &resp);
    printf(">>>result is :%d\n" , result);
    SOAP_CHECK_ERROR(result, soap, "GetProfiles");
     //printf information of profiles
     printf("\n\n");
     printf("__sizeprofiles is  :%d\n" , resp.__sizeProfiles);
     printf("name is            :%s\n" , resp.Profiles->Name);
     printf("token is           :%s\n" , resp.Profiles->VideoSourceConfiguration->token);
     printf("sourcetoken is     :%s\n" , resp.Profiles->VideoSourceConfiguration->SourceToken);
     printf("usecoutn is        :%d\n" , resp.Profiles->VideoEncoderConfiguration->UseCount);
     //printf("__size is          :%d\n" , resp.Profiles->VideoEncoderConfiguration->__size);
     //printf("__any is           :%d\n" , resp.Profiles->VideoEncoderConfiguration->__any );
     printf("profile token is   :%s\n" , resp.Profiles->token);
     printf("\n\n");
    if (resp.__sizeProfiles > 0)
    {
        profiles = (struct tagProfile *)malloc(resp.__sizeProfiles * sizeof(struct tagProfile));//malloc cache
        SOAP_ASSERT(NULL != profiles);
        memset(profiles, 0x00, resp.__sizeProfiles * sizeof(struct tagProfile));
    }
    /*resolve respone*/
    for(i = 0; i < resp.__sizeProfiles; i++) {
        struct tt__Profile *RespProfile = &resp.Profiles[i];
        struct tagProfile *StoreProfile = &profiles[i];

        if (NULL != RespProfile->token)// store Token
        {
            strncpy(StoreProfile->token, RespProfile->token, sizeof(StoreProfile->token) - 1);
        }

        if (NULL != RespProfile->VideoEncoderConfiguration) //video encode configuration
        {
            if (NULL != RespProfile->VideoEncoderConfiguration->token) // store video Token
            {
                strncpy(StoreProfile->venc.token, RespProfile->VideoEncoderConfiguration->token, sizeof(StoreProfile->venc.token) - 1);
            }
            if (NULL != RespProfile->VideoEncoderConfiguration->Resolution) // video resolutio
            {
                StoreProfile->venc.Width  = RespProfile->VideoEncoderConfiguration->Resolution->Width;
                StoreProfile->venc.Height = RespProfile->VideoEncoderConfiguration->Resolution->Height;
            }
        }
          ONVIF_GetStreamUri(Media_ip , profiles[i].token);
          printf("profile token is     :%s\n" , StoreProfile->token);
          printf("video token is       :%s\n" , StoreProfile->venc.token);
          printf("video width is       :%d\n" , StoreProfile->venc.Width);
          printf("video height is      :%d\n" , StoreProfile->venc.Height);
          if(RespProfile->VideoEncoderConfiguration->Encoding == 0)
          {
                printf("video encode is      :JPEG\n");
          }
          if(RespProfile->VideoEncoderConfiguration->Encoding == 1)
          {
                printf("video encode is      :MPEG4\n");
          }
          if(RespProfile->VideoEncoderConfiguration->Encoding == 2)
          {
                printf("video encode is      :H264\n");
          }
            printf("\n");
    }

     return resp.__sizeProfiles;
EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }

    return resp.__sizeProfiles;
}
/*get PTZ profiles*/
int ONVIF_PTZ_GetProfiles(const char *media_ip , char *absolute_ip , char *zoom_ip)
{
    int i = 0;
    int result = 0;
    struct soap *soap = NULL;
    struct tagProfile *profiles = NULL;
    struct _trt__GetProfiles            req;
    struct _trt__GetProfilesResponse    resp;

    SOAP_ASSERT(NULL != media_ip);
    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    memset(&req, 0x00, sizeof(req));
    memset(&resp, 0x00, sizeof(resp));
    result = soap_call___trt__GetProfiles(soap, media_ip, NULL, &req, &resp);
    printf("result is :%d\n" ,result);
    SOAP_CHECK_ERROR(result, soap, "GetProfiles");
     //printf information of profiles
     printf("\n\n");
     printf("__sizeprofiles is                             :%d\n" , resp.__sizeProfiles);
     printf("name is                                       :%s\n" , resp.Profiles->Name);
     printf("token is                                      :%s\n" , resp.Profiles->PTZConfiguration->token);
     printf("NodeToken is                                  :%s\n" , resp.Profiles->PTZConfiguration->NodeToken);
     printf("UseCount is                                   :%d\n" , resp.Profiles->PTZConfiguration->UseCount);
     printf("DefaultAbsolutePantTiltPositionSpace is       :%d\n" , resp.Profiles->PTZConfiguration->DefaultAbsolutePantTiltPositionSpace);
     printf("DefaultAbsoluteZoomPositionSpace is           :%d\n" , resp.Profiles->PTZConfiguration->DefaultAbsoluteZoomPositionSpace);
     printf("DefaultRelativePanTiltTranslationSpace is     :%d\n" , resp.Profiles->PTZConfiguration->DefaultRelativePanTiltTranslationSpace );
     printf("DefaultRelativeZoomTranslationSpace is        :%s\n" , resp.Profiles->PTZConfiguration->DefaultRelativeZoomTranslationSpace);
     printf("DefaultRelativeZoomTranslationSpace is        :%s\n" , resp.Profiles->PTZConfiguration->DefaultRelativeZoomTranslationSpace);
     printf("DefaultContinuousPanTiltVelocitySpace is      :%s\n" , resp.Profiles->PTZConfiguration->DefaultContinuousPanTiltVelocitySpace);
     printf("DefaultContinuousZoomVelocitySpace is         :%s\n" , resp.Profiles->PTZConfiguration->DefaultContinuousZoomVelocitySpace);
     printf("DefualtPTZTimeout is                          :%d\n" , *(resp.Profiles->PTZConfiguration->DefaultPTZTimeout));
     printf("DefaultPTZSpeed Vector2D PanTilt x is         :%f\n" , resp.Profiles->PTZConfiguration->DefaultPTZSpeed->PanTilt->x);
     printf("DefaultPTZSpeed dVector2D PanTilt y is        :%f\n" , resp.Profiles->PTZConfiguration->DefaultPTZSpeed->PanTilt->y);
     printf("DefaultPTZSpeed dVector2D PanTilt space is    :%s\n" , resp.Profiles->PTZConfiguration->DefaultPTZSpeed->PanTilt->space);
     memcpy(absolute_ip ,resp.Profiles->PTZConfiguration->DefaultPTZSpeed->PanTilt->space , strlen(resp.Profiles->PTZConfiguration->DefaultPTZSpeed->PanTilt->space));
     printf("DefaultPTZSpeed Vector1D Zoom x is            :%f\n" , resp.Profiles->PTZConfiguration->DefaultPTZSpeed->Zoom->x);
     printf("DefaultPTZSpeed Vector2D Zoom space is        :%s\n" , resp.Profiles->PTZConfiguration->DefaultPTZSpeed->Zoom->space);
     printf("PanTiltLimits Range  URI is                   :%s\n" ,resp.Profiles->PTZConfiguration->PanTiltLimits->Range->URI);
     printf("PanTiltLimits Range  XRange Min  is           :%f\n" ,resp.Profiles->PTZConfiguration->PanTiltLimits->Range->XRange->Min);
     printf("PanTiltLimits Range  XRange Max is            :%f\n" ,resp.Profiles->PTZConfiguration->PanTiltLimits->Range->XRange->Max);
     printf("PanTiltLimits Range  YRange Min  is           :%f\n" ,resp.Profiles->PTZConfiguration->PanTiltLimits->Range->YRange->Min);
     printf("PanTiltLimits Range  YRange Max is            :%f\n" ,resp.Profiles->PTZConfiguration->PanTiltLimits->Range->YRange->Max);
     printf("ZoomLimits Range  URI is                      :%s\n" ,resp.Profiles->PTZConfiguration->ZoomLimits->Range->URI);
     memcpy(zoom_ip , resp.Profiles->PTZConfiguration->ZoomLimits->Range->URI , strlen(resp.Profiles->PTZConfiguration->ZoomLimits->Range->URI));
     printf("ZoomLimits Range  XRange Min is               :%f\n" ,resp.Profiles->PTZConfiguration->ZoomLimits->Range->XRange->Min);
     printf("ZoomLimits Range  XRange Max is               :%f\n" ,resp.Profiles->PTZConfiguration->ZoomLimits->Range->XRange->Max);

     printf("\n\n");
#if 0
    if (resp.__sizeProfiles > 0)
    {                                               // 分配缓存
        profiles = (struct tagProfile *)malloc(resp.__sizeProfiles * sizeof(struct tagProfile));
        SOAP_ASSERT(NULL != profiles);
        memset(profiles, 0x00, resp.__sizeProfiles * sizeof(struct tagProfile));
    }

    for(i = 0; i < resp.__sizeProfiles; i++) {                                   // 提取所有配置文件信息
        struct tt__Profile *RespProfile = &resp.Profiles[i];
        struct tagProfile *StoreProfile = &profiles[i];

        if (NULL != RespProfile->token)// store Token
        {
            strncpy(StoreProfile->token, RespProfile->token, sizeof(StoreProfile->token) - 1);
        }

        if (NULL != RespProfile->VideoEncoderConfiguration) //video encode configuration
        {
            if (NULL != RespProfile->VideoEncoderConfiguration->token) // store video Token
            {
                strncpy(StoreProfile->venc.token, RespProfile->VideoEncoderConfiguration->token, sizeof(StoreProfile->venc.token) - 1);
            }
            if (NULL != RespProfile->VideoEncoderConfiguration->Resolution) // video resolutio
            {
                StoreProfile->venc.Width  = RespProfile->VideoEncoderConfiguration->Resolution->Width;
                StoreProfile->venc.Height = RespProfile->VideoEncoderConfiguration->Resolution->Height;
            }
        }
          ONVIF_GetStreamUri(MediaXAddr , profiles[i].token);
          printf("profile token is     :%s\n" , StoreProfile->token);
          printf("video token is       :%s\n" , StoreProfile->venc.token);
          printf("video width is       :%d\n" , StoreProfile->venc.Width);
          printf("video height is      :%d\n" , StoreProfile->venc.Height);
          if(RespProfile->VideoEncoderConfiguration->Encoding == 0)
          {
                printf("video encode is      :JPEG\n");
          }
          if(RespProfile->VideoEncoderConfiguration->Encoding == 1)
          {
                printf("video encode is      :MPEG4\n");
          }
          if(RespProfile->VideoEncoderConfiguration->Encoding == 2)
          {
                printf("video encode is      :H264\n");
          }
            printf("\n");
    }
#endif
     return resp.__sizeProfiles;
EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }

    return resp.__sizeProfiles;
}


int ONVIF_Get_Gateway(char *device_ip)
{
    int result = -1;
    struct soap *soap = NULL;// point of soap
    struct _tds__GetNetworkDefaultGateway      req;//requst
    struct _tds__GetNetworkDefaultGatewayResponse resp;// respone

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);
    result = soap_call___tds__GetNetworkDefaultGateway(soap , device_ip , NULL , &req , &resp);

    printf("result is :%d\n" , result);
#if 1
    if(*(resp.NetworkGateway->IPv4Address) == NULL)
    {
        printf("can't get gateway address \n");
        return -1;
    }
    else
    {
        printf("the gateway address is          :%s\n" , *(resp.NetworkGateway->IPv4Address));
    }
#endif
    ONVIF_soap_delete(soap);
    return 0;
}

int ONVIF_SetNetworkDefaultGateway(char *device_ip)
{
    char new_gateway[128] = {0};
    char *ip_p = new_gateway;
#if 1
    printf("please input new gateway:");
    scanf("%s" , new_gateway);
    if(strcmp(new_gateway , "q") == 0)
    {
        return 0;
    }
    printf("the new gateway is :%s\n" , new_gateway);
#endif

    int result = -1;
    int size = 0;
    struct soap *soap = NULL;// point of soap
    struct _tds__SetNetworkDefaultGateway req;//requst
    struct _tds__SetNetworkDefaultGatewayResponse resp;// respone

    //size = strlen(ip_p);
    req.IPv4Address = &ip_p;
    req.__sizeIPv4Address = 1;

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));
    ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    result = soap_call___tds__SetNetworkDefaultGateway(soap , device_ip , NULL , &req ,&resp);
    printf("result is                 :%d\n" , result);

    ONVIF_soap_delete(soap);
}

#if 1
int ONVIF_AbsoluteMove(char *ptz_ip , char *absolute_move_ip  , char *absolute_zoom_ip)
{

    int result = -1;
    struct soap *soap = NULL;// pointer of soap
    struct _tptz__AbsoluteMove req;
    struct _tptz__AbsoluteMoveResponse resp;

    struct tt__Vector2D vector2d;
    vector2d.x = -1;//absolute x angle
    vector2d.y = -1;//absolute y angle
    vector2d.space = absolute_move_ip;
#if 1
    struct tt__Vector1D vector1d;
    vector1d.x = 0;//absolute foci
    vector1d.space = absolute_zoom_ip;
#endif

    struct tt__PTZVector ptzvector;
    ptzvector.PanTilt = &vector2d;
    ptzvector.Zoom = &vector1d;

    req.Position = &ptzvector;
    req.ProfileToken = "Profile_1";
    req.Speed = NULL;

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    result = ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    printf("ONvif_SetAuthInfo is :%d\n" , result);

    result =soap_call___tptz__AbsoluteMove(soap , ptz_ip , NULL , &req , &resp);
    sleep(1);
    ONVIF_soap_delete(soap);
    printf("result is            :%d\n" , result);
    return result;
}
#endif

#if 0
#endifSOAP_FMAC5 int SOAP_FMAC6 soap_call___tptz__RelativeMove(struct soap *soap, const char *soap_endpoint, const char *soap_action, struct _tptz__RelativeMove *tptz__RelativeMove, struct _tptz__RelativeMoveResponse *tptz__RelativeMoveResponse)
#endif

int ONVIf_RelativeMove(char *tpz_ip , char *relative_move_ip  , char *relative_zoom_ip )
{
    int result = -1;
    struct soap *soap = NULL;// pointer of soap
    struct _tptz__RelativeMove req;
    struct _tptz__RelativeMoveResponse resp;

    struct tt__Vector2D vector2d;
    vector2d.x = 0.3;
    vector2d.y = 0.4;
    vector2d.space = relative_move_ip;

    struct tt__Vector1D vector1d;
    vector1d.x = 0.5;
    vector1d.space = relative_zoom_ip;

    struct tt__PTZVector ptzvector;
    ptzvector.PanTilt = &vector2d;
    ptzvector.Zoom = &vector1d;

    req.ProfileToken = "Profile_1";
    req.Translation = &ptzvector;
    req.Speed = NULL;


    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    result = ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);
    printf("ONvif_SetAuthInfo is :%d\n" , result);

    result = soap_call___tptz__RelativeMove(soap , tpz_ip , NULL , &req ,&resp);

    ONVIF_soap_delete(soap);

    printf("result is               :%d\n" , result);
    return result;
}

int ONVIF_Stop()
{

    char *ptz_ip = "http://192.168.123.100/onvif/PTZ";
    int result = -1;
    struct soap *soap = NULL;// pointer of soap
    struct _tptz__Stop req;
    struct _tptz__StopResponse resp;

    enum xsd__boolean stop_rote = xsd__boolean__true_;
    enum xsd__boolean stop_foci = xsd__boolean__false_;

    req.PanTilt = &stop_rote;
    req.Zoom = &stop_foci;
    req.ProfileToken = "Profile_1";

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));
    result = ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);

    printf("ONvif_SetAuthInfo is :%d\n" , result);
    result = soap_call___tptz__Stop(soap , ptz_ip , NULL , &req , &resp);

    ONVIF_soap_delete(soap);
}

int ONVIF_ContinuousMove(char *ptz_ip , char *continuous_move_ip , char *continuous_zoom_ip)
{
    int result = -1;
    struct soap *soap = NULL;// pointer of soap
    struct _tptz__ContinuousMove req;
    struct _tptz__ContinuousMoveResponse resp;

    struct tt__Vector2D vector2d;
    vector2d.x = 1;
    vector2d.y = 0;
    vector2d.space = continuous_move_ip;

    struct tt__Vector1D vector1d;
    vector1d.x = 0;
    vector1d.space = continuous_zoom_ip;

    struct tt__PTZSpeed ptzspeed;
    ptzspeed.PanTilt = &vector2d;
    ptzspeed.Zoom = &vector1d;

    req.ProfileToken = "Profile_1";
    req.Velocity = &ptzspeed;


    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    result = ONVIF_SetAuthInfo(soap, USERNAME, PASSWORD);
    printf("ONvif_SetAuthInfo is :%d\n" , result);
    result = soap_call___tptz__ContinuousMove(soap , ptz_ip , NULL , &req , &resp);
    printf("result is                     :%d\n" , result);

    ONVIF_soap_delete(soap);
    return result;

}

void sig_fun(int signum)
{
    printf("stop roting \n");
    ONVIF_Stop();
    exit(1);
}


int main(int argc, char **argv)
{
#if 0
    signal(SIGINT , sig_fun);
#endif

    //char device_ip[128] = {0};
    char *device_ip = "http://192.168.123.100/onvif/device_service";
    char media_ip[128] = {0};
    char new_ip[128] = {0};
    //char ptz_ip[128] = {0};
    char *ptz_ip = "http://192.168.123.100/onvif/PTZ";
    char device_token[16] = {0};
    char absolute_ip[128] = {0};
    char *absolute_move_ip = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace";
    char zoom_ip[128] = {0};
    char *absolute_zoom_ip = "http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace";
    //char *addr = NULL;
    //struct tagProfile *profiles = NULL;
    char *relative_move_ip = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace";
    char *relative_zoom_ip = "http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace";

    char *continuous_move_ip = "http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace";
    char *continuous_zoom_ip = "http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace";
    int result = 0;

    //ONVIF_Get_Gateway(device_ip);

    ONVIF_SetNetworkDefaultGateway(device_ip);

#if 1
#if 0
    char device_service_ip[128] = {"http://192.168.235.200/onvif/device_service"};
#endif
    /*get device ip*/
    ONVIF_DetectDevice(NULL , device_ip);
    printf("device ip is :%s\n" , device_ip);
    /*get device token*/
    //ONVIF_GetZeroConfiguration(device_ip , device_token);

#if 1
    //printf("\n\n");
    printf("please input new ip :");
    scanf("%s" , new_ip);
    printf("new_ip is :%s\n" , new_ip);
    if(strcmp(new_ip , "q") != 0)
    {
        result = ONVIF_SetNetworkInterfaces(device_ip , new_ip , device_token);
        printf("result of set ip is           :%d\n" , result);
        sleep(10);
        ONVIF_DetectDevice(NULL , device_ip);
        printf("\n\n");
    }
#endif
    //get device info
    ONVIF_GetDeviceInformation(device_ip);
    //get capabilities
    ONVIF_GetCapabilities(device_ip , media_ip , ptz_ip);
    printf("ptz_ip is                 :%s\n" , ptz_ip);
    //get profiles
    result = ONVIF_Media_GetProfiles(media_ip);
    result = ONVIF_PTZ_GetProfiles(media_ip , absolute_ip , zoom_ip);
    printf("continuous_move_ip is :%s\n" , absolute_ip);
    printf("continuous_zoom_ip is   :%s\n" , zoom_ip);
#endif
//#if 1
    //ONVIF_AbsoluteMove(ptz_ip , absolute_move_ip , absolute_zoom_ip);
    //printf("result is :%d\n" , result);
//#endif

//#if 1
    //ONVIf_RelativeMove(ptz_ip , relative_move_ip , relative_zoom_ip);
//#endif

#if 1
    ONVIF_ContinuousMove(ptz_ip , continuous_move_ip , continuous_zoom_ip);
    sleep(2);
    ONVIF_Stop();
	sleep(2);
	ONVIF_ContinuousMove(ptz_ip , continuous_move_ip , continuous_zoom_ip);
    sleep(2);
    ONVIF_Stop();
    //while(1);
    return 0;
#endif
}
