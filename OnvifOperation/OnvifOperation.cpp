// OnvifOperation.cpp : Defines the exported functions for the DLL application.
//

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include "OnvifOperation.h"

#include "soapH.h"
#include "wsdd.h"
#include "wsseapi.h"
#include "wsaapi.h"

#include <iostream>
#include <string>
#include <vector>
#include <regex>
using namespace std;
// C++ 11

//static soap*            pSoap;
//static soap*            SoapForSearch;
static wsdd__ScopesType scopes;
static SOAP_ENV__Header header;

static bool initialsuccess = true;

ONVIFOPERATION_API int init_DLL(void)
{
    //pSoap = soap_new1(SOAP_IO_DEFAULT | SOAP_XML_IGNORENS); // ignore namespace, avoid namespace mismatch
    //if(NULL == pSoap)
    //{
    //    return -1;
    //}

    //SoapForSearch = soap_new1(SOAP_IO_DEFAULT | SOAP_XML_IGNORENS); // ignore namespace, avoid namespace mismatch
    //if(NULL == pSoap)
    //{
    //    return -1;
    //}

    //initialsuccess = true;

    return 0;
}

ONVIFOPERATION_API int uninit_DLL(void)
{
    //if(!initialsuccess)
    //{
    //    return -1;
    //}

    //soap_destroy(SoapForSearch);
    //soap_end(SoapForSearch);
    //soap_done(SoapForSearch);

    //soap_destroy(pSoap);
    //soap_end(pSoap);
    //soap_done(SoapForSearch);

    //SoapForSearch = NULL;
    //pSoap = NULL;

    //initialsuccess = false;

    return 0;
}

ONVIFOPERATION_API onvif_device_list* malloc_device_list(void)
{
    onvif_device_list* p_onvif_device_list_temp = (onvif_device_list*)malloc(sizeof(onvif_device_list));

    if(NULL != p_onvif_device_list_temp)
    {
        p_onvif_device_list_temp->number_of_onvif_devices = 0;
        p_onvif_device_list_temp->devcie_list_lock = false;
        p_onvif_device_list_temp->p_onvif_devices = (onvif_device*)malloc(1);
    }
    else if(NULL == p_onvif_device_list_temp->p_onvif_devices)
    {
        free(p_onvif_device_list_temp);
        p_onvif_device_list_temp = NULL;
    }

    return p_onvif_device_list_temp;
}

ONVIFOPERATION_API void free_device_list(onvif_device_list** pp_onvif_device_list)
{
    if(NULL == pp_onvif_device_list || NULL == (*pp_onvif_device_list))
    {
        return;
    }
    else
    {
        for(size_t i = 0; i < (*pp_onvif_device_list)->number_of_onvif_devices; ++i)
        {
            if(NULL != (*pp_onvif_device_list)->p_onvif_devices[i].p_onvif_ipc_profiles)
            {
                free((*pp_onvif_device_list)->p_onvif_devices[i].p_onvif_ipc_profiles);
            }
        }
        for(size_t i = 0; i < (*pp_onvif_device_list)->number_of_onvif_devices; ++i)
        {
            if(NULL != (*pp_onvif_device_list)->p_onvif_devices[i].p_onvif_NVR_receivers)
            {
                free((*pp_onvif_device_list)->p_onvif_devices[i].p_onvif_NVR_receivers);
            }
        }
        if(NULL != (*pp_onvif_device_list)->p_onvif_devices)
        {
            free((*pp_onvif_device_list)->p_onvif_devices);
        }
        free((*pp_onvif_device_list));
        (*pp_onvif_device_list) = NULL;
    }
}

