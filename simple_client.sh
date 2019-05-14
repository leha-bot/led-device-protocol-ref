function print_usage()
{
	echo "Usage: $0 <pipe_name>"
	return 0;
}

function send_input()
{
	echo "Will write to pipe \"$1\""
	read -p "C: "
	echo $REPLY
	echo $REPLY > $1
	read $1
	echo $REPLY
}

# script entry point
if [ $# == 0 ]
then
	print_usage;
	exit 0;
else
	PIPE_NAME="${1}"
	send_input $PIPE_NAME;
	exit 0;
fi

