function print_usage()
{
	echo "Usage: led-client [(-p=|--pipe=)<pipe_name>]"
	return 0;
}

function client_pipe()
{
	echo pipe $1 yay!..
	echo Pipe name len ${#1}
	if [ ${#1} == 0 ]
	then
		echo "Error: empty pipe name!"
		exit;
	fi
	echo pipe $1 yay!..
	read REQUEST
	echo $REQUEST > $1
}
if [ $# == 0 ]
then
	echo "Total args: $#"
	print_usage;
else
	# handle command line args
	# (%=* removes all chars after '=' in $1 arg)
	echo "Parse-whoyarse ${1%=*}"
	case "${1%=*}" in
		-h| -?)
			echo "Help me, obi-wan!!!";
			usage;;
		-p| --pipe)
			echo "Parsed pipe name as \"${1#*=}\""
			client_pipe ${1#*=}
			;;
	esac

fi
commands=(pipe help)