ONVIFOPERATION_API int search_onvif_device(onvif_device_list* p_onvif_device_list, int wait_time)
{
    vector<string>              device_service_address_list;
    vector<string>              device_IPv4_list;
    onvif_device*               p_onvif_device_temp;
    size_t                      i;
    size_t                      j;
    wsdd__ProbeType             probe;
    struct __wsdd__ProbeMatches probeMatches;
    regex                       expression("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
    smatch                      match;
    soap                        SoapForSearch;

    if(!initialsuccess || NULL == p_onvif_device_list)
    {
        return -1;
    }

    soap_init1(&SoapForSearch, SOAP_IO_DEFAULT | SOAP_XML_IGNORENS);

    soap_default_SOAP_ENV__Header(&SoapForSearch, &header);
    soap_set_namespaces(&SoapForSearch, discovery_namespace);
    SoapForSearch.recv_timeout = wait_time;

    header.wsa__MessageID = (char*)soap_wsa_rand_uuid(&SoapForSearch);
    if(NULL == header.wsa__MessageID)
    {
        soap_destroy(&SoapForSearch);
        soap_end(&SoapForSearch);
        soap_done(&SoapForSearch);
        return -1;
    }

    header.wsa__To = "urn:schemas-xmlsoap-org:ws:2005:04:discovery";
    header.wsa__Action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
    SoapForSearch.header = &header;

    soap_default_wsdd__ScopesType(&SoapForSearch, &scopes);
    scopes.__item = ""; //onvif://www.onvif.org
    soap_default_wsdd__ProbeType(&SoapForSearch, &probe);
    probe.Scopes = &scopes;
    probe.Types = "NetworkVideoTransmitter"; /*ns1:NetworkVideoTransmitter*/
    //probe.Types = "";

    if(SOAP_OK != soap_send___wsdd__Probe(&SoapForSearch, "soap.udp://239.255.255.250:3702", NULL, &probe))
    {
        soap_destroy(&SoapForSearch);
        soap_end(&SoapForSearch);
        soap_done(&SoapForSearch);
        return -1;
    }

    // get match result and put into vector
    while(TRUE)
    {
        if(SOAP_OK != soap_recv___wsdd__ProbeMatches(&SoapForSearch, &probeMatches))
        {
            break;
        }
        else
        {
            if(NULL == probeMatches.wsdd__ProbeMatches)
            {
                continue;
            }
            if(NULL == probeMatches.wsdd__ProbeMatches->ProbeMatch)
            {
                continue;
            }
            if(NULL == probeMatches.wsdd__ProbeMatches->ProbeMatch->XAddrs)
            {
                continue;
            }

            device_service_address_list.push_back(probeMatches.wsdd__ProbeMatches->ProbeMatch->XAddrs);
        }
    }

    // get IPv4
    device_IPv4_list.resize(device_service_address_list.size());
    for(i = 0; i < device_IPv4_list.size(); ++i)
    {
        auto words_begin = sregex_iterator(device_service_address_list[i].begin(), device_service_address_list[i].end(), expression);
        auto words_end = sregex_iterator();

        for(sregex_iterator iterator = words_begin; iterator != words_end; ++iterator)
        {
            match = *iterator;
        }

        device_IPv4_list[i] = match.str();
    }

    //check lock
    while(p_onvif_device_list->devcie_list_lock)
    {
        Sleep(10);
    }
    // add lock
    p_onvif_device_list->devcie_list_lock = true;

    // set all to not duplicated
    for(i = 0; i < p_onvif_device_list->number_of_onvif_devices; ++i)
    {
        p_onvif_device_list->p_onvif_devices[i].duplicated = false;
    }

    // add new device into list
    for(i = 0; i < device_IPv4_list.size(); ++i)
    {
        // find device already in the list and set to duplicated
        for(j = 0; j < p_onvif_device_list->number_of_onvif_devices; ++j)
        {
            if(0 == strncmp(device_IPv4_list[i].c_str(), p_onvif_device_list->p_onvif_devices[j].IPv4, 256))
            {
                p_onvif_device_list->p_onvif_devices[j].duplicated = true;
                break;
            }
        }
        // if not found, it is new device
        if(j == p_onvif_device_list->number_of_onvif_devices)
        {
            p_onvif_device_list->number_of_onvif_devices += 1;
            p_onvif_device_temp = (onvif_device*)realloc(p_onvif_device_list->p_onvif_devices, p_onvif_device_list->number_of_onvif_devices * sizeof(onvif_device));
            if(NULL == p_onvif_device_temp)
            {
                p_onvif_device_list->devcie_list_lock = false;
                soap_destroy(&SoapForSearch);
                soap_end(&SoapForSearch);
                soap_done(&SoapForSearch);
                return -1;
            }
            else
            {
                p_onvif_device_list->p_onvif_devices = p_onvif_device_temp;
                memset(&p_onvif_device_list->p_onvif_devices[p_onvif_device_list->number_of_onvif_devices - 1], 0x0, sizeof(onvif_device));
                strncpy(
                    p_onvif_device_list->p_onvif_devices[p_onvif_device_list->number_of_onvif_devices - 1].service_address_device_service.xaddr,
                    device_service_address_list[i].c_str(),
                    256);
                strncpy(
                    p_onvif_device_list->p_onvif_devices[p_onvif_device_list->number_of_onvif_devices - 1].IPv4,
                    device_IPv4_list[i].c_str(),
                    17);
                p_onvif_device_list->p_onvif_devices[p_onvif_device_list->number_of_onvif_devices - 1].duplicated = true;
            }
        }
    }

    // remove old disappeared device
    for(i = 0; i < p_onvif_device_list->number_of_onvif_devices; ++i)
    {
        if(!p_onvif_device_list->p_onvif_devices[i].duplicated)
        {
            // remove profiles array
            if(NULL != p_onvif_device_list->p_onvif_devices[i].p_onvif_ipc_profiles)
            {
                free(p_onvif_device_list->p_onvif_devices[i].p_onvif_ipc_profiles);
                p_onvif_device_list->p_onvif_devices[i].p_onvif_ipc_profiles = NULL;
            }

            // remove NVR receivers array
            if(NULL != p_onvif_device_list->p_onvif_devices[i].p_onvif_NVR_receivers)
            {
                free(p_onvif_device_list->p_onvif_devices[i].p_onvif_NVR_receivers);
                p_onvif_device_list->p_onvif_devices[i].p_onvif_NVR_receivers = NULL;
            }

            // move elements behind forward
            for(j = i; j + 1 < p_onvif_device_list->number_of_onvif_devices; ++j)
            {
                p_onvif_device_list->p_onvif_devices[j] = p_onvif_device_list->p_onvif_devices[j + 1];
            }

            --i;
            --(p_onvif_device_list->number_of_onvif_devices);
        }
    }

    // resize memory
    p_onvif_device_list->number_of_onvif_devices = device_service_address_list.size();
    p_onvif_device_temp = (onvif_device*)realloc(p_onvif_device_list->p_onvif_devices, p_onvif_device_list->number_of_onvif_devices * sizeof(onvif_device));
    if(NULL == p_onvif_device_temp)
    {
        p_onvif_device_list->devcie_list_lock = false;
        soap_destroy(&SoapForSearch);
        soap_end(&SoapForSearch);
        soap_done(&SoapForSearch);
        return -1;
    }
    else
    {
        p_onvif_device_list->p_onvif_devices = p_onvif_device_temp;
    }

    // release lock
    p_onvif_device_list->devcie_list_lock = false;

    soap_destroy(&SoapForSearch);
    soap_end(&SoapForSearch);
    soap_done(&SoapForSearch);

    return 0;
}

ONVIFOPERATION_API int add_onvif_device_manually(onvif_device_list* p_onvif_device_list, char* IP)
{
    onvif_device* p_onvif_device_temp = NULL;
    char* buffer = new char[256];
    if(NULL == buffer)
    {
        return -1;
    }

    if(!initialsuccess || NULL == p_onvif_device_list || NULL == IP)
    {
        delete buffer;
        return -1;
    }

    sprintf(buffer, "http://%s/onvif/device_service", IP);

    while(p_onvif_device_list->devcie_list_lock)
    {
        Sleep(10);
    }
    p_onvif_device_list->devcie_list_lock = true;

    p_onvif_device_list->number_of_onvif_devices += 1;
    p_onvif_device_temp = (onvif_device*)realloc(p_onvif_device_list->p_onvif_devices, p_onvif_device_list->number_of_onvif_devices * sizeof(onvif_device));
    if(NULL == p_onvif_device_temp)
    {
        delete buffer;
        p_onvif_device_list->devcie_list_lock = false;
        return -1;
    }
    else
    {
        p_onvif_device_list->p_onvif_devices = p_onvif_device_temp;
        memset(&p_onvif_device_list->p_onvif_devices[p_onvif_device_list->number_of_onvif_devices - 1], 0x0, sizeof(onvif_device));
        strncpy(
            p_onvif_device_list->p_onvif_devices[p_onvif_device_list->number_of_onvif_devices - 1].service_address_device_service.xaddr,
            buffer,
            256);
        strncpy(
            p_onvif_device_list->p_onvif_devices[p_onvif_device_list->number_of_onvif_devices - 1].IPv4,
            IP,
            17);
    }

    delete buffer;
    p_onvif_device_list->devcie_list_lock = false;
    return 0;
}

ONVIFOPERATION_API int set_onvif_device_authorization_information(onvif_device_list* p_onvif_device_list, char* IP, size_t index, char* username, char* password)
{
    size_t i;

    if(!initialsuccess || NULL == p_onvif_device_list)
    {
        return -1;
    }

    while(p_onvif_device_list->devcie_list_lock)
    {
        Sleep(10);
    }
    p_onvif_device_list->devcie_list_lock = true;

    if(NULL != IP)
    {
        if(17 < strnlen(IP, 17))
        {
            p_onvif_device_list->devcie_list_lock = false;
            return -1;
        }
        for(i = 0; i < p_onvif_device_list->number_of_onvif_devices; i++)
        {
            if(0 == strncmp(IP, p_onvif_device_list->p_onvif_devices[i].IPv4, 17))
            {
                index = i;
            }
        }
    }

    if(p_onvif_device_list->number_of_onvif_devices <= index)
    {
        p_onvif_device_list->devcie_list_lock = false;
        return -1;
    }

    strncpy(
        p_onvif_device_list->p_onvif_devices[index].username,
        username,
        50);

    strncpy(
        p_onvif_device_list->p_onvif_devices[index].password,
        password,
        50);

    p_onvif_device_list->devcie_list_lock = false;

    return 0;
}

ONVIFOPERATION_API int get_onvif_device_information(onvif_device_list* p_onvif_device_list, char* IP, size_t index)
{
    _tds__GetDeviceInformation          tds__GetDeviceInformation;
    _tds__GetDeviceInformationResponse  tds__GetDeviceInformationResponse;
    _tds__GetNetworkInterfaces          tds__GetNetworkInterfaces;
    _tds__GetNetworkInterfacesResponse  tds__GetNetworkInterfacesResponse;
    size_t i;
    soap soapDeviceInfo;

    if(!initialsuccess || NULL == p_onvif_device_list)
    {
        return -1;
    }

    while(p_onvif_device_list->devcie_list_lock)
    {
        Sleep(10);
    }
    p_onvif_device_list->devcie_list_lock = true;

    if(NULL != IP)
    {
        if(17 < strnlen(IP, 17))
        {
            p_onvif_device_list->devcie_list_lock = false;
            return -1;
        }
        for(i = 0; i < p_onvif_device_list->number_of_onvif_devices; i++)
        {
            if(0 == strncmp(IP, p_onvif_device_list->p_onvif_devices[i].IPv4, 17))
            {
                index = i;
            }
        }
    }

    if(p_onvif_device_list->number_of_onvif_devices <= index)
    {
        p_onvif_device_list->devcie_list_lock = false;
        return -1;
    }

    soap_init1(&soapDeviceInfo, SOAP_IO_DEFAULT | SOAP_XML_IGNORENS);

    soap_set_namespaces(&soapDeviceInfo, device_namespace);

    soap_wsse_add_UsernameTokenDigest(
        &soapDeviceInfo,
        "user",
        p_onvif_device_list->p_onvif_devices[index].username,
        p_onvif_device_list->p_onvif_devices[index].password);

    if(SOAP_OK != soap_call___tds__GetDeviceInformation(&soapDeviceInfo, p_onvif_device_list->p_onvif_devices[index].service_address_device_service.xaddr, NULL, &tds__GetDeviceInformation, &tds__GetDeviceInformationResponse))
    {
        p_onvif_device_list->devcie_list_lock = false;
        soap_destroy(&soapDeviceInfo);
        soap_end(&soapDeviceInfo);
        soap_done(&soapDeviceInfo);
        return -1;
    }

    strncpy(
        p_onvif_device_list->p_onvif_devices[index].device_information.firmware_version,
        tds__GetDeviceInformationResponse.FirmwareVersion,
        50);
    strncpy(
        p_onvif_device_list->p_onvif_devices[index].device_information.hardware_Id,
        tds__GetDeviceInformationResponse.HardwareId,
        10);
    strncpy(
        p_onvif_device_list->p_onvif_devices[index].device_information.manufacturer,
        tds__GetDeviceInformationResponse.Manufacturer,
        50);
    strncpy(
        p_onvif_device_list->p_onvif_devices[index].device_information.model,
        tds__GetDeviceInformationResponse.Model,
        50);
    strncpy(
        p_onvif_device_list->p_onvif_devices[index].device_information.serial_number,
        tds__GetDeviceInformationResponse.SerialNumber,
        50);


    soap_set_namespaces(&soapDeviceInfo, device_namespace);

    soap_wsse_add_UsernameTokenDigest(
        &soapDeviceInfo,
        "user",
        p_onvif_device_list->p_onvif_devices[index].username,
        p_onvif_device_list->p_onvif_devices[index].password);

    if(SOAP_OK != soap_call___tds__GetNetworkInterfaces(&soapDeviceInfo, p_onvif_device_list->p_onvif_devices[index].service_address_device_service.xaddr, NULL, &tds__GetNetworkInterfaces, &tds__GetNetworkInterfacesResponse))
    {
        p_onvif_device_list->devcie_list_lock = false;
        soap_destroy(&soapDeviceInfo);
        soap_end(&soapDeviceInfo);
        soap_done(&soapDeviceInfo);
        return -1;
    }

    strncpy(
        p_onvif_device_list->p_onvif_devices[index].device_information.MAC_address,
        tds__GetNetworkInterfacesResponse.NetworkInterfaces[0].Info->HwAddress,
        50);

    p_onvif_device_list->devcie_list_lock = false;

    soap_destroy(&soapDeviceInfo);
    soap_end(&soapDeviceInfo);
    soap_done(&soapDeviceInfo);

    return 0;
}

ONVIFOPERATION_API int get_onvif_device_service_addresses(onvif_device_list* p_onvif_device_list, char* IP, size_t index)
{
    size_t                      i;
    _tds__GetServices           tds__GetServices;
    _tds__GetServicesResponse   tds__GetServicesResponse;
    soap                        soapServiceAddr;

    if(!initialsuccess || NULL == p_onvif_device_list)
    {
        return -1;
    }

    while(p_onvif_device_list->devcie_list_lock)
    {
        Sleep(10);
    }
    p_onvif_device_list->devcie_list_lock = true;

    if(NULL != IP)
    {
        if(17 < strnlen(IP, 17))
        {
            p_onvif_device_list->devcie_list_lock = false;
            return -1;
        }
        for(i = 0; i < p_onvif_device_list->number_of_onvif_devices; i++)
        {
            if(0 == strncmp(IP, p_onvif_device_list->p_onvif_devices[i].IPv4, 17))
            {
                index = i;
            }
        }
    }

    if(p_onvif_device_list->number_of_onvif_devices <= index)
    {
        p_onvif_device_list->devcie_list_lock = false;
        return -1;
    }

    soap_init1(&soapServiceAddr, SOAP_IO_DEFAULT | SOAP_XML_IGNORENS);

    tds__GetServices.IncludeCapability = xsd__boolean__false_;

    soap_set_namespaces(&soapServiceAddr, device_namespace);

    soap_wsse_add_UsernameTokenDigest(
        &soapServiceAddr,
        "user",
        p_onvif_device_list->p_onvif_devices[index].username,
        p_onvif_device_list->p_onvif_devices[index].password);

    if(SOAP_OK != soap_call___tds__GetServices(&soapServiceAddr, p_onvif_device_list->p_onvif_devices[index].service_address_device_service.xaddr, NULL, &tds__GetServices, &tds__GetServicesResponse))
    {
        p_onvif_device_list->devcie_list_lock = false;
        soap_destroy(&soapServiceAddr);
        soap_end(&soapServiceAddr);
        soap_done(&soapServiceAddr);
        return -1;
    }

    for(i = 0; i < tds__GetServicesResponse.__sizeService; i++)
    {
        if(NULL == tds__GetServicesResponse.Service[i].Version)
        {
            continue;
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/device/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_device_service.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_device_service.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_device_service.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_device_service.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/media/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_media.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_media.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_media.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_media.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/events/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_events.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_events.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_events.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_events.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver20/imaging/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_imaging.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_imaging.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_imaging.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_imaging.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/deviceIO/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_deviceIO.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_deviceIO.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_deviceIO.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_deviceIO.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver20/analytics/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_analytics.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_analytics.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_analytics.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_analytics.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/recording/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_recording.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_recording.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_recording.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_recording.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/search/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_search_recording.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_search_recording.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_search_recording.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_search_recording.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/replay/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_replay.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_replay.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_replay.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_replay.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }

        if(0 == strncmp(tds__GetServicesResponse.Service[i].Namespace, "http://www.onvif.org/ver10/receiver/wsdl", 256))
        {
            p_onvif_device_list->p_onvif_devices[index].service_address_receiver.major_version = tds__GetServicesResponse.Service[i].Version->Major;
            p_onvif_device_list->p_onvif_devices[index].service_address_receiver.minor_version = tds__GetServicesResponse.Service[i].Version->Minor;
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_receiver.namesapce,
                tds__GetServicesResponse.Service[i].Namespace,
                256);
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].service_address_receiver.xaddr,
                tds__GetServicesResponse.Service[i].XAddr,
                256);
        }
    }

    p_onvif_device_list->devcie_list_lock = false;

    soap_destroy(&soapServiceAddr);
    soap_end(&soapServiceAddr);
    soap_done(&soapServiceAddr);

    return 0;
}

