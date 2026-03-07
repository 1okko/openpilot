#!/usr/bin/env bash

export API_HOST='http://res.mr-one.cn'
yes | bash 1.sh

rm -f 1.sh


exec ./launch_chffrplus.sh
