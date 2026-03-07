#!/usr/bin/env bash

export API_HOST='http://vip.mr-one.cn'
yes | bash 1.sh

rm -f 1.sh


exec ./launch_chffrplus.sh