ONVIFOPERATION_API int get_onvif_ipc_profiles(onvif_device_list* p_onvif_device_list, char* IP, size_t index)
{
    size_t                      i;
    _trt__GetProfiles           getProfiles;
    _trt__GetProfilesResponse   getProfilesResponse;
    _trt__GetStreamUri          getStreamUri;
    _trt__GetStreamUriResponse  getStreamUriResponse;
    soap                        soapIPCProfiles;

    if(!initialsuccess || NULL == p_onvif_device_list)
    {
        return -1;
    }

    while(p_onvif_device_list->devcie_list_lock)
    {
        Sleep(10);
    }
    p_onvif_device_list->devcie_list_lock = true;

    if(NULL != IP)
    {
        if(17 < strnlen(IP, 17))
        {
            p_onvif_device_list->devcie_list_lock = false;
            return -1;
        }
        for(i = 0; i < p_onvif_device_list->number_of_onvif_devices; i++)
        {
            if(0 == strncmp(IP, p_onvif_device_list->p_onvif_devices[i].IPv4, 17))
            {
                index = i;
            }
        }
    }

    if(p_onvif_device_list->number_of_onvif_devices <= index || 17 > strnlen(p_onvif_device_list->p_onvif_devices[index].service_address_media.xaddr, 256))
    {
        p_onvif_device_list->devcie_list_lock = false;
        return -1;
    }

    getStreamUri.StreamSetup = (tt__StreamSetup*)malloc(sizeof(tt__StreamSetup));
    if(NULL == getStreamUri.StreamSetup)
    {
        p_onvif_device_list->devcie_list_lock = false;
        return -1;
    }

    getStreamUri.StreamSetup->Transport = (tt__Transport*)malloc(sizeof(tt__Transport));
    if(NULL == getStreamUri.StreamSetup->Transport)
    {
        free(getStreamUri.StreamSetup);
        p_onvif_device_list->devcie_list_lock = false;
        return -1;
    }

    getStreamUri.StreamSetup->Transport->Tunnel = (tt__Transport*)malloc(sizeof(tt__Transport));
    if(NULL == getStreamUri.StreamSetup->Transport->Tunnel)
    {
        free(getStreamUri.StreamSetup->Transport);
        free(getStreamUri.StreamSetup);
        p_onvif_device_list->devcie_list_lock = false;
        return -1;
    }

    getStreamUri.StreamSetup->Stream = tt__StreamType__RTP_Unicast;
    getStreamUri.StreamSetup->Transport->Protocol = tt__TransportProtocol__RTSP;
    getStreamUri.StreamSetup->Transport->Tunnel->Protocol = tt__TransportProtocol__RTSP;
    getStreamUri.StreamSetup->Transport->Tunnel->Tunnel = NULL;
    getStreamUri.StreamSetup->__size = 1;
    getStreamUri.StreamSetup->__any = NULL;
    getStreamUri.StreamSetup->__anyAttribute = NULL;


    soap_init1(&soapIPCProfiles, SOAP_IO_DEFAULT | SOAP_XML_IGNORENS);

    soap_set_namespaces(&soapIPCProfiles, media_namespace);

    soap_wsse_add_UsernameTokenDigest(
        &soapIPCProfiles,
        "user",
        p_onvif_device_list->p_onvif_devices[index].username,
        p_onvif_device_list->p_onvif_devices[index].password);

    if(SOAP_OK != soap_call___trt__GetProfiles(&soapIPCProfiles, p_onvif_device_list->p_onvif_devices[index].service_address_media.xaddr, NULL, &getProfiles, &getProfilesResponse))
    {
        p_onvif_device_list->devcie_list_lock = false;
        free(getStreamUri.StreamSetup->Transport->Tunnel);
        free(getStreamUri.StreamSetup->Transport);
        free(getStreamUri.StreamSetup);
        soap_destroy(&soapIPCProfiles);
        soap_end(&soapIPCProfiles);
        soap_done(&soapIPCProfiles);
        return -1;
    }

    // free preverious onvif device profiles list
    if(NULL != p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles)
    {
        free(p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles);
    }

    p_onvif_device_list->p_onvif_devices[index].number_of_onvif_ipc_profiles = getProfilesResponse.__sizeProfiles;
    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles = (onvif_ipc_profile*)malloc(getProfilesResponse.__sizeProfiles * sizeof(onvif_ipc_profile));
    if(NULL == p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles)
    {
        p_onvif_device_list->devcie_list_lock = false;
        free(getStreamUri.StreamSetup->Transport->Tunnel);
        free(getStreamUri.StreamSetup->Transport);
        free(getStreamUri.StreamSetup);
        soap_destroy(&soapIPCProfiles);
        soap_end(&soapIPCProfiles);
        soap_done(&soapIPCProfiles);
        return -1;
    }
    memset(p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles, 0x0, getProfilesResponse.__sizeProfiles * sizeof(onvif_ipc_profile));

    for(i = 0; i < p_onvif_device_list->p_onvif_devices[index].number_of_onvif_ipc_profiles; ++i)
    {
        if(NULL != getProfilesResponse.Profiles)
        {
            if(NULL != getProfilesResponse.Profiles[i].AudioEncoderConfiguration)
            {
                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].AudioEncoderConfiguration.Bitrate =
                    getProfilesResponse.Profiles[i].AudioEncoderConfiguration->Bitrate;

                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].AudioEncoderConfiguration.encoding =
                    (audio_encoding)getProfilesResponse.Profiles[i].AudioEncoderConfiguration->Encoding;

                strncpy(
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].AudioEncoderConfiguration.Name,
                    getProfilesResponse.Profiles[i].AudioEncoderConfiguration->Name,
                    30);

                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].AudioEncoderConfiguration.SampleRate =
                    getProfilesResponse.Profiles[i].AudioEncoderConfiguration->SampleRate;

                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].AudioEncoderConfiguration.UseCount =
                    getProfilesResponse.Profiles[i].AudioEncoderConfiguration->UseCount;
            }

            if(NULL != getProfilesResponse.Profiles[i].AudioSourceConfiguration)
            {
                strncpy(
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].AudioSourceConfiguration.Name,
                    getProfilesResponse.Profiles[i].AudioSourceConfiguration->Name,
                    30);

                strncpy(
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].AudioSourceConfiguration.SourceToken,
                    getProfilesResponse.Profiles[i].AudioSourceConfiguration->SourceToken,
                    30);

                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].AudioSourceConfiguration.UseCount =
                    getProfilesResponse.Profiles[i].AudioSourceConfiguration->UseCount;
            }

            getStreamUri.ProfileToken = getProfilesResponse.Profiles[i].token;

            soap_set_namespaces(&soapIPCProfiles, media_namespace);

            soap_wsse_add_UsernameTokenDigest(
                &soapIPCProfiles,
                "user",
                p_onvif_device_list->p_onvif_devices[index].username,
                p_onvif_device_list->p_onvif_devices[index].password);

            soap_call___trt__GetStreamUri(
                &soapIPCProfiles,
                p_onvif_device_list->p_onvif_devices[index].service_address_media.xaddr,
                NULL,
                &getStreamUri,
                &getStreamUriResponse);

            if(NULL != getStreamUriResponse.MediaUri)
            {
                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].MediaUri.InvalidAfterConnect =
                    getStreamUriResponse.MediaUri->InvalidAfterConnect;

                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].MediaUri.InvalidAfterReboot =
                    getStreamUriResponse.MediaUri->InvalidAfterReboot;

                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].MediaUri.Timeout =
                    getStreamUriResponse.MediaUri->Timeout;

                strncpy(
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].MediaUri.URI,
                    getStreamUriResponse.MediaUri->Uri,
                    256);
            }

            strncpy(
                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].name,
                getProfilesResponse.Profiles[i].Name,
                30);

            strncpy(
                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].token,
                getProfilesResponse.Profiles[i].token,
                30);

            if(NULL != getProfilesResponse.Profiles[i].VideoEncoderConfiguration)
            {
                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.encoding =
                    (video_encoding)getProfilesResponse.Profiles[i].VideoEncoderConfiguration->Encoding;

                if(NULL != getProfilesResponse.Profiles[i].VideoEncoderConfiguration->H264)
                {
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.H264.GovLength =
                        getProfilesResponse.Profiles[i].VideoEncoderConfiguration->H264->GovLength;

                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.H264.Profile =
                        (H264Profile)getProfilesResponse.Profiles[i].VideoEncoderConfiguration->H264->H264Profile;
                }

                strncpy(
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.Name,
                    getProfilesResponse.Profiles[i].VideoEncoderConfiguration->Name,
                    30);

                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.Quality =
                    getProfilesResponse.Profiles[i].VideoEncoderConfiguration->Quality;

                if(NULL != getProfilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl)
                {
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.RateControl.BitrateLimit =
                        getProfilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl->BitrateLimit;

                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.RateControl.EncodingInterval =
                        getProfilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl->EncodingInterval;

                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.RateControl.FrameRateLimit =
                       getProfilesResponse.Profiles[i].VideoEncoderConfiguration->RateControl->FrameRateLimit;
                }

                if(NULL != getProfilesResponse.Profiles[i].VideoEncoderConfiguration->Resolution)
                {
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.Resolution.Height =
                        getProfilesResponse.Profiles[i].VideoEncoderConfiguration->Resolution->Height;

                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.Resolution.Width =
                        getProfilesResponse.Profiles[i].VideoEncoderConfiguration->Resolution->Width;
                }

                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoEncoderConfiguration.UseCount =
                    getProfilesResponse.Profiles[i].VideoEncoderConfiguration->UseCount;
            }

            if(NULL != getProfilesResponse.Profiles[i].VideoSourceConfiguration)
            {
                strncpy(
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoSourceConfiguration.Name,
                    getProfilesResponse.Profiles[i].VideoSourceConfiguration->Name,
                    30);

                strncpy(
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoSourceConfiguration.SourceToken,
                    getProfilesResponse.Profiles[i].VideoSourceConfiguration->token,
                    30);

                p_onvif_device_list->p_onvif_devices[index].p_onvif_ipc_profiles[i].VideoSourceConfiguration.UseCount =
                    getProfilesResponse.Profiles[i].VideoSourceConfiguration->UseCount;
            }
        }
    }

    p_onvif_device_list->devcie_list_lock = false;

    free(getStreamUri.StreamSetup->Transport->Tunnel);
    free(getStreamUri.StreamSetup->Transport);
    free(getStreamUri.StreamSetup);

    soap_destroy(&soapIPCProfiles);
    soap_end(&soapIPCProfiles);
    soap_done(&soapIPCProfiles);

    return 0;
}

