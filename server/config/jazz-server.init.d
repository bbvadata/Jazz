#!/bin/bash

#    (c) 2018 kaalam.ai (The Authors of Jazz)
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

start(){
	echo "Starting jazz-server ..."
	/etc/jazz-server/jazz /etc/jazz-server/config/jazz_config.ini start

	exit $?
}

stop(){
	echo "Stopping jazz-server ..."
	/etc/jazz-server/jazz stop
}

status(){
	echo  "jazz-server status ..."
	/etc/jazz-server/jazz status

	exit $?
}

case "$1" in
	start)
		start
		;;
	stop)
		stop
		exit $?
		;;
	restart)
		stop
		start
		;;
	status)
		status
		;;
	*)
		echo $"Usage: $0 {start|stop|restart|status}"
esac

exit 1
