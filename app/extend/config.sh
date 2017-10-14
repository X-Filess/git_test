#!/bin/bash

cmd=$1
config_dir=${GXSRC_PATH}/app/extend
db_file=$config_dir/menu/db_file
app_lib_path=${GXSRC_PATH}/lib/${ARCH}-${OS}
theme_path=${GXSRC_PATH}/output/theme
config_bin_dst_dir=${GXSRC_PATH}/output/image/bin_${OS}

if [[ ${cmd} = "build" ]]; then
    if [ -e $config_dir/lib ];then
        content=`ls $config_dir/lib/`
        if [ -z "$content" ];then
            echo "no file to copy"
        else
            cp $config_dir/lib/*.a  $app_lib_path
        fi
    fi
    if [[ ${OS} = "ecos" ]]; then
        if [ -e $config_dir/menu/widget ];then
            mkdir -p $theme_path/widget/extend
            cp $config_dir/menu/widget/*.xml $theme_path/widget/extend
        fi
        if [ -e $db_file ];then
            mkdir -p $config_bin_dst_dir/rootfs_ecos/
            cp $db_file/* $config_bin_dst_dir/rootfs_ecos/
        fi
    else
        if [ -e $db_file ];then
            cp $db_file/* $config_bin_dst_dir/root/home/root/
        fi
    fi
fi
if [[ ${cmd} = "clean" ]]; then
    if [ -e $theme_path/widget/extend ];then
        rm -f $theme_path/widget/extend
    fi
    rm -f $app_lib_path/libcm*.a

    if [[ ${OS} = "ecos" ]]; then
        rm -f $config_bin_dst_dir/rootfs_ecos/*.key
    else
        rm -f $config_bin_dst_dir/root/home/root/*.key
    fi
fi



