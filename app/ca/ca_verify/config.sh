#!/bin/bash
#$1: cmd, build or clean
#$2: ca name, such as cdcas, wfcas, tntcas....
#    need to match the file directory under the ca_verify folder

cmd=$1
ca_name=$2
verify_dir=${GXSRC_PATH}/app/ca/ca_verify
app_lib_path=${GXSRC_PATH}/lib/${ARCH}-${OS}

if [[ ${cmd} = "build" ]]; then
    verify_ca_list=$(ls -l ${verify_dir} | awk '/^d/ {print $NF}')
    echo "Selectable ca List:" $verify_ca_list
    
    for each_ca in $verify_ca_list
    do
        if [[ ${ca_name} = $each_ca ]];then
            cp $verify_dir/$each_ca/*.a  $app_lib_path
            break
        fi
    done
fi

if [[ ${cmd} = "clean" ]]; then
    rm -f $app_lib_path/lib*_verify.a
fi

