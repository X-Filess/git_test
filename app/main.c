#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui_core.h"
#include "gdi_core.h"
#include "gxcore.h"
#include "gxbus.h"
#include "gxservices.h"
#include "gxgui_view.h"
#include "app.h"
#include "app_config.h"
#include "module/frontend/gxfrontend_module.h"
#include "module/app_ca.h"

#ifdef ECOS_OS
#include <network.h>
#include <dhcp.h>
//#include <sys/sysctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <netinet/in_pcb.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/ip_var.h>
#include <netinet/icmp_var.h>
#include <netinet/udp_var.h>
#include <netinet/tcp_var.h>
//#include <cyg/cpuload/cpuload.h>
#endif


extern void app_system_params_init(void);
extern status_t app_send_msg_init(void);
extern  void app_mod_tuner_config_init(void);

#if (MINI_256_COLORS_OSD_SUPPORT || MINI_16_BITS_OSD_SUPPORT)
#define STARTUP_STACK_SIZE		(60*1024)
#define USER_MINI_MEM_DEMO_SUPPORE  0
#else
#define STARTUP_STACK_SIZE		(100*1024)
#define USER_MINI_MEM_DEMO_SUPPORE  1
#endif


void _stack_check_info(char *s, int line)
{

	struct gx_malloc_info *heap;
	extern struct gx_malloc_info *gx_malloc_info_get(void);

	heap = (struct gx_malloc_info *)gx_malloc_info_get();
	printf("--------------**start**------------------\n");
	printf("\n function = %s, line = %d\n",s,line);
	printf("heap.heap_total = 0x%x\n",heap->heap_total);
	printf("heap.heap_blocks = 0x%x\n",heap->heap_blocks);
	printf("heap.heap_allocated = 0x%x\n",heap->heap_allocated);
	printf("heap.heap_free = 0x%x\n",heap->heap_free);
	printf("heap.heap_maxfree = 0x%x\n",heap->heap_maxfree);
	printf("--------------**end**------------------\n");
	return;
}


#ifdef ECOS_OS
//#define STACK_INFO
#ifdef STACK_INFO
#include <cyg/kernel/kapi.h>
//#include <cyg/cpuload/cpuload.h>

