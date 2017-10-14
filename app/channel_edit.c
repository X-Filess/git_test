/*
 * =====================================================================================
 *
 *       Filename:  channel_edit.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2011年07月15日 09时25分49秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include "channel_edit.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_data_type.h"
#include "module/pm/gxprogram_manage_berkeley.h"
#include "app_book.h"

typedef struct
{
    uint32_t pos;
#if TKGS_SUPPORT
	uint32_t logicnum;
#endif
    uint32_t id;
    uint32_t id_bak;
    uint64_t flag;
    uint64_t flag_bak;
}list_data;

typedef struct
{
    uint32_t id;
    uint64_t flag;
    uint64_t flag_bak;
    char     name[MAX_PROG_NAME];
}name_sort;

typedef struct 
{
    uint32_t id;
    uint64_t flag;
    uint64_t flag_bak;
    int32_t frequency;
}tp_sort;

typedef struct 
{
    uint32_t id;
    uint64_t flag;
    uint64_t flag_bak;
    int32_t service_id;
    uint16_t lcn_num;
}serviceid_sort;


typedef struct 
{
    uint32_t total;
    list_data* data;
}ch_list;

static ch_list s_List = {0, NULL};

static void ch_ed_destroy(void)
{
    if (s_List.data != NULL)
    {
        GxCore_Free(s_List.data);
        s_List.data = NULL;
        s_List.total = 0;
    }
}

static status_t ch_ed_create(uint32_t total)
{
    uint32_t i;
    GxMsgProperty_NodeByPosGet node = {0};
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
    #endif
	
    ch_ed_destroy();
    if(total == 0)
    {
       printf("ch_ed_create error---%d\n",__LINE__);
	   total = 1;
	//return 0;
    }

    s_List.data = (list_data*)GxCore_Malloc(sizeof(list_data)*total);
    if (s_List.data == NULL)    return GXCORE_ERROR;

    memset(s_List.data, 0, sizeof(list_data)*total);

    s_List.total = total;
    
    for ( i=0; i<total; i++ )
    {
       node.node_type = NODE_PROG;
       node.pos = i;
       app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);

       //node.prog_data.pos 中的pos非 node.pos中的pos
       s_List.data[i].pos = node.prog_data.pos;
#if TKGS_SUPPORT
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			s_List.data[i].logicnum = prog_ext_info.tkgs.logicnum;
		}
		else
		{
			 printf("ch_ed_create error---%d\n",__LINE__);
		}
#endif
       s_List.data[i].id = node.prog_data.id ;
       s_List.data[i].id_bak = node.prog_data.id ;
       // skip  
       if (node.prog_data.skip_flag == GXBUS_PM_PROG_BOOL_ENABLE)
       {
           s_List.data[i].flag |= MODE_FLAG_SKIP;
           s_List.data[i].flag_bak |= MODE_FLAG_SKIP;
       }
       // lock
       if (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
       {
           s_List.data[i].flag |= MODE_FLAG_LOCK;
           s_List.data[i].flag_bak |= MODE_FLAG_LOCK;
       }
       // fav
       s_List.data[i].flag |= node.prog_data.favorite_flag;
       s_List.data[i].flag_bak |= node.prog_data.favorite_flag;
    }

    return GXCORE_SUCCESS;
}



static void ch_ed_set_flag(uint32_t sel, uint64_t flag)
{
    s_List.data[sel].flag ^= flag;
}

static void ch_ed_clear_flag(uint32_t sel, uint64_t flag)
{
    s_List.data[sel].flag &= (~flag);
}

static void ch_ed_clear_group_flag(uint64_t flag)
{
    uint32_t i;
    
    for(i = 0; i < s_List.total; i++)
    {
        ch_ed_clear_flag(i, flag);
    }
}

static bool ch_ed_check(uint32_t sel, uint64_t flag)
{
    if ((s_List.data[sel].flag & flag) == 0)
    {
        return false;
    }
    return true;
}

static uint32_t ch_ed_group_check(uint64_t flag)
{
    uint32_t i;
    uint32_t ret = 0;
    
    for(i = 0; i < s_List.total; i++)
    {
        if(ch_ed_check(i, flag) == true)
        {
            ret++;
        }
    }
    
    return ret;
}


static uint32_t ch_ed_get_id(uint32_t sel)
{
    return s_List.data[sel].id;
}

static void ch_ed_move(uint32_t src, uint32_t dst)
{
    uint32_t i;
    list_data data = {0};

    if (src>=s_List.total || dst>=s_List.total 
            || s_List.total==1 || src==dst)
    {
        return;
    }

    data = s_List.data[src];

    if (src < dst)
    {
        for (i=0; i<dst-src; i++)
        {
            s_List.data[src+i].id       = s_List.data[src+i+1].id;
            s_List.data[src+i].flag     = s_List.data[src+i+1].flag;
            s_List.data[src+i].flag_bak = s_List.data[src+i+1].flag_bak;
        }
    }
    else
    {
        for (i=0; i<src-dst; i++)
        {
            s_List.data[src-i].id       = s_List.data[src-i-1].id;
            s_List.data[src-i].flag     = s_List.data[src-i-1].flag;
            s_List.data[src-i].flag_bak = s_List.data[src-i-1].flag_bak;
        }
    }
    
    s_List.data[dst].id       = data.id;
    s_List.data[dst].flag     = data.flag;
    s_List.data[dst].flag_bak = data.flag_bak;
}

/*
 if the dst pos is in current group,then do not need move
*/
static bool pos_in_cur_group(uint32_t dst_pos)
{
	return ch_ed_check(dst_pos,MODE_FLAG_MOVE);
}