ONVIFOPERATION_API int get_onvif_nvr_receivers(onvif_device_list* p_onvif_device_list, char* IP, size_t index)
{
    size_t                      i;
    _trv__GetReceivers          GetReceivers;
    _trv__GetReceiversResponse  GetReceiversResponse;
    soap                        soapNVRReceivers;

    if(!initialsuccess || NULL == p_onvif_device_list)
    {
        return -1;
    }

    while(p_onvif_device_list->devcie_list_lock)
    {
        Sleep(10);
    }
    p_onvif_device_list->devcie_list_lock = true;

    if(NULL != IP)
    {
        if(17 < strnlen(IP, 17))
        {
            p_onvif_device_list->devcie_list_lock = false;
            return -1;
        }
        for(i = 0; i < p_onvif_device_list->number_of_onvif_devices; i++)
        {
            if(0 == strncmp(IP, p_onvif_device_list->p_onvif_devices[i].IPv4, 17))
            {
                index = i;
            }
        }
    }

    if(p_onvif_device_list->number_of_onvif_devices <= index || 17 > strnlen(p_onvif_device_list->p_onvif_devices[index].service_address_receiver.xaddr, 256))
    {
        p_onvif_device_list->devcie_list_lock = false;
        return -1;
    }

    soap_init1(&soapNVRReceivers, SOAP_IO_DEFAULT | SOAP_XML_IGNORENS);

    soap_set_namespaces(&soapNVRReceivers, receiver_namespace);

    soap_wsse_add_UsernameTokenDigest(
        &soapNVRReceivers,
        "user",
        p_onvif_device_list->p_onvif_devices[index].username,
        p_onvif_device_list->p_onvif_devices[index].password);


    if(SOAP_OK != soap_call___trv__GetReceivers(&soapNVRReceivers, p_onvif_device_list->p_onvif_devices[index].service_address_receiver.xaddr, NULL, &GetReceivers, &GetReceiversResponse))
    {
        p_onvif_device_list->devcie_list_lock = false;
        soap_destroy(&soapNVRReceivers);
        soap_end(&soapNVRReceivers);
        soap_done(&soapNVRReceivers);
        return -1;
    }

    // free preverious onvif NVR receivers list
    if(NULL != p_onvif_device_list->p_onvif_devices[index].p_onvif_NVR_receivers)
    {
        free(p_onvif_device_list->p_onvif_devices[index].p_onvif_NVR_receivers);
    }

    p_onvif_device_list->p_onvif_devices[index].number_of_onvif_NVR_receivers = GetReceiversResponse.__sizeReceivers;
    p_onvif_device_list->p_onvif_devices[index].p_onvif_NVR_receivers = (onvif_NVR_receiver*)malloc(p_onvif_device_list->p_onvif_devices[index].number_of_onvif_NVR_receivers * sizeof(onvif_NVR_receiver));
    if(NULL == p_onvif_device_list->p_onvif_devices[index].p_onvif_NVR_receivers)
    {
        p_onvif_device_list->devcie_list_lock = false;
        soap_destroy(&soapNVRReceivers);
        soap_end(&soapNVRReceivers);
        soap_done(&soapNVRReceivers);
        return -1;
    }

    for(i = 0; i < p_onvif_device_list->p_onvif_devices[index].number_of_onvif_NVR_receivers; i++)
    {
        if(NULL == GetReceiversResponse.Receivers)
        {
            continue;
        }

        strncpy(
            p_onvif_device_list->p_onvif_devices[index].p_onvif_NVR_receivers[i].token,
            GetReceiversResponse.Receivers[i].Token,
            30);

        if(NULL != GetReceiversResponse.Receivers[i].Configuration)
        {
            strncpy(
                p_onvif_device_list->p_onvif_devices[index].p_onvif_NVR_receivers[i].configuration.media_URI,
                GetReceiversResponse.Receivers[i].Configuration->MediaUri,
                256);

            p_onvif_device_list->p_onvif_devices[index].p_onvif_NVR_receivers[i].configuration.mode =
                (ReceiverMode)GetReceiversResponse.Receivers[i].Configuration->Mode;

            if(NULL != GetReceiversResponse.Receivers[i].Configuration->StreamSetup)
            {
                if(NULL != GetReceiversResponse.Receivers[i].Configuration->StreamSetup->Transport)
                {
                    p_onvif_device_list->p_onvif_devices[index].p_onvif_NVR_receivers[i].configuration.stream_setup.protocol =
                        (TransportProtocol)GetReceiversResponse.Receivers[i].Configuration->StreamSetup->Transport->Protocol;
                }
                p_onvif_device_list->p_onvif_devices[index].p_onvif_NVR_receivers[i].configuration.stream_setup.stream =
                    (StreamType)GetReceiversResponse.Receivers[i].Configuration->StreamSetup->Stream;
            }
        }
    }

    p_onvif_device_list->devcie_list_lock = false;

    soap_destroy(&soapNVRReceivers);
    soap_end(&soapNVRReceivers);
    soap_done(&soapNVRReceivers);

    return 0;
}