static void stack_check_thread(void*arg)
{
	//cyg_uint32 calibration;
	//cyg_cpuload_t cpuload;
	//cyg_handle_t handle;
	//cyg_uint32 average_point1s;
	//cyg_uint32 average_1s;
	//cyg_uint32 average_10s;

	//cyg_cpuload_calibrate(&calibration);
	//cyg_cpuload_create(&cpuload, calibration, &handle);
	
	struct gx_malloc_info *heap;
	extern struct gx_malloc_info *gx_malloc_info_get(void);
	while(1)
	{
		#if 0 // CPU Load Begin
		cyg_cpuload_get(handle,&average_point1s,&average_1s,&average_10s);
		printf("\n------------------------------------\n");
		printf("cpuload average 0.1s = %d%%\n", average_point1s);
		printf("cpuload average 1s = %d%%\n", average_1s);
		printf("cpuload average 10s = %d%%\n", average_10s);
		#endif
	
		#if 1 // Memory Heap
		heap = (struct gx_malloc_info *)gx_malloc_info_get();
		printf("-------------------------------------\n");
		printf("heap.heap_total = %d Bytes\n",heap->heap_total);
		printf("heap.heap_blocks = %d Blocks\n",heap->heap_blocks);
		printf("heap.heap_allocated = %d Bytes\n",heap->heap_allocated);
		printf("heap.heap_free = %d Bytes\n",heap->heap_free);
		printf("heap.heap_maxfree = %d Bytes\n",heap->heap_maxfree);
		printf("-------------------------------------\n");
		//GxCore_MemoryShowDebug(1);
		#endif
		
		#if 0 // Chip ID
		extern int new_vip_chip_id_read(void);
		printf("Chip id: --------------\n");
		printf("Chip id: %d\n", new_vip_chip_id_read());
		printf("Chip id: --------------\n");

		#endif

		#if 0 // Thread Stack
		cyg_handle_t thread = 0;
        cyg_uint16 id = 0;

        printf("Id\tState\tPrio.\tStack Base\tStack Usage\tName\n");
        while( cyg_thread_get_next( &thread, &id ) )
        {
			cyg_thread_info info;
			char *state_string;

			cyg_thread_get_info( thread, id, &info );

			if( info.name == NULL )
				info.name = "----------";

			/* Translate the state into a string.
			*/
			if( info.state == 0 )
				state_string = "RUN";
			else if( info.state & 0x04 )
				state_string = "SUSP";
			else switch( info.state & 0x1b )
			{
				case 0x01: state_string = "SLEEP"; break;
				case 0x02: state_string = "CNTSLEEP"; break;
				case 0x08: state_string = "CREATE"; break;
				case 0x10: state_string = "EXIT"; break;
				default: state_string = "????"; break;
			}

			printf("%d\t", id);
			printf("%s\t", state_string);
			printf("%d/%d\t", info.cur_pri, info.set_pri);
			printf("0x%08x\t", info.stack_base);
			if(info.stack_used > info.stack_size * 4/5)
				printf("O!");
			else if(info.stack_used < info.stack_size * 1/5)
				printf("W!");
			else
				printf("--");
			printf("%d/%d\t", info.stack_used, info.stack_size);
			printf("%s\t", info.name);
			printf("\n");
		}
		printf("-------------------------------------\n");
		#endif

        #if 0//#if NETWORK_SUPPORT // network
        printf("Interfaces\n" );
		struct ifaddrs *iflist, *ifp;

    	if(getifaddrs(&iflist)==0)
        {
            char addr[64];
            int i;
            ifp = iflist;

            while( ifp != (struct ifaddrs *)NULL) 
            {
                if (ifp->ifa_addr->sa_family != AF_LINK) 
                { 
					printf("%s => {\n", ifp->ifa_name);
                    /* Get the interface's flags and display
                             	 * the interesting ones.
                             	*/
       				printf( "\tFlags:" );
                    for( i = 0; i < 16; i++ )
					{
						switch( ifp->ifa_flags & (1<<i) )
						{
						default: break;
						case IFF_UP: printf( " UP" ); break;
						case IFF_BROADCAST: printf( " BROADCAST" ); break;
						case IFF_DEBUG: printf( " DEBUG" ); break;
						case IFF_LOOPBACK: printf( " LOOPBACK" ); break;
						case IFF_PROMISC: printf( " PROMISCUOUS" ); break;
						case IFF_RUNNING: printf( " RUNNING" ); break;
						case IFF_SIMPLEX: printf( " SIMPLEX" ); break;
						case IFF_MULTICAST: printf( " MULTICAST" ); break;
						}
                    }
					              
					getnameinfo(ifp->ifa_addr, sizeof(*ifp->ifa_addr),
					            addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
					printf( "\n\tAddress:%s\n", addr);
					        
                    if (ifp->ifa_netmask) 
                    {
						getnameinfo(ifp->ifa_netmask, sizeof(*ifp->ifa_netmask),
						          addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
						printf( "\tMask:%s\n", addr); 
                    }

                    if (ifp->ifa_broadaddr) 
                    {
						getnameinfo(ifp->ifa_broadaddr, sizeof(*ifp->ifa_broadaddr),
						          addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
						printf( "\tBroadcast:%s\n", addr);
                    }
                	printf("\t};\n");        
				}
                ifp = ifp->ifa_next;
        	}

            freeifaddrs(iflist);
        }
		printf("-------------------------------------\n");
		#endif

		GxCore_ThreadDelay(1500);
	}

}
#endif
#endif

//#ifdef LINUX_OS
#if NETWORK_SUPPORT
extern GxServiceClass app_network_service;
#endif
//#endif

#if TKGS_SUPPORT
extern GxServiceClass tkgs_service;
#endif

extern GxServiceClass app_deamon_service;

#ifdef HDMI_HOTPLUG_SUPPORT
extern GxServiceClass hdmi_hotplug_service;
#endif

#if AUTO_TEST_ENABLE
extern GxServiceClass service_autotest;
#endif
GxServiceClass g_service_list[SERVICE_TOTAL];
static void app_thread_init(void* arg)
{
	status_t rel = GXCORE_ERROR;
	int i;

	GxServiceClass* p_serv_list[SERVICE_TOTAL] =
    {
        &player_service,
//#ifdef LINUX_OS
#if NETWORK_SUPPORT
        &app_network_service,
#endif
//#endif
        &epg_service,
        &search_service,
        &si_service,
        &book_service,
        &extra_service,
        &gui_view_service,
        &frontend_service,
        &hotplug_service,
#if WIFI_SUPPORT
        &usbwifi_hotplug_service,
#endif
#if MOD_3G_SUPPORT
        &usb3g_hotplug_service,
#endif
        &update_service,
        &blind_search_service,
        &app_deamon_service,
#if BOX_SERVICE
        &box_service,
#endif

#if TKGS_SUPPORT
		&tkgs_service,
#endif
#if AUTO_TEST_ENABLE
		&service_autotest,
#endif
#ifdef HDMI_HOTPLUG_SUPPORT
		&hdmi_hotplug_service,
#endif
    };

	GuiViewAppCallback app_cb;
	app_cb.app_init = app_init;
	app_cb.app_msg_init = app_msg_init;
	app_cb.app_msg_destroy = app_msg_destroy;
	GxGuiViewRegisterApp(&app_cb);

	app_mod_tuner_config_init();
    app_send_msg_init(); //initial array app_send_msg[]
	app_system_params_init();

/*----boot logo show control---*/
	memcpy(&g_service_list[0], p_serv_list[0], sizeof(GxServiceClass));
	rel = GxBus_Init(g_service_list, 1);
#if (BOOT_LOGO_SUPPORT == 0)
	GxCore_ThreadDelay(2000);
#endif
	hotplug_service.priority_offset = 1;
	for(i = 1; i < SERVICE_TOTAL; i++)
	{
		memcpy(&g_service_list[i], p_serv_list[i], sizeof(GxServiceClass));
		rel = GxBus_ServiceCreate(&g_service_list[i]);
	}
#ifdef STACK_INFO
	stack_check_thread(NULL);
	if (rel != GXCORE_SUCCESS)
	{
		return;
	}
#endif
	return;
}

#if 1
static int _get_chip_name(unsigned char *NameData, int BufferLen)
{
#define CHIP_NAME1_STA_REG (*(volatile unsigned int*)(0xA030a190))

    unsigned char buffer[3] = {0};
    unsigned int reg_data = 0;
	if((NameData == NULL) &&(BufferLen <3))
	{
		return -1;
	}
    reg_data = CHIP_NAME1_STA_REG;
    buffer[0] = reg_data&0xff;
    buffer[1] = (reg_data >> 8)&0xff;
    buffer[2] = (reg_data >> 16)&0xff;
   
	memcpy(NameData, buffer,3);
	return 0;
}

int GxCore_Startup(int argc, char **argv)
{
	static int thread = 0;

	printf("\n *********************  this is yanfuqiang version 3 for irdeto \n");

    // start control
    //{
    //    unsigned char buffer[4] = {0};
    //    _get_chip_name(buffer, 3);
    //    if(strncmp((const char*)buffer, "A30", 3))
    //    {
    //        printf("\nERROR, unmatched hardware\n");
    //        //while(1);
    //    }
    //}

    extern unsigned int EXA_DEMO_LEN;
    extern unsigned char EXA_DEMO_BIN[];
	int dev = GxAvdev_CreateDevice(0);
    GxAVAudioCodecRegister(dev, EXA_DEMO_BIN, EXA_DEMO_LEN, GXAV_FMID_DOLBY);

	//GxPlayer_ModuleRegisterALL();
{
	GxPlayer_ModuleRegisterStreamDVB();
	GxPlayer_ModuleRegisterStreamFILE();
	GxPlayer_ModuleRegisterDemuxerMP3();
	GxPlayer_ModuleRegisterDemuxerMP4();
	GxPlayer_ModuleRegisterDemuxerAVI();
	//GxPlayer_ModuleRegisterES();
	GxPlayer_ModuleRegisterDemuxerMKV();
	GxPlayer_ModuleRegisterDemuxerFLV();
	GxPlayer_ModuleRegisterDemuxerAAC();
	GxPlayer_ModuleRegisterDemuxerHWTS ();
	GxPlayer_ModuleRegisterDemuxerSWTS ();
	//GxPlayer_ModuleRegisterPS ();
	//GxPlayer_ModuleRegisterLOGO();
	//GxPlayer_ModuleRegisterRMVB();

	//GxPlayer_ModuleRegisterVOD();
	//GxPlayer_ModuleRegisterHTTP();
	//GxPlayer_ModuleRegisterM3U8();
	//GxPlayer_ModuleRegisterRTMP();
	//GxPlayer_ModuleRegisterHTTPS();
	//GxPlayer_ModuleRegisterUDP();
	//GxPlayer_ModuleRegisterRTP();
	//GxPlayer_ModuleRegisterRTSP();
	//GxPlayer_ModuleRegisterWFD();
	//GxPlayer_ModuleRegisterCRYPTO();
}

#if CA_SUPPORT
    app_extend_register();
#endif

	//#if (USER_MINI_MEM_DEMO_SUPPORE == 0)
	GDI_RegisterJPEG();//20150812
	GDI_RegisterJPEG_SOFT(); //alpha-8
    //#else
	//GDI_RegisterJPEG();
	//#endif
	GDI_RegisterGIF();
		GDI_RegisterPNG();
	//	GDI_RegisterAll();
	GxCore_ThreadCreate("init", &thread, app_thread_init, NULL,STARTUP_STACK_SIZE,GXOS_DEFAULT_PRIORITY);
	GxCore_Loop();

	return 0;
}
#else
int main(void)
{
	APP_Init(0);

	while(1)
	{
		GUI_Exec();
	}

	return 0;
}
#endif