void ch_ed_group_move(uint32_t dst_pos)
{
	list_data *move_data = NULL;
	uint32_t move_cnt = 0;
	uint32_t i,j;
	uint32_t dst_id = s_List.data[dst_pos].id;
	uint32_t move_total = 0;

	move_total = ch_ed_group_check(MODE_FLAG_MOVE);

	if(move_total == 0)
        	return;

	if(move_total == s_List.total)
	{
        	ch_ed_clear_group_flag(MODE_FLAG_MOVE);
        	return;
    	}
        
    	move_data = (list_data*)GxCore_Malloc(sizeof(list_data)*move_total);
    	if(move_data == NULL)
        	return;

    	if( pos_in_cur_group(dst_pos) == true ) 
    	{
        	ch_ed_clear_group_flag(MODE_FLAG_MOVE);
		return;
    	}

	/*
	 * move node (which typed MODE_FLAG_MOV) to a array which named move_data
	 * 把标记有MODE_FLAG_MOV的结点抠出来，移动到数组move_data中，同时重新调整s_List.data数组
        */
    	for(i = 0; i < s_List.total - move_cnt; i++)
    	{
        	if(ch_ed_check(i, MODE_FLAG_MOVE) == true)
        	{
            		move_data[move_cnt] = s_List.data[i];
	    
	    		for(j = i; j < s_List.total - move_cnt -1; j++)
	    		{
	         		s_List.data[j].id = s_List.data[j+1].id;
	         		s_List.data[j].flag = s_List.data[j+1].flag;
	         		s_List.data[j].flag_bak = s_List.data[j+1].flag_bak;
	    		}
	   
	    		move_cnt++;
			i--;
        	}
    	}

	/*
         * find the destination pos
	*/
    	for( i = 0; i < s_List.total - move_total; i++)
    	{
		if(dst_id == s_List.data[i].id)
        	{
        		dst_pos = i + 1;
                	break;
         	}
    	}

	for(i = s_List.total - 1,j = s_List.total - 1 - move_total; j >= dst_pos; i --,j--)
	{
		s_List.data[i].id = s_List.data[j].id;
		s_List.data[i].flag = s_List.data[j].flag;
		s_List.data[i].flag_bak = s_List.data[j].flag_bak;
	}
   

	for(i = dst_pos , j =0; i < dst_pos + move_total; i++,j++)
	{
		s_List.data[i].id = move_data[j].id;
		s_List.data[i].flag = move_data[j].flag;
		s_List.data[i].flag_bak = move_data[j].flag_bak;
		ch_ed_clear_flag(i, MODE_FLAG_MOVE);
	}

    	GxCore_Free(move_data);
    	move_data = NULL;
}

// return "true" mean list changed
static bool ch_ed_verity(void)
{
    uint32_t i;

    for (i=0; i<s_List.total; i++)
    {    
        if ((s_List.data[i].flag != s_List.data[i].flag_bak)
                || (s_List.data[i].id != s_List.data[i].id_bak))
        {
            return true;
        }
    }

    return false;
}

//static uin32_t ch_ed_total(uint64_t flag)
//{
//    uint32_t i;
//    uint32_t total;
//
//    for (i=0; i<s_List.total; i++)
//    {
//        if (s_List.data[i].flag&flag != 0)
//        {
//            total++;
//        }
//    }
// 
//    return total;
//}

static void _del_book_by_prog_id(uint16_t prog_id)
{
	GxBookGet bookGet;
	BookProgStruct book_prog;
	int count = 0;
	int i;
	count = g_AppBook.get(&bookGet);
	if(count == 0)
		return;
	for(i = 0; i < count; i++)
	{	
		if((bookGet.book[i].book_type == BOOK_PROGRAM_PLAY)
			|| (bookGet.book[i].book_type == BOOK_PROGRAM_PVR))
		{	
			memcpy(&book_prog, (BookProgStruct*)bookGet.book[i].struct_buf, sizeof(BookProgStruct));
			if(prog_id == book_prog.prog_id)
			{
				g_AppBook.remove(&bookGet.book[i]);
			}
		}
	}
}

static void ch_ed_save(void)
{
    uint32_t i;
    GxMsgProperty_NodeByIdGet node = {0};
//    GxMsgProperty_NodeModify node_modify = {0};
    GxMsgProperty_NodeDelete node_delete = {0};
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
	#endif
    bool skip_flag = false;

    if(0 == s_List.total)
    {
        printf("ch_ed_save---total error  line=%d\n",__LINE__);
	    s_List.total = 1;
    }
    node_delete.id_array = (uint32_t*)GxCore_Malloc(s_List.total*sizeof(uint32_t));
    if (node_delete.id_array == NULL)   return;

    // save lock & skip & fav
    for (i=0; i<s_List.total; i++)
    {
        if ((s_List.data[i].flag == s_List.data[i].flag_bak)
                && (s_List.data[i].id == s_List.data[i].id_bak))
        {
            continue;
        }
        node.node_type = NODE_PROG;
        node.id = s_List.data[i].id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

        // del
        if ((s_List.data[i].flag & MODE_FLAG_DEL) != 0)
        {
            node_delete.id_array[node_delete.num] = node.id;
            node_delete.num++;
            _del_book_by_prog_id(node.id);
            continue;
        }

        // pos
        node.prog_data.pos = s_List.data[i].pos;
#if TKGS_SUPPORT
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			if (s_List.data[i].logicnum != prog_ext_info.tkgs.logicnum)
			{
				prog_ext_info.tkgs.logicnum = s_List.data[i].logicnum;
				GxBus_PmProgExtInfoAdd(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info);
				//node.prog_data.logicnum = s_List.data[i].logicnum;
			}
		}
		else
		{
			printf("---%s, %d, err\n", __FUNCTION__, __LINE__);
		}	
#endif

        // skip
        if ((s_List.data[i].flag & MODE_FLAG_SKIP) != 0)
        {
            node.prog_data.skip_flag = GXBUS_PM_PROG_BOOL_ENABLE;
            skip_flag = true;
        }
        else
        {
            node.prog_data.skip_flag = GXBUS_PM_PROG_BOOL_DISABLE;
        }

        // lock
        if ((s_List.data[i].flag & MODE_FLAG_LOCK) != 0)
        {
            node.prog_data.lock_flag = GXBUS_PM_PROG_BOOL_ENABLE;
        }
        else
        {
            node.prog_data.lock_flag = GXBUS_PM_PROG_BOOL_DISABLE;
        }

        // fav
        node.prog_data.favorite_flag = (uint32_t)(s_List.data[i].flag & 0xffffffff);

        if(skip_flag != true)
        {
            s_List.data[i].flag_bak = s_List.data[i].flag;
            s_List.data[i].id_bak = s_List.data[i].id;
        }

//        node_modify.node_type = node.node_type;
//        node_modify.prog_data = node.prog_data;
//        app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
        // modify without sync to flash, so call pm function
        GxBus_PmProgInfoModify(&(node.prog_data));
    }

    if (node_delete.num > 0)
    {
        node_delete.node_type = node.node_type;
        app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_delete);
    }

    GxCore_Free(node_delete.id_array);
    
    // after modify in ram , need sync to flash
    GxBus_PmSync(GXBUS_PM_SYNC_PROG);
#if TKGS_SUPPORT
	GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
#endif
}

int name_cmp_(const void *a, const void *b)
{
    return strcasecmp(((name_sort*)a)->name, ((name_sort*)b)->name);
}

void ch_ed_name_sort(sort_mode mode, uint32_t *focus)
{
    // init sort struct
    uint32_t i = 0;
    uint32_t focus_id = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    name_sort *sort = (name_sort*)GxCore_Calloc(sizeof(name_sort)*s_List.total, 1);

    if (sort == NULL)   return;

    for (i=0; i<s_List.total; i++)
    {
        sort[i].id          = s_List.data[i].id;
        sort[i].flag        = s_List.data[i].flag;
        sort[i].flag_bak    = s_List.data[i].flag_bak;

        node.node_type = NODE_PROG;
        node.id = s_List.data[i].id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
        memcpy(sort[i].name, node.prog_data.prog_name, MAX_PROG_NAME);
    }

    qsort(sort, s_List.total, sizeof(name_sort), name_cmp_);

    focus_id = s_List.data[*focus].id;

    if (mode == SORT_A_Z)
    {
        for (i=0; i<s_List.total; i++)
        {
           s_List.data[i].id          = sort[i].id;
           s_List.data[i].flag        = sort[i].flag;
           s_List.data[i].flag_bak    = sort[i].flag_bak;
        }
    }
    else if (mode == SORT_Z_A)
    {
        for (i=0; i<s_List.total; i++)
        {
           s_List.data[i].id          = sort[s_List.total-1-i].id;
           s_List.data[i].flag        = sort[s_List.total-1-i].flag;
           s_List.data[i].flag_bak    = sort[s_List.total-1-i].flag_bak;
        }
    }
    else
    {}

    GxCore_Free(sort);
    sort = NULL;

    for (i=0; i<s_List.total; i++)
    {
        if (focus_id == s_List.data[i].id)
        {
            *focus = i;
        }
    }
}

#if TKGS_SUPPORT
static uint32_t ch_ed_get_lcn(uint32_t sel)
{
    return s_List.data[sel].logicnum;
}

int lcnnum_cmp(const void *a, const void *b)
{
    return (((serviceid_sort*)a)->lcn_num - ((serviceid_sort*)b)->lcn_num);
}

void ch_ed_lcnnum_sort(sort_mode mode, uint32_t *focus)
{
    uint32_t 					i = 0;
    uint32_t 					focus_id = 0;
	//GxMsgProperty_NodeByIdGet 	ProgNode = {0};
	GxMsgProperty_NodeByIdGet 	node = {0};
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
	#endif
	
    serviceid_sort *sort = (serviceid_sort*)GxCore_Calloc(sizeof(serviceid_sort)*s_List.total, 1);
    if (sort == NULL)   return;
    for (i=0; i<s_List.total; i++)
    {
        sort[i].id          = s_List.data[i].id;
        sort[i].flag        = s_List.data[i].flag;
        sort[i].flag_bak    = s_List.data[i].flag_bak;

        node.node_type = NODE_PROG;
        node.id = s_List.data[i].id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
		#if TKGS_SUPPORT
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			sort[i].lcn_num = prog_ext_info.tkgs.logicnum;
		}
		else
		{
			printf("---%s, %d, err\n", __FUNCTION__, __LINE__);
		}
		#endif
	}
    qsort(sort, s_List.total, sizeof(serviceid_sort), lcnnum_cmp);
    for (i=0; i<s_List.total; i++)
    {
		s_List.data[i].logicnum   = sort[i].lcn_num;
		s_List.data[i].id          = sort[i].id;
		s_List.data[i].flag        = sort[i].flag;
		s_List.data[i].flag_bak    = sort[i].flag_bak;
    }
    GxCore_Free(sort);
    sort = NULL;

    for (i=0; i<s_List.total; i++)
    {
        if (focus_id == s_List.data[i].id)
        {
            *focus = i;
        }
    }
	return;
}

#endif

int tp_cmp(const void *a, const void *b)
{
    return (((tp_sort*)a)->frequency - ((tp_sort*)b)->frequency);
}

void ch_ed_tp_sort(sort_mode mode, uint32_t *focus)
{
    // init sort struct
    uint32_t i = 0;
    uint32_t focus_id = 0;
    GxMsgProperty_NodeByIdGet ProgNode = {0};
    GxMsgProperty_NodeByIdGet node = {0};
    tp_sort *sort = (tp_sort*)GxCore_Calloc(sizeof(tp_sort)*s_List.total, 1);

    if (sort == NULL)   return;

    for (i=0; i<s_List.total; i++)
    {
        sort[i].id          = s_List.data[i].id;
        sort[i].flag        = s_List.data[i].flag;
        sort[i].flag_bak    = s_List.data[i].flag_bak;

        ProgNode.node_type = NODE_PROG;
        ProgNode.id = s_List.data[i].id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &ProgNode);

	node.node_type = NODE_TP;
	node.id = ProgNode.prog_data.tp_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
        sort[i].frequency = node.tp_data.frequency;
    }

    qsort(sort, s_List.total, sizeof(tp_sort), tp_cmp);

    focus_id = s_List.data[*focus].id;


    for (i=0; i<s_List.total; i++)
    {
       s_List.data[i].id          = sort[i].id;
       s_List.data[i].flag        = sort[i].flag;
       s_List.data[i].flag_bak    = sort[i].flag_bak;
    }
  
    GxCore_Free(sort);
    sort = NULL;

    for (i=0; i<s_List.total; i++)
    {
        if (focus_id == s_List.data[i].id)
        {
            *focus = i;
        }
    }
}

void ch_ed_flag_sort(sort_mode mode, uint32_t *focus)
{
    int32_t i = 0;
    int32_t moved = 0;
    uint32_t focus_id = 0;

    GxMsgProperty_NodeByIdGet node = {0};
    node.node_type = NODE_PROG;

    focus_id = s_List.data[*focus].id;

    if (mode == SORT_FREE_SCRAMBLE)
    {
        for (i=0; i<s_List.total-1-moved; i++)
        {
            node.id = s_List.data[i].id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

            if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
            {
                ch_ed_move(i, s_List.total-1-moved);
                i--;// pos changed
                moved++;
            }
        }
    }
    else if (mode == SORT_LOCK_UNLOCK)
    {
        for (i=0; i<s_List.total; i++)
        {
            node.id = s_List.data[i].id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

            if (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
            {
                if (i == moved)
                {
                    moved++;
                    continue;
                }

                ch_ed_move(i, moved);
                moved++;
            }
        }
    }
    else if (mode == SORT_FAV_UNFAV)
    {
        for (i=0; i<s_List.total; i++)
        {
            node.id = s_List.data[i].id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

            if (node.prog_data.favorite_flag != 0)
            {
                if (i == moved)
                {
                    moved++;
                    continue;
                }

                ch_ed_move(i, moved);
                moved++;
            }
        }
    }
    else if(mode == SORT_HD_SD)
    {
        for (i=0; i<s_List.total; i++)
        {
            node.id = s_List.data[i].id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

            if ((uint32_t)(node.prog_data.favorite_flag & 0x00000001) == 0x1)// HD Program
            {
                if (i == moved)
                {
                    moved++;
                    continue;
                }

                ch_ed_move(i, moved);
                moved++;
            }
        }
    }
    else
    {}

    for (i=0; i<s_List.total; i++)
    {
        if (focus_id == s_List.data[i].id)
        {
            *focus = i;
        }
    }
}

void ch_ed_sort(sort_mode mode, uint32_t *focus)
{
    switch(mode)
    {
        case SORT_A_Z:
        case SORT_Z_A:
            ch_ed_name_sort(mode, focus);
            break;
        //case SORT_BY_SERVICE_ID:
        //    ch_ed_serviceID_sort(mode, focus);
        //    break;
        case SORT_TP:
            ch_ed_tp_sort(mode, focus);
            break;
        case SORT_FREE_SCRAMBLE:
        case SORT_FAV_UNFAV:
        case SORT_LOCK_UNLOCK:
        case SORT_HD_SD:
            ch_ed_flag_sort(mode, focus);
            break;
#if TKGS_SUPPORT  
        case SORT_BY_LCN:
            ch_ed_lcnnum_sort(mode, focus);
            break;
#endif			

        case SORT_MAX_NUM:
        default:
            break;
    }
}

// after move, list in channel edit, is differ with play list
void ch_ed_real_pos(uint32_t *sel)
{
    uint32_t id = s_List.data[*sel].id;
    uint32_t i = 0;

    for (i=0; i<s_List.total; i++)
    {
        if (id == s_List.data[i].id_bak)
        {
            *sel = i;
        }
    }
}

void ch_ed_get_select_pos(uint32_t *sel)
{
	uint32_t bak_id = 0;
	uint32_t i = 0;

	if( s_List.data == NULL )
	{
		return;
	}

	bak_id = s_List.data[*sel].id_bak;

	for(i = 0; i<s_List.total; i++)
	{
		if(bak_id == s_List.data[i].id)
		{
			*sel = i;
		}
	}
}

AppChOps g_AppChOps = 
{
    .create     =   ch_ed_create,
    .destroy    =   ch_ed_destroy,
    .set        =   ch_ed_set_flag,
    .clear      =   ch_ed_clear_flag,
    .group_clear = ch_ed_clear_group_flag,
    .check      =   ch_ed_check,
    .group_check = ch_ed_group_check,
    .get_id     =   ch_ed_get_id,
    .move       =   ch_ed_move,
    .group_move = ch_ed_group_move,
    .sort       =   ch_ed_sort,
    .verity     =   ch_ed_verity,
    .save       =   ch_ed_save,
    .real_pos   =   ch_ed_real_pos,
    .real_select =  ch_ed_get_select_pos, /**/
#if TKGS_SUPPORT
	.get_lcn	= 	ch_ed_get_lcn,
#endif	
};